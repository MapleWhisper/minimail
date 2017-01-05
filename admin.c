//
// Created by maple on 17-1-5.
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "database.h"
#include "server.h"
#include "admin.h"

void admin_handler(FILE *client, void *arg) {
    admin(client, (const char *) arg);
}

#define RESPOND(client, code, message)                  \
    printf("ADMIN response: %d %s\r\n", code, message);   \
    fprintf(client, "%d %s\r\n", code, message);            \





void admin(FILE *client, const char *dbfile) {
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
    struct recipient *recipients = NULL;
    while (!feof(client)) {
        char line[512];
        fgets(line, sizeof(line), client);
        if (DEBUG) {
            printf("SMTP request : %s\n", line);
        }
        char command[5] = {line[0], line[1], line[2], line[3]};
        strupr(command, sizeof(command));
        if (strcmp(command, "ADD") == 0) {
            // ADD username password
            // 明文存储
            char *user_line = line+3;
            UserInfo *user_info = malloc(sizeof(user_info));

            char *parts[128];
            int  size = strsplit(user_line, parts, " ");
            if(size!=2){
                RESPOND(client, 500, "Syntax error ; usage: ADD username password");
                continue;
            }
            if(DEBUG){
                for (int i =0; i < size; ++i) {
                    printf("%s\n", parts[i]);
                }
            }

            strcpy( user_info->username, parts[0]);
            strcpy( user_info->password, parts[1]);
            database_save_user(&db, user_info);

            free(user_info);
            RESPOND(client, 250, "add user success");

        }  else if (strcmp(command, "QUIT") == 0) {
            RESPOND(client, 221, "Bye");
            break;

        } else {
            RESPOND(client, 500, "command unrecognized");
        }
    }
    fclose(client);
    database_close(&db);
}
