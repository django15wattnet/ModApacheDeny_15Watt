/**
 * Various string functions.
 * Created by Thomas Siemion on 26.12.25.
 */
#ifndef MODAPACHEDENY_15WATT_FUNCTIONSSTRING_H
#define MODAPACHEDENY_15WATT_FUNCTIONSSTRING_H

#include <stdio.h>
#include <string.h>
#include <stdbool.h>

bool stringEndsWith(const char *str, const char *suffix);
bool stringStartsWith(const char *str, const char *prefix);
void stringDeleteCharRight(char *str);
void stringDeleteCharLeft(char *str);

#endif //MODAPACHEDENY_15WATT_FUNCTIONSSTRING_H