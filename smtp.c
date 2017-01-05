#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "database.h"
#include "smtp.h"
#include "server.h"
#include "base64.h"


void smtp_handler(FILE *client, void *arg)
{
    smtp(client, (const char *) arg);
}

#define RESPOND(client, code, message)                  \
    printf("SMTP response: %d %s\r\n", code, message);   \
    fprintf(client, "%d %s\r\n", code, message);            \



void smtp(FILE *client, const char *dbfile)
{
    int auth_status = 0;
    struct database db;
    if (database_open(&db, dbfile) != 0) {
        RESPOND(client, 421, "database error");
        fclose(client);
        return;
    } else {
        RESPOND(client, 220, "localhost");
    }

    char from[512] = {0};
    UserInfo *userInfo = NULL;
    struct recipient *recipients = NULL;
    int user_auth_pass = 0;
    while (!feof(client)) {
        char line[512];
        fgets(line, sizeof(line), client);

        if(DEBUG){
            printf("SMTP request : %s\n",line);
        }
        char command[5] = {line[0], line[1], line[2], line[3]};
        strupr(command , sizeof(command));
        if (strcmp(command, "HELO") == 0) {

            RESPOND(client, 250, "localhost");

        }else if(strcmp(command, "EHLO")==0){

            fprintf(client, "%d%s\r\n", 250, "-mail");
            fprintf(client, "%d%s\r\n", 250, "-AUTH LOGIN PLAIN");
            fprintf(client, "%d%s\r\n", 250, "-AUTH=LOGIN PLAIN");
            fprintf(client, "%d%s\r\n", 250, "-coremail 1Uxr2xKj7kG0xkI17xGrU7I0s8FY2U3Uj8Cz28x1UUUUU7Ic2I0Y2Ur-HQ1KUCa0xDrUUUUj");
            fprintf(client, "%d%s\r\n", 250, "-STARTTLS");
            fprintf(client, "%d%s\r\n", 250, " 8BITMIME");
        }
        else if (strcmp(command, "MAIL") == 0) {
            if(!user_auth_pass){
                RESPOND(client, 500 ,"auth first");
                continue;
            }
            strcpy(from, line);

            RESPOND(client, 250, "OK");
        } else if(strcmp(command,"AUTH")==0){
            remove_line_break(line);
            if(strcmp(line+5,"LOGIN")==0){
                // auth login
                //进入 验证状态
                //输入用户名
                auth_status = 1;
                RESPOND(client,334,"VXNlcm5hbWU6");
            }else{
                // auth plain
                RESPOND(client,235,"Authentication successful");
            }
        } else if (auth_status){
            remove_line_break(line);
            if(auth_status == 1){
                //判断用户
                char user[128] ={0};
                base64_decode(line,user);
                get_word(user, user);
                if(strlen(user)<=1){
                    RESPOND(client,535,"user not found");
                    continue;
                }
                userInfo = database_get_user(&db ,user);
                if(userInfo==NULL){
                    RESPOND(client,535,"user not found");
                    continue;
                }
                //发送密码
                RESPOND(client,334,"UGFzc3dvcmQ6");
                auth_status = 2;

            }else if(auth_status == 2){
                if(userInfo==NULL){
                    auth_status = 0;
                    RESPOND(client,535,"authentication failed");
                    continue;
                }
                char pwd[128] = {0};
                base64_decode(line,pwd);
                if(strcmp(userInfo->password , pwd)==0){
                    //验证成功
                    RESPOND(client,235,"Authentication successful");
                    user_auth_pass = 1;
                    auth_status = 0;
                }else{
                    RESPOND(client,535,"authentication failed , password invalid");
                }
            }
        } else if (strcmp(command, "RCPT") == 0) {
            if(!user_auth_pass){
                RESPOND(client, 500 ,"auth first");
                continue;
            }
            if (!from[0]) {
                RESPOND(client, 503, "bad sequence");
            } else if (strlen(line) < 12) {
                RESPOND(client, 501, "syntax error");
            } else {
                UserInfo *rcpt_user = parserUseInfo(line);
                if(DEBUG){
                    printf("username=%s  domain=%s\n" , rcpt_user->username , rcpt_user->domain);
                }
//                //非本机域名
//                if(strcmp(rcpt_user->domain,SERVER_DOMAIN)!=0){
//                    char msg[256];
//                    snprintf(msg, 256 , "domain :'%s' not support ;\n server domain is : '%s'" ,rcpt_user->domain , SERVER_DOMAIN);
//                    RESPOND(client, 500, msg);
//                    continue;
//                }
                recipient_push(&recipients, line + 9);
                RESPOND(client, 250, "OK");
            }

        } else if (strcmp(command, "DATA") == 0) {
            if(!user_auth_pass){
                RESPOND(client, 500 ,"auth first");
                continue;
            }
            RESPOND(client, 354, "end with .");
            size_t size = 4096, fill = 0;
            char *content = malloc(size);
            for (;;) {
                fgets(line, sizeof(line), client);
                if (strcmp(line, ".\r\n") == 0)
                    break;
                if (strlen(line) + fill >= size) {
                    size *= 2;
                    content = realloc(content, size);
                }
                strcpy(content + fill, line);
                fill += strlen(line);
            }
            while (recipients) {
                struct recipient *r = recipient_pop(&recipients);
                database_send(&db, r->email, content);
                free(r);
            }
            free(content);
            RESPOND(client, 250, "OK");

        } else if (strcmp(command, "QUIT") == 0) {
            RESPOND(client, 221, "Bye localhost");
            break;

        } else {
            RESPOND(client, 500, "command unrecognized");
        }
    }
    fclose(client);
    database_close(&db);
}
