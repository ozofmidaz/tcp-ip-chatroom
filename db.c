#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "struct.c"



int main()
{
    FILE *fp;
    struct user_db user = {"user1", "pass1", 1, 100};
    fp = fopen("user.txt", "wb");
    if(fp == NULL)
    {
        perror("file open failed\n");
        return -1;
    }
    fwrite(&user, sizeof(user), 1, fp);
    fclose(fp);
    struct user_db temp;

    fp = fopen("user.txt", "rb");
    if(fp == NULL)
    {
        perror("File open failed");
        return -1;
    }

    fread(&temp, sizeof(temp), 1, fp);
    fclose(fp);

    printf("Username: %s\n", temp.username);
    printf("Password: %s\n", temp.password);
    printf("Status: %d\n", temp.status);
    printf("Sockfd: %d\n", temp.sockfd);

    // update status
    temp.status = 0;

    fp = fopen("user.txt", "wb");
    fwrite(&temp, sizeof(temp), 1, fp);
    fclose(fp);

    printf("User set to OFFLINE\n");

    return 0;
}

// struct user_db user;
// strcpy(user.username, input_username);
// strcpy(user.password, input_password);
// user.status = 0;
// user.sockfd = client_fd;