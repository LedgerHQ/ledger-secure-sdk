#pragma once

/**
 * Macro for the size of a specific structure field.
 */
#define MEMBER_SIZE(type, member) (sizeof(((type *) 0)->member))

/**
 * Marks a function or variable as a weak symbol, allowing application code to
 * override the default SDK implementation by providing a strong definition.
 */
#define WEAK __attribute((weak))

/**
 * Marks a function as non-returning (e.g. loops forever or calls exit).
 * Must be placed before the return type in function definitions.
 */
#define NORETURN __attribute__((noreturn))

/**
 * Places a function in the @c .boot linker section.
 * Must be placed before the return type in function definitions.
 */
#define BOOT_SECTION __attribute__((section(".boot")))

/**
 * Returns the number of elements in a statically allocated array.
 * Must not be used on a pointer.
 */
#define ARRAY_LENGTH(array) (sizeof((array)) / sizeof((array)[0]))
