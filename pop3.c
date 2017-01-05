#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "database.h"
#include "pop3.h"
#include "server.h"

void pop3_handler(FILE *client, void *arg)
{
    pop3(client, (const char *) arg);
}

#define RESPOND(client, format, args...)        \
    fprintf(client, format "\r\n", args);       \
    printf("POP3 >>>: " format "\r\n\n", args)

void pop3(FILE *client, const char *dbfile)
{
    struct database db;
    if (database_open(&db, dbfile) != 0) {
        RESPOND(client, "%s", "-ERR");
        fclose(client);
        return;
    } else {
        RESPOND(client, "+OK %s", "POP3 server ready");
    }

    char user[512] = {0};
    char pwd[512] = {0};
    struct message *messages = NULL;
    UserInfo *userInfo = NULL;
    int user_auth_pass = 0;
    while (!feof(client)) {
        char line[512]="";
        char *f ;
        if((f = fgets(line, sizeof(line), client))==NULL){
            break;
        }
        if(DEBUG){
            printf("POP3 <<< : %s",line);
        }
        char command[5] = {line[0], line[1], line[2], line[3]};
        strupr(command, sizeof(command));
        if (strcmp(command, "USER") == 0) {

            if (strlen(line) > 5) {
                get_word(line+5 , user);
                RESPOND(client, "%s", "+OK");
            } else {
                RESPOND(client, "%s", "-ERR");
            }
            userInfo = database_get_user(&db , user);
            if(userInfo == NULL){
                RESPOND(client, "%s","-ERR user not found");
            }else{
                messages = database_list(&db, user);
            }
        } else if (strcmp(command, "PASS") == 0) {
            if(userInfo == NULL){
                RESPOND(client, "%s","-ERR user not found");
                continue;
            }
            if (strlen(line) > 5) {
                get_word(line+5 , pwd);

                if(strlen(pwd)>=1 && strcmp(pwd, userInfo->password)==0){
                    user_auth_pass = 1;
                    RESPOND(client, "%s", "+OK"); // don't care
                }else{
                    RESPOND(client, "%s", "-ERR auth fail"); // don't care
                }

            }

        } else if (strcmp(command, "STAT") == 0) {

            int count = 0, size = 0;
            for (struct message *m = messages; m; m = m->next) {
                count++;
                size += m->length;
            }
            RESPOND(client, "+OK %d %d", count, size);
        } else if (strcmp(command, "LIST") == 0) {
            if(!user_auth_pass){
                RESPOND(client, "%s","-ERR Command not valid in this state");
                continue;
            }
            RESPOND(client, "%s", "+OK");
            int i = 1;
            for (struct message *m = messages; m; m = m->next ,i++){
                RESPOND(client, "%d %d", i , m->length);
            }

            RESPOND(client, "%s", ".");
        }
        else if (strcmp(command, "UIDL") == 0) {
            if(!user_auth_pass){
                RESPOND(client, "%s","-ERR Command not valid in this state");
                continue;
            }
            int index = atoi(line + 4);
            RESPOND(client, "%s", "+OK");
            int i = 1;
            //int find = 0;
            for (struct message *m = messages; m; m = m->next ,i++){
                    RESPOND(client, "%d %d",  i , m->id);
            }
            RESPOND(client, "%s", ".");
        }
        else if (strcmp(command, "RETR") == 0) {
            if(!user_auth_pass){
                RESPOND(client, "%s","-ERR Command not valid in this state");
                continue;
            }
            int id = atoi(line + 4);
            int found = 0;
            int index = 1;
            for (struct message *m = messages; m; m = m->next , index++)
                if (index == id) {
                    found = 1;
                    RESPOND(client, "%s %d octets", "+OK" , m->length);
                    RESPOND(client, "%s", m->content);
                }
            RESPOND(client, "%s", found ? "." : "-ERR");
        } else if (strcmp(command, "DELE") == 0) {
            if(!user_auth_pass){
                RESPOND(client, "%s","-ERR Command not valid in this state");
                continue;
            }
            int id = atoi(line + 4);
            int found = 0;
            for (struct message  *m = messages; m; m = m->next) {
                if (m->id == id) {
                    RESPOND(client, "%s", "+OK");
                    database_delete(&db, id);
                    break;
                }
            }
            if (!found)
                RESPOND(client, "%s", "-ERR");
        }
//        else if (strcmp(command, "TOP ") == 0) {
//            RESPOND(client, "%s", "-ERR");
//            continue;
//
//            int id, lines;
//            sscanf(line, "TOP %d %d", &id, &lines);
//            int found = 0;
//            int index = 1;
//            for (struct message *m = messages; m; m = m->next , index++) {
//                if (index == id) {
//                    RESPOND(client, "%s", "+OK");
//                    found = 1;
//                    char *p = m->content;
//                    while (*p && memcmp(p, "\r\n\r\n", 4) != 0) {
//                        fputc(*p, client);
//                        p++;
//                    }
//                    if (*p) {
//                        p += 4;
//                        int line = 0;
//                        while (*p && line < lines) {
//                            if (*p == '\n')
//                                line++;
//                            p++;
//                        }
//                    }
//                    break;
//                }
//            }
//            RESPOND(client, "%s", found ? "\r\n." : "-ERR");
//        }
        else if(strcmp(command, "NOOP") == 0){

            RESPOND(client, "%s", "+OK");

        } else if (strcmp(command, "QUIT") == 0) {

            RESPOND(client, "%s", "+OK");
            break;
        } else {
            RESPOND(client, "%s", "-ERR command not found");
        }
    }

    if(messages){
        while (messages) {
            struct message *dead = messages;
            messages = messages->next;
            if(dead){
                free(dead);
            }
        }
    }
    if(client){
        fclose(client);
    }

    if(&db){
        database_close(&db);
    }
    printf("pop3 client thread terminal\n");

}
