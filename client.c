#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#include "struct.c"

#define PORT 6333
#define LOGIN 1
#define REGISTER 2


void client_chat(int sock)
{
    struct request req;

    req.option = CHAT;

    printf("Enter receiver (or ALL): ");
    scanf("%s", req.receiver);

    printf("Enter message: ");
    scanf(" %[^\n]", req.message);

    printf("Enter your username: ");
    scanf("%s", req.sender);

    send(sock, &req, sizeof(req), 0);
}

void client_login(int sock)
{
    struct request req;

    printf("--------------\n");
    printf("Enter username: ");
    scanf("%49s", req.username);

    printf("Enter password: ");
    scanf("%49s", req.password);

    req.option = LOGIN;
    printf("--------------\n");

    send(sock, &req, sizeof(req), 0);
}

void client_register(int sock)
{
    struct request req;

    printf("--------------\n");
    printf("Enter username: ");
    scanf("%49s", req.username);

    printf("Enter password: ");
    scanf("%49s", req.password);

    req.option = REGISTER;
    printf("--------------\n");

    send(sock, &req, sizeof(req), 0);
}

int main()
{
    int sock;
    int choice;
    struct sockaddr_in server_addr;

    sock = socket(AF_INET, SOCK_STREAM, 0);
    if(sock == -1)
    {
        perror("socket failed");
        return -1;
    }

    printf("Socket created\n");

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    if(connect(sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        perror("connect failed");
        return -1;
    }

    printf("Connected\n");

    // 🔥 RECEIVER PROCESS
    if(fork() == 0)
    {close(STDIN_FILENO);
        char msg[500];

        while(1)
        {
            int r = recv(sock, msg, sizeof(msg), 0);
            if(r > 0)
            {
                msg[r] = '\0';

                printf("\n\n========== MESSAGE ==========\n");
               char *line = strtok(msg, "\n");

while(line != NULL)
{
    printf("%s\n", line);
    line = strtok(NULL, "\n");
}
                printf("=============================\n");
             //   printf("\n>> ");   // 🔥 FIXED PROMPT
             sleep(1); 
                fflush(stdout);
            }
        }
    }

    // 🔥 MAIN MENU LOOP
    while(1)
    {
  
        printf("\n ||||||[MENU]||||||\n");
        printf("1. Register\n");
        printf("2. Login\n");
        printf("3. Chat\n");
        printf("4. Logout\n");
        printf("5. Exit\n");
        
        printf(">> ");  
        scanf("%d", &choice);

        if(choice == 1)
        {
            client_register(sock);
        }
        else if(choice == 2)
        {
            client_login(sock);
        }
        else if(choice == 3)
        {
            client_chat(sock);
        }
        else if(choice == 4)
        {
           struct request req;
           req.option = LOGOUT;

           send(sock, &req, sizeof(req), 0);
        }
           else if(choice == 5)
        {
           close(sock);
            break;
        }
    }

    return 0;
}