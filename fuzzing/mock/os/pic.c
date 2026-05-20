/* PIC address translation mocks.
 * Fuzz builds run at linked addresses, so PIC helpers are identity functions.
 */

#include "os_pic.h"

void *pic(void *linked_address)
{
    return linked_address;
}

void *pic_shared(const void *linked_address)
{
    return (void *) linked_address;
}

void pic_init(void *pic_flash_start, void *pic_ram_start)
{
    (void) pic_flash_start;
    (void) pic_ram_start;
}
