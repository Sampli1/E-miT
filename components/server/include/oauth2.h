#ifndef OAUTH2_H
#define OAUTH2_H

#include "client.h"
#include "utils.h"

void token_management(char *code, char *scope, char *id);

int refresh_token_managment(int id, char *response);

#endif // OAUTH2_H