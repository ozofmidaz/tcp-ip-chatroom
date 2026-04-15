#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <pthread.h>

#define PORT 6333
#define FILE_NAME "user.txt"
#define LOGIN 1
#define REGISTER 2

#include "struct.c"

int clients[100];
int count = 0;
char client_usernames[100][50];
// maps:
// clients[i]  ↔ client_usernames[i]
pthread_mutex_t lock;

void broadcast(char *msg, int exclude_fd)
{
    pthread_mutex_lock(&lock);

    for (int i = 0; i < count; i++)
    {
        if (clients[i] != exclude_fd)
        {
            send(clients[i], msg, strlen(msg), 0);
        }
    }

    pthread_mutex_unlock(&lock);
}

int register_user(char *username, char *password)
{
    FILE *fp;
    struct user_db user, temp;

    fp = fopen(FILE_NAME, "rb");
    if (fp != NULL)
    {
        while (fread(&temp, sizeof(temp), 1, fp))
        {
            if (strcmp(temp.username, username) == 0)
            {
                fclose(fp);
                return 1;
            }
        }
        fclose(fp);
    }

    strcpy(user.username, username);
    strcpy(user.password, password);
    user.status = 0;

    fp = fopen(FILE_NAME, "ab");
    fwrite(&user, sizeof(user), 1, fp);
    fclose(fp);

    return 2;
}

int login_user(char *username, char *password)
{
    FILE *fp;
    struct user_db temp;

    fp = fopen(FILE_NAME, "rb+");
    if (fp == NULL)
        return -1;

    while (fread(&temp, sizeof(temp), 1, fp))
    {
        if (strcmp(temp.username, username) == 0)
        {
            if (strcmp(temp.password, password) == 0)
            {
                temp.status = 1;

                fseek(fp, -sizeof(temp), SEEK_CUR);
                fwrite(&temp, sizeof(temp), 1, fp);
                fclose(fp);
                return 1;
            }
            else
            {
                fclose(fp);
                return 2;
            }
        }
    }

    fclose(fp);
    return -1;
}

void set_user_offline(char *username)
{
    FILE *fp;
    struct user_db temp;

    fp = fopen(FILE_NAME, "rb+");
    if (fp == NULL)
        return;

    while (fread(&temp, sizeof(temp), 1, fp))
    {
        if (strcmp(temp.username, username) == 0)
        {
            temp.status = 0;

            fseek(fp, -sizeof(temp), SEEK_CUR);
            fwrite(&temp, sizeof(temp), 1, fp);
            break;
        }
    }

    fclose(fp);
}

void *client_handler(void *arg)
{
    int client_fd = *(int *)arg;
    free(arg);

    while (1)
    {
        struct request req;
        char buffer[100];

        //   int ret = recv(client_fd, &req, sizeof(req), 0);

        int received = 0;
        int total = sizeof(req);

        while (received < total)
        {
            int r = recv(client_fd, ((char *)&req) + received, total - received, 0);
            if (r <= 0)
                break;
            received += r;
        }

        if (received <= 0)
        {
            char username[50] = "";

            pthread_mutex_lock(&lock);

            for (int i = 0; i < count; i++)
            {
                if (clients[i] == client_fd)
                {
                    // 🔥 GET USERNAME BEFORE REMOVING
                    strcpy(username, client_usernames[i]);

                    // REMOVE CLIENT
                    clients[i] = clients[count - 1];
                    strcpy(client_usernames[i], client_usernames[count - 1]);

                    count--;
                    break;
                }
            }

            pthread_mutex_unlock(&lock);

            close(client_fd);

            // 🔥 UPDATE FILE STATUS
            if (strlen(username) > 0)
            {
                set_user_offline(username);

                // 🔥 BROADCAST OFFLINE
                char msg[100];
                sprintf(msg, "User %s is OFFLINE\n", username);
                broadcast(msg, -1);
            }

            break;
        }
        if (req.option == LOGIN)
        {
            int result = login_user(req.username, req.password);

            if (result == 1)
            {
                strcpy(buffer, "Logged in successfully\n");

                pthread_mutex_lock(&lock);

                for (int i = 0; i < count; i++)
                {
                    if (clients[i] == client_fd)
                    {
                        strcpy(client_usernames[i], req.username);
                        break;
                    }
                }

                pthread_mutex_unlock(&lock);

                send(client_fd, buffer, strlen(buffer), 0);

                char msg[100];
                sprintf(msg, "User %s is ONLINE\n", req.username);
                broadcast(msg, -1);

                FILE *fp;
                struct user_db temp;
                char list[500] = "Online users:\n";

                fp = fopen(FILE_NAME, "rb");

                while (fread(&temp, sizeof(temp), 1, fp))
                {
                    if (temp.status == 1)
                    {
                        strcat(list, temp.username);
                        strcat(list, "\n");
                    }
                }

                fclose(fp);
                send(client_fd, list, strlen(list), 0);
            }

            else if (result == 2)
            {
                strcpy(buffer, "Password incorrect\n");
                send(client_fd, buffer, strlen(buffer), 0);
            }
            else if (result == -1)
            {
                strcpy(buffer, "User not found\n");
                send(client_fd, buffer, strlen(buffer), 0);
            }
        }
        else if (req.option == REGISTER)
        {
            int result = register_user(req.username, req.password);

            if (result == 1)
                strcpy(buffer, "Duplicate user-name\n");
            else
                strcpy(buffer, "Registered successfully\n");

            send(client_fd, buffer, strlen(buffer), 0);
        }
        else if (req.option == CHAT)
        {
            char msg[300];

            sprintf(msg, "[%s]: %s\n", req.sender, req.message);

            if (strcmp(req.receiver, "ALL") == 0)
            {
                // GROUP CHAT
                broadcast(msg, -1);
            }
            else
            {

                // SINGLE CHAT
                pthread_mutex_lock(&lock);

                int found = 0;

                for (int i = 0; i < count; i++)
                {
                    if (strcmp(client_usernames[i], req.receiver) == 0)
                    {
                        send(clients[i], msg, strlen(msg), 0);
                        found = 1;
                        break;
                    }
                }

                pthread_mutex_unlock(&lock);

                if (found == 0)
                {
                    char err[] = "User not found\n";
                    send(client_fd, err, strlen(err), 0);
                }
            }
        }
        else if (req.option == LOGOUT)
        {
            char username[50] = "";

            pthread_mutex_lock(&lock);

            for (int i = 0; i < count; i++)
            {
                if (clients[i] == client_fd)
                {
                    strcpy(username, client_usernames[i]);

                    // REMOVE CLIENT
                    clients[i] = clients[count - 1];
                    strcpy(client_usernames[i], client_usernames[count - 1]);

                    count--;
                    break;
                }
            }

            pthread_mutex_unlock(&lock);

            close(client_fd);

            if (strlen(username) > 0)
            {
                set_user_offline(username);

                char msg[100];
                sprintf(msg, "User %s is OFFLINE\n", username);
                broadcast(msg, -1);
            }

            break;
        }
    }

    return NULL;
}

int main()
{
    int server_fd, client_fd;
    struct sockaddr_in server_addr, client_addr;
    socklen_t addr_len = sizeof(client_addr);

    pthread_mutex_init(&lock, NULL);

    server_fd = socket(AF_INET, SOCK_STREAM, 0);

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr));
    listen(server_fd, 5);

    printf("Server listening...\n");

    while (1)
    {
        client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &addr_len);

        printf("Client connected\n");

        pthread_mutex_lock(&lock);
        clients[count++] = client_fd;
        pthread_mutex_unlock(&lock);

        pthread_t tid;
        int *pclient = malloc(sizeof(int));
        *pclient = client_fd;

        pthread_create(&tid, NULL, client_handler, pclient);
    }

    return 0;
}