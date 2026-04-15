#ifndef STRUCT_H
#define STRUCT_H

#define LOGIN 1
#define REGISTER 2
#define CHAT 3
#define LOGOUT 4

#define MAX 50

struct user_db
{
    char username[MAX];
    char password[MAX];
    int status;
};
struct request
{
    int option;

    char username[50];
    char password[50];

    char receiver[50];
    char message[200];

    char sender[50];
};

#endif