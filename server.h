#pragma once

#include <pthread.h>

typedef void (*handler_t)(FILE *client, void *arg);

#define DEBUG 1

/* Configuration */
const static int SMTP_PORT = 7725;
const static int POP3_PORT = 7110;
const static int ADMIN_PORT = 7777;     //邮件用户管理端口
const static char *DBFILE = "/home/maple/email.db";
const static char *SERVER_DOMAIN = "localhost";


struct server {
    unsigned short port;
    handler_t handler;
    void *arg;
    int fd;
    pthread_t thread;
};

typedef  struct _UserInfo{
    char username[128];
    char password[128];
    char domain[128];
} UserInfo;

struct recipient {
    char email[512];
    struct recipient *next;
};

UserInfo *parserUseInfo(char *line);

void recipient_push(struct recipient **rs, char *email);
struct recipient *recipient_pop(struct recipient **rs);

int server_start(struct server *server);

char *strupr(char *str , size_t len);

int strsplit (const char *str, char *parts[], const char *delimiter);