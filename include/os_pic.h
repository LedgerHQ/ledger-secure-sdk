#pragma once

// Position-independent code reference
// Function that align the dereferenced value in a rom struct to use it depending on the execution
// address. Can be used even if code is executing at the same place where it had been linked.
#if defined(HAVE_BOLOS) && !defined(BOLOS_OS_UPGRADER_APP)
#define PIC(x) shared_pic((void *) x)
void *shared_pic(void *linked_address);
#endif
#ifndef PIC
#define PIC(x) pic((void *) x)
void *pic(void *linked_address);
void *get_flash_reloc_addr(void);
void *get_flash_reloc_end(void);
void *get_ram_reloc_addr(void);
void *get_ram_reloc_end(void);
#endif
void init_pic(void *flash_start,
              void *flash_end,
              void *ram_start,
              void *ram_end,
              void *pic_flash_start,
              void *pic_ram_start);
