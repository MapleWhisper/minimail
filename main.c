#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <getopt.h>

#include "server.h"
#include "database.h"
#include "smtp.h"
#include "pop3.h"
#include "admin.h"

#define ERROR(format, args...)                                  \
    do {                                                        \
        fprintf(stderr, "%s: " format "\n", argv[0], args);     \
        exit(EXIT_FAILURE);                                     \
    } while (0)

int main(int argc, char **argv)
{


    /* Option parsing */
    static struct option options[] = {
        {"smtp-port", required_argument, 0, 's'},
        {"pop3-port", required_argument, 0, 'p'},
        {"database",  required_argument, 0, 'd'},
        {0}
    };
    int option;
    while ((option = getopt_long(argc, argv, "", options, NULL)) != -1) {
        switch (option) {
            /*×
        case 's':
            SMTP_PORT = atoi(optarg);
            break;
        case 'p':
            POP3_PORT = atoi(optarg);
            break;
             */
        case 'd':
            DBFILE = optarg;
            break;
        default:
            exit(EXIT_FAILURE);
        }
    }

    /* Ensure database works */
    struct database db;
    if (database_open(&db, DBFILE) != 0)
        ERROR("failed to open database, %s", DBFILE);
    database_close(&db);

    /* Launch servers. */
    struct server smtp = {
        .port = SMTP_PORT,
        .handler = smtp_handler,
        .arg = DBFILE
    };
    struct server pop3 = {
        .port = POP3_PORT,
        .handler = pop3_handler,
        .arg = DBFILE
    };
    struct server admin = {
        .port = ADMIN_PORT,
        .handler = admin_handler,
        .arg = DBFILE
    };
    if (server_start(&smtp) != 0)
        ERROR("%s", "failed to start SMTP server");
    if (server_start(&pop3) != 0)
        ERROR("%s", "failed to start POP3 server");
    if (server_start(&admin) != 0)
        ERROR("%s", "failed to start ADMIN server");
    pthread_join(smtp.thread, NULL);
    pthread_join(pop3.thread, NULL);
    pthread_join(admin.thread, NULL);
    return 0;
}
