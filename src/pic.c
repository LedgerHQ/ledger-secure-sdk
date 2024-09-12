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

/* physical base addresses of flash & RAM for the running app*/
static void *flash_real_addr;
static void *ram_real_addr;

// TODO: these 4constants should be extracted from linker script
// (public_sdk/target/<product>/script.ld)
/* base and end of relocated flash for the running app (usually 0xC0DE0000)*/
#define APP_FLASH_RELOC_ADDR (void *) 0xC0DE0000
#define APP_FLASH_RELOC_END  (void *) (APP_FLASH_RELOC_ADDR + 400 * 1024)
/* base and end of relocated ram for the running app (usually 0xDA7A0000)*/
#define APP_DATA_RELOC_ADDR  (void *) 0xDA7A0000
#define APP_DATA_RELOC_END   (void *) (APP_DATA_RELOC_ADDR + 40 * 1024)

/**
 * @brief PIC function to be used in UX, to retrieve the physical address from the
 * potentially relocated given address.
 * @note This function only works if @ref pic_init() is called at Application start-up
 * to initiate the relocated Flash and RAM limits and the corresponding physical Flash & RAM
 * addresses
 *
 * @param link_address potentially relocated address
 * @return the physical address
 */
void *pic_shared(const void *link_address)
{
    // check if in the LINKED TEXT zone
    if ((link_address >= APP_FLASH_RELOC_ADDR) && (link_address < APP_FLASH_RELOC_END)) {
        link_address = link_address - APP_FLASH_RELOC_ADDR + flash_real_addr;
    }

    // check if in the LINKED RAM zone
    if ((link_address >= APP_DATA_RELOC_ADDR) && (link_address < APP_DATA_RELOC_END)) {
        link_address = link_address - APP_DATA_RELOC_ADDR + ram_real_addr;
    }

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdiscarded-qualifiers"
    return link_address;
#pragma GCC diagnostic pop
}

/**
 * @brief Function to be called just before Application start-up to initiate the relocated Flash and
 * RAM limits and the corresponding physical Flash & RAM addresses
 *
 * @param real_flash_start physical base address for App code (= PIC(flash_start))
 * @param real_ram_start physical base address for App data (= PIC(ram_start))
 */
void pic_init(void *real_flash_start, void *real_ram_start)
{
    flash_real_addr = real_flash_start;
    ram_real_addr   = real_ram_start;
}

#endif  // !defined(HAVE_BOLOS) || defined(BOLOS_OS_UPGRADER_APP)

#else

#error "invalid architecture"

#endif
