#pragma once

// Position-independent code reference
// Function that align the dereferenced value in a rom struct to use it depending on the execution
// address. Can be used even if code is executing at the same place where it had been linked.
#ifndef PIC
#if defined(OS_UNIT_TEST) || defined(FUZZING)
#define PIC(x) (x)
#ifndef OS_UNIT_TEST
// Identity, but a real function: callers passing `pic` as a callback need an
// address, which a macro cannot provide.
static inline void *pic(void *linked_address)
{
    return linked_address;
}
#endif
#elif defined(HAVE_BOLOS) && !defined(BOLOS_OS_UPGRADER_APP)
#define PIC(x) pic_shared((const void *) x)
void *pic_shared(const void *linked_address);
#else
#define PIC(x) pic((void *) x)
void *pic(void *linked_address);
#endif
#endif  // PIC
void pic_init(void *pic_flash_start, void *pic_ram_start);
