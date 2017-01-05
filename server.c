#define _POSIX_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include <pthread.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <ctype.h>
#include <arpa/inet.h>

#include "server.h"

#define ERROR(format, args...)                                       \
    do {                                                             \
        fprintf(stderr, "%s: " format "\n", strerror(errno), args);  \
        return errno;                                                \
    } while (0)

struct job {
    struct server *server;
    FILE *client;
};

static void *handler_thread(void *arg) {
    struct job *job = arg;
    job->server->handler(job->client, job->server->arg);
    free(job);
    return NULL;
}

static void *server_thread(void *arg) {
    struct server *server = arg;
    for (;;) {
        int fd;
        if ((fd = accept(server->fd, NULL, NULL)) == -1) {
            const char *err = strerror(errno);
            fprintf(stderr, "cannot accept connections: %s\n", err);
            return NULL;
        }

        struct sockaddr_in addr;
        socklen_t len = sizeof(addr);
        getpeername(fd, (struct sockaddr *) &addr, &len );
        size_t sock_addr_length = 30;
        char ip_addr[sock_addr_length];
        char sock_addr[sock_addr_length];
        inet_ntop(AF_INET, (void *) &addr.sin_addr, ip_addr, (socklen_t) sock_addr_length);
        snprintf(sock_addr,sock_addr_length ,"%s:%d", ip_addr ,ntohs(addr.sin_port));
        printf("client connection success   : %s\n" , sock_addr);

        struct job *job = malloc(sizeof(*job));
        job->server = server;
        job->client = fdopen(fd, "r+");
        //job->client = fdopen(fd, "r+");
        pthread_t handler;
        if (pthread_create(&handler, NULL, handler_thread, job) != 0) {
            fprintf(stderr, "cannot start client thread\n");
            fclose(job->client);
        } else {
            pthread_detach(handler);
        }
    }
    return NULL;
}

int server_start(struct server *server) {
    struct sockaddr_in addr = {0};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(server->port);
    if ((server->fd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
        ERROR("%s", "could not create socket");

    int flag=1,len=sizeof(int);
    if( setsockopt(server->fd, SOL_SOCKET, SO_REUSEADDR, &flag, len) == -1) {
        perror("setsockopt");
        return 0;
    }

    if (bind(server->fd, (void *) &addr, sizeof(addr)) != 0)
        ERROR("%s", "could not bind");
    if (listen(server->fd, 1024) != 0)
        ERROR("%s", "could not listen");
    if (pthread_create(&server->thread, NULL, server_thread, server) != 0)
        ERROR("%s", "could create server thread");
    printf("server start OK! \n");
    return 0;
}

char *strupr(char *str , size_t len) {
    char *orign = str;
    int cnt = 0;
    for (; *str != '\0 ' && cnt < len; str++ , cnt++)
        *str = toupper(*str);
    return orign;
}

UserInfo *parserUseInfo(char *line) {
    UserInfo *userInfo = malloc(sizeof(UserInfo));
    char *username = userInfo->username;
    char *domain = userInfo->domain;
    size_t line_len = strlen(line);
    int u_idx = 0;
    int d_idx = 0;
    if(line_len>0){
        for (int i = 0 ; i<line_len ; i++){
            if(line[i]=='<'){
                i++;
                while(line[i]!='@' && i<line_len){
                    username[u_idx++]=line[i++];
                }
                username[u_idx]= '\0';
                i++;
                while(line[i]!='>' && i<line_len){
                    domain[d_idx++]=line[i++];
                }
                domain[d_idx] = '\0';
            }
        }
    }
    return userInfo;
}


/**
 * 忽略邮箱域名 只存储用户名
 */
void recipient_push(struct recipient **rs, char *email) {
    struct recipient *r = malloc(sizeof(*r));
    char *p;
    int index = 0;

    for (p = r->email; *email && *email != '@';  email++){
        if(*email == '<' || *email=='>' || *email==' '){
            continue;
        }
        *p = *email;
        p++;
    }

    *p = '\0';
    r->next = *rs;
    *rs = r;
}

struct recipient *recipient_pop(struct recipient **rs) {
    struct recipient *r = *rs;
    if (*rs)
        *rs = (*rs)->next;
    return r;
}


int strsplit (const char *str, char *parts[], const char *delimiter)
{
    char *pch;
    int i = 0;
    char *copy = NULL, *tmp = NULL;

    copy = strdup(str);
    if (! copy)
        goto bad;

    pch = strtok(copy, delimiter);

    tmp = strdup(pch);
    if (! tmp)
        goto bad;

    parts[i++] = tmp;

    while (pch) {
        pch = strtok(NULL, delimiter);
        if (NULL == pch) break;

        tmp = strdup(pch);
        if (! tmp)
            goto bad;

        parts[i++] = tmp;
    }

    free(copy);
    return i;

    bad:
    free(copy);
    for (int j = 0; j < i; j++)
        free(parts[j]);
    return -1;
}

void remove_line_break(char *line) {
    if(line==NULL || strlen(line)<=1){
        return;
    }
    size_t  line_len = strlen(line);
    if (line[line_len - 1] == '\n' && line[line_len - 2] == '\r') {
        line[line_len - 2] = '\0';
    }

}

void get_word(char *src, char *output) {
    if(src == NULL || output == NULL){
        return;
    }
    char *a = src, *b = output;
    while (isalnum(*a))
        *(b++) = *(a++);
    *b = '\0';
}
