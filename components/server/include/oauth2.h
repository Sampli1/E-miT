#ifndef OAUTH2_H
#define OAUTH2_H

#include "client.h"
#include "jsmn.h"

void token_management(char *code, char *scope);

#endif // OAUTH2_H