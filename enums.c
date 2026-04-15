#include <stdio.h>

// REQ_053
enum auth_option {
    REGISTER,
    LOGIN
};

// REQ_065, REQ_066, REQ_067
enum chat_option {
    SINGLE_CHAT,
    GROUP_CHAT,
    LOGOUT
};

int main()
{
    printf("Enums created\n");
    return 0;
}