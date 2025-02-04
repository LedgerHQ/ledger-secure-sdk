/**
 * @file properties.h
 *
 */

#ifndef PROPERTIES_H
#define PROPERTIES_H

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

void  properties_init(char *filename);
long  properties_read_int(char *name, long defaultValue);
char *properties_read_string(char *name, char *defaultValue);
bool  properties_read_bool(char *name, bool defaultValue);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* PROPERTIES_H */
