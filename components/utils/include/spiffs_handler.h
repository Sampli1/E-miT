#ifndef SPIFFS_H
#define SPIFFS_H

void init_spiffs();

char* read_from_spiffs(char *filename);

int write_on_spiffs(char *filename, char *content);

#endif // SPIFFS_H