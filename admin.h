//
// Created by maple on 17-1-5.
//

#ifndef MINIMAIL_ADMIN_H
#define MINIMAIL_ADMIN_H


#include <stdio.h>
#include "database.h"

void admin(FILE *client, const char *dbfile);
void admin_handler(FILE *client, void *arg);

#endif //MINIMAIL_ADMIN_H
