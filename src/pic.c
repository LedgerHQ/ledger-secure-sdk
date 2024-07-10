#include "bolos_target.h"
#include "os_pic.h"

#if !defined(HAVE_BOLOS) || defined(BOLOS_OS_UPGRADER_APP)
#pragma GCC diagnostic                                      push
#pragma GCC diagnostic                                      ignored "-Wunused-parameter"
__attribute__((naked, no_instrument_function)) static void *pic_internal(void *link_address)
{
    // compute the delta offset between LinkMemAddr & ExecMemAddr
    __asm volatile("mov r2, pc\n");
    __asm volatile("ldr r1, =pic_internal\n");
    __asm volatile("adds r1, r1, #3\n");
    __asm volatile("subs r1, r1, r2\n");

    // adjust value of the given parameter
    __asm volatile("subs r0, r0, r1\n");
    __asm volatile("bx lr\n");
}
#pragma GCC diagnostic pop

// only apply PIC conversion if link_address is in linked code (over 0xC0D00000 in our example)
// this way, PIC call are armless if the address is not meant to be converted
extern void _nvram;
extern void _envram;
#endif  // !defined(HAVE_BOLOS) || defined(BOLOS_OS_UPGRADER_APP)

#if defined(ST31)

void *pic(void *link_address)
{
    // check if in the LINKED TEXT zone
    if (link_address >= &_nvram && link_address < &_envram) {
        link_address = pic_internal(link_address);
    }

    return link_address;
}

#elif defined(ST33) || defined(ST33K1M5)

#if !defined(HAVE_BOLOS) || defined(BOLOS_OS_UPGRADER_APP)
extern void _bss;
extern void _estack;

void *pic(void *link_address)
{
    void *n, *en;

    // check if in the LINKED TEXT zone
    __asm volatile("ldr %0, =_nvram" : "=r"(n));
    __asm volatile("ldr %0, =_envram" : "=r"(en));
    if (link_address >= n && link_address <= en) {
        link_address = pic_internal(link_address);
    }

    // check if in the LINKED RAM zone
    __asm volatile("ldr %0, =_bss" : "=r"(n));
    __asm volatile("ldr %0, =_estack" : "=r"(en));
    if (link_address >= n && link_address <= en) {
        __asm volatile("mov %0, r9" : "=r"(en));
        // deref into the RAM therefore add the RAM offset from R9
        link_address = (char *) link_address - (char *) n + (char *) en;
    }

    return link_address;
}

#else  // !defined(HAVE_BOLOS) || defined(BOLOS_OS_UPGRADER_APP)

void *flash_real_addr;
void *ram_real_addr;
/* base and end of relocated flash for the app (usually 0xC0DE0000)*/
void *flash_reloc_addr;
void *flash_reloc_end;
/* base and end of relocated ram for the app (usually 0xDA7A0000)*/
void *ram_reloc_addr;
void *ram_reloc_end;

void *shared_pic(void *link_address)
{
    // check if in the LINKED TEXT zone
    if ((link_address > flash_reloc_addr) && (link_address <= flash_reloc_end)) {
        link_address = link_address - flash_reloc_addr + flash_real_addr;
    }

    // check if in the LINKED RAM zone
    if ((link_address >= ram_reloc_addr) && (link_address <= ram_reloc_end)) {
        link_address = link_address - ram_reloc_addr + ram_real_addr;
    }

    return link_address;
}

void init_pic(void *flash_start,
              void *flash_end,
              void *ram_start,
              void *ram_end,
              void *pic_flash_start,
              void *pic_ram_start)
{
    flash_real_addr  = pic_flash_start;
    ram_real_addr    = pic_ram_start;
    flash_reloc_addr = flash_start;
    flash_reloc_end  = flash_end;
    ram_reloc_addr   = ram_start;
    ram_reloc_end    = ram_end;
}

#endif  // !defined(HAVE_BOLOS) || defined(BOLOS_OS_UPGRADER_APP)

inline void *get_flash_reloc_addr(void)
{
    void *n;
    __asm volatile("ldr %0, =_nvram" : "=r"(n));
    return n;
}
inline void *get_flash_reloc_end(void)
{
    void *n;
    __asm volatile("ldr %0, =_envram" : "=r"(n));
    return n;
}
inline void *get_ram_reloc_addr(void)
{
    void *n;
    __asm volatile("ldr %0, =_bss" : "=r"(n));
    return n;
}
inline void *get_ram_reloc_end(void)
{
    void *n;
    __asm volatile("ldr %0, =_estack" : "=r"(n));
    return n;
}
#else

#error "invalid architecture"

#endif
