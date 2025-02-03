/**
 * @file properties.c
 * @brief read properties from a file.properties file, in JSON format
 *
 */

/*********************
 *      INCLUDES
 *********************/
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "properties.h"
// You may have to install packet libjson-c-dev
#include <json-c/json.h>

static char *buffer = NULL;

void properties_init(char *filename)
{
    FILE  *json_file;
    size_t size;

    if ((json_file = fopen(filename, "r")) == NULL) {
        fprintf(stdout, "Unable to read properties JSON file %s!\n", filename);
        return;
    }

    // Determine file size
    if (fseek(json_file, 0, SEEK_END) != 0) {
        fprintf(stdout, "Error seeking to the end of JSON file %s!\n", filename);
        fclose(json_file);
        return;
    }

    if ((size = ftell(json_file)) == 0) {
        fprintf(stdout, "Error getting size of JSON file %s!\n", filename);
        fclose(json_file);
        return;
    }

    if (fseek(json_file, 0, SEEK_SET) != 0) {
        fprintf(stdout, "Error seeking to the start of JSON file %s!\n", filename);
        fclose(json_file);
        return;
    }

    // Allocate memory and read the file
    if ((buffer = malloc(size + 1)) == NULL) {
        fprintf(stdout, "Error allocating %lu bytes to read JSON file %s!\n", size, filename);
        fclose(json_file);
        return;
    }
    if (fread(buffer, 1, size, json_file) != size) {
        fprintf(stdout, "Error reading %lu bytes from JSON file %s!\n", size, filename);
        free(buffer);
        fclose(json_file);
        return;
    }
    buffer[size] = 0;
    fclose(json_file);
}

static bool search_string_property(char *prop_name, char **val)
{
    struct json_object *json_root;

    if ((json_root = json_tokener_parse(buffer)) == NULL) {
        fprintf(stdout, "An error occurred while parsing properties JSON file!\n");
        return false;
    }

    if (json_type_object != json_object_get_type(json_root)) {
        fprintf(stdout, "There is no valid entry in file!\n");
        json_object_put(json_root);
        return false;
    }
    // Next line is a macro that iterate through keys/values of that JSON object
    json_object_object_foreach(json_root, key, value)
    {
        if (!strcmp(key, prop_name)) {
            *val = (char *) json_object_get_string(value);
            return true;
        }
    }
    json_object_put(json_root);
    return false;
}

static bool search_int_property(char *prop_name, long *val)
{
    struct json_object *json_root;

    if ((json_root = json_tokener_parse(buffer)) == NULL) {
        fprintf(stdout, "An error occurred while parsing properties JSON file!\n");
        return false;
    }

    if (json_type_object != json_object_get_type(json_root)) {
        fprintf(stdout, "There is no valid entry in file!\n");
        json_object_put(json_root);
        return false;
    }
    // Next line is a macro that iterate through keys/values of that JSON object
    json_object_object_foreach(json_root, key, value)
    {
        if (!strcmp(key, prop_name)) {
            *val = json_object_get_int(value);
            return true;
        }
    }
    json_object_put(json_root);
    return false;
}

long properties_read_int(char *name, long defaultValue)
{
    long val;
    bool found;

    // search property name
    found = search_int_property(name, &val);
    if (!found) {
        return defaultValue;
    }
    return val;
}

char *properties_read_string(char *name, char *defaultValue)
{
    bool  found;
    char *ret, *val;

    // search property name
    found = search_string_property(name, &val);
    if (found == false) {
        return defaultValue;
    }
    ret = (char *) malloc(strlen(val) + 1);
    memcpy(ret, val, strlen(val) + 1);

    return ret;
}

bool properties_read_bool(char *name, bool defaultValue)
{
    bool                ret;
    struct json_object *json_root;

    if ((json_root = json_tokener_parse(buffer)) == NULL) {
        fprintf(stdout, "An error occurred while parsing properties JSON file!\n");
        return NULL;
    }

    if (json_type_object != json_object_get_type(json_root)) {
        fprintf(stdout, "There is no valid entry in file!\n");
        json_object_put(json_root);
        return NULL;
    }
    // Next line is a macro that iterate through keys/values of that JSON object
    json_object_object_foreach(json_root, key, value)
    {
        if (!strcmp(key, name)) {
            ret = (json_object_get_boolean(value) != 0);
            return ret;
        }
    }
    json_object_put(json_root);

    return defaultValue;
}
