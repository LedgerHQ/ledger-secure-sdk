/*******************************************************************************
*   Ledger - Secure firmware
*   (c) 2016, 2017, 2018, 2019, 2020, 2021, 2022, 2023 Ledger
*
*  Licensed under the Apache License, Version 2.0 (the "License");
*  you may not use this file except in compliance with the License.
*  You may obtain a copy of the License at
*
*      http://www.apache.org/licenses/LICENSE-2.0
*
*  Unless required by applicable law or agreed to in writing, software
*  distributed under the License is distributed on an "AS IS" BASIS,
*  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*  See the License for the specific language governing permissions and
*  limitations under the License.
********************************************************************************/

/**
 * Global chip memory layout and constants
 *
 */

MEMORY
{
  DISCARD (rwx) : ORIGIN = 0xd0000000, LENGTH = 1M

  FLASH   (rx)  : ORIGIN = 0xc0de0000, LENGTH = 400K
  DATA    (r)   : ORIGIN = 0xc0de0000, LENGTH = 400K
  SRAM    (rwx) : ORIGIN = 0xda7a0000, LENGTH = 40K
}

PAGE_SIZE  = 512;
STACK_MIN_SIZE = DEFINED(stack_min_size) ? stack_min_size : 1500;
END_STACK  = ORIGIN(SRAM) + LENGTH(SRAM);

ENTRY(main);

SECTIONS
{
  /****************************************************************/
  /* This section locates the code in FLASH                       */
  /****************************************************************/

  /** put text in Flash memory, VMA will be equal to LMA */
  .text :
  {
    /* provide start code symbol, shall be zero */
    _text = .;
    _nvram_start = .;

    /* ensure main is always @ 0xC0D00000 */
    *(.boot*)

    /* place the other code and rodata defined BUT nvram variables that are displaced in a r/w area */
    _code = .;
    *(.text*)
    _ecode = .;
    *(.rodata)
    *(.rodata.[^N]*) /*.data.rel.ro* not here to detect invalid PIC usage */
    *(.rodata.N[^_]*)

    . = ALIGN(4);

    /* all code placed */
    _etext = .;

    . = ALIGN(PAGE_SIZE);

    _nvram_data = .;

    /* App storage */
    *(.storage_section)

    /* NVM data (ex-filesystem) */
    *(.bss.N_* .rodata.N_*)

    . = ALIGN(PAGE_SIZE);
    _envram_data = .;

    _install_parameters = .;
    *(.install_parameters)
    KEEP(*(.install_parameters))
    _einstall_parameters = .;
    _nvram_end = .;
  } > FLASH = 0x00

  .data (NOLOAD):
  {
    . = ALIGN(4);

    /**
     * Place RAM initialized variables
     */
    _data = .;

    *(vtable)
    *(.data*)

    _edata = .;

  } > DISCARD /*> SRAM AT>FLASH = 0x00 */

  ASSERT( (_edata - _data) <= 0, ".data section must be empty" )

  .bss :
  {
    /**
     * Place RAM uninitialized variables
     */
    _bss = .;
    *(.bss*)
    _ebss = .;


    /**
     * Reserve stack size
     */
    . = ALIGN(4);
    app_stack_canary = .;
    PROVIDE(app_stack_canary = .);
    . += 4;
    _stack = .;
    PROVIDE( _stack = .);
    _estack = ABSOLUTE(END_STACK);
    PROVIDE( _estack = ABSOLUTE(END_STACK) );

  } > SRAM = 0x00

  ASSERT( (_estack - _stack) >= STACK_MIN_SIZE, "stack section too small" )

  /****************************************************************/
  /* DEBUG                                                        */
  /****************************************************************/

  /* remove the debugging information from the standard libraries */
  DEBUG (NOLOAD) :
  {
    libc.a ( * )
    libm.a ( * )
    libgcc.a ( * )
    *(.ARM.exidx* .gnu.linkonce.armexidx.*)
  } > DISCARD

  /* Stabs debugging sections.  */
  .stab          0 : { *(.stab) }
  .stabstr       0 : { *(.stabstr) }
  .stab.excl     0 : { *(.stab.excl) }
  .stab.exclstr  0 : { *(.stab.exclstr) }
  .stab.index    0 : { *(.stab.index) }
  .stab.indexstr 0 : { *(.stab.indexstr) }
  .comment       0 : { *(.comment) }
  /* DWARF debug sections.
      Symbols in the DWARF debugging sections are relative to the beginning
      of the section so we begin them at 0.  */
  /* DWARF 1 */
  .debug          0 : { *(.debug) }
  .line           0 : { *(.line) }
  /* GNU DWARF 1 extensions */
  .debug_srcinfo  0 : { *(.debug_srcinfo) }
  .debug_sfnames  0 : { *(.debug_sfnames) }
  /* DWARF 1.1 and DWARF 2 */
  .debug_aranges  0 : { *(.debug_aranges) }
  .debug_pubnames 0 : { *(.debug_pubnames) }
  /* DWARF 2 */
  .debug_info     0 : { *(.debug_info .gnu.linkonce.wi.*) }
  .debug_abbrev   0 : { *(.debug_abbrev) }
  .debug_line     0 : { *(.debug_line) }
  .debug_frame    0 : { *(.debug_frame) }
  .debug_str      0 : { *(.debug_str) }
  .debug_loc      0 : { *(.debug_loc) }
  .debug_macinfo  0 : { *(.debug_macinfo) }
  /* SGI/MIPS DWARF 2 extensions */
  .debug_weaknames 0 : { *(.debug_weaknames) }
  .debug_funcnames 0 : { *(.debug_funcnames) }
  .debug_typenames 0 : { *(.debug_typenames) }
  .debug_varnames  0 : { *(.debug_varnames) }

  ledger.target (INFO): { KEEP(*(ledger.target)) }
  ledger.target_name (INFO): { KEEP(*(ledger.target_name)) }
  ledger.target_id (INFO): { KEEP(*(ledger.target_id)) }
  ledger.app_name (INFO): { KEEP(*(ledger.app_name)) }
  ledger.app_version (INFO): { KEEP(*(ledger.app_version)) }
  ledger.api_level (INFO): { KEEP(*(ledger.api_level)) }
  ledger.sdk_name (INFO): { KEEP(*(ledger.sdk_name)) }
  ledger.sdk_version (INFO): { KEEP(*(ledger.sdk_version)) }
  ledger.sdk_hash (INFO): { KEEP(*(ledger.sdk_hash)) }
  ledger.sdk_graphics (INFO): { KEEP(*(ledger.sdk_graphics)) }
}

PROVIDE(_nvram = ABSOLUTE(_nvram_start));
PROVIDE(_envram = ABSOLUTE(_nvram_end));
