/* Fuzzing wrapper for <string.h>: adds strlcpy/strlcat prototypes missing from C99. */
#include_next <string.h>

size_t strlcpy(char *dst, const char *src, size_t size);
size_t strlcat(char *dst, const char *src, size_t size);
