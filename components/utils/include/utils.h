#ifndef UTILS_H
#define UTILS_H

#include "jsmn.h"

void decompose_json_dynamic_params(char *json, int num_params, ...);

uint32_t example_uri_encode(char *dest, const char *src, size_t len);

void example_uri_decode(char *dest, const char *src, size_t len);

void from_string_to_json_string_vec(char *input, char *jsonArray[20], int*length);

void from_string_to_int_array(char *input, int *array, int *size);


#endif // UTILS_H
