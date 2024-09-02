#pragma once

// Position-independent code reference
// Function that align the dereferenced value in a rom struct to use it depending on the execution
// address. Can be used even if code is executing at the same place where it had been linked.
#if defined(HAVE_BOLOS) && !defined(BOLOS_OS_UPGRADER_APP)
#define PIC(x) (x)
#endif
#ifndef PIC
#define PIC(x) pic((void *) x)
void *pic(void *linked_address);
#endif
void pic_init(void *pic_flash_start, void *pic_ram_start);
