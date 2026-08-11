#######################################################################
#                      Ledger compile options                         #
#######################################################################
option(LEDGER_DEVICE_BUILD "Build for the device: link options requiring lld and the SDK link script" OFF)
option(ENABLE_SDK_WERROR "Turn SDK warnings into errors" OFF)
option(ENABLE_STACK_PROTECTOR "Build with -fstack-protector-strong" OFF)
option(ENABLE_LINK_TIME_OPTIMIZATION "Build with LTO" OFF)

add_library(ledger_compile_options INTERFACE)
add_library(ledger::compile-options ALIAS ledger_compile_options)

target_compile_options(ledger_compile_options INTERFACE
    $<$<COMPILE_LANGUAGE:ASM>:-Wno-unused-command-line-argument>
)

target_compile_options(ledger_compile_options INTERFACE
    $<$<COMPILE_LANGUAGE:C>:-fdata-sections>
    $<$<COMPILE_LANGUAGE:C>:-ffunction-sections>
    $<$<COMPILE_LANGUAGE:C>:-fno-common>
    $<$<COMPILE_LANGUAGE:C>:-fomit-frame-pointer>
    $<$<COMPILE_LANGUAGE:C>:-momit-leaf-frame-pointer>
    $<$<COMPILE_LANGUAGE:C>:-fshort-enums>
    $<$<COMPILE_LANGUAGE:C>:-funsigned-char>
    # jump tables would emit invalid PIC accesses
    $<$<COMPILE_LANGUAGE:C>:-fno-jump-tables>
)

target_compile_options(ledger_compile_options INTERFACE
    $<$<COMPILE_LANGUAGE:C>:-Wall>
    $<$<COMPILE_LANGUAGE:C>:-Wextra>
    $<$<COMPILE_LANGUAGE:C>:-Wno-main>
    $<$<COMPILE_LANGUAGE:C>:-Werror=int-to-pointer-cast>
    $<$<COMPILE_LANGUAGE:C>:-Wno-implicit-function-declaration>
    $<$<COMPILE_LANGUAGE:C>:-Wno-error=int-conversion>
    $<$<COMPILE_LANGUAGE:C>:-Wimplicit-fallthrough>
    $<$<COMPILE_LANGUAGE:C>:-Wvla>
    $<$<COMPILE_LANGUAGE:C>:-Wundef>
    $<$<COMPILE_LANGUAGE:C>:-Wshadow>
    $<$<COMPILE_LANGUAGE:C>:-Wformat=2>
    $<$<COMPILE_LANGUAGE:C>:-Wformat-security>
    $<$<COMPILE_LANGUAGE:C>:-Wwrite-strings>
)

if(ENABLE_SDK_WERROR)
    target_compile_options(ledger_compile_options INTERFACE
        $<$<COMPILE_LANGUAGE:C>:-Werror>
        "$<$<COMPILE_LANGUAGE:C>:-Werror=#pragma-messages>")
endif()

if(LEDGER_DEVICE_BUILD)
    if(ENABLE_STACK_PROTECTOR)
        target_compile_options(ledger_compile_options INTERFACE -fstack-protector-strong)
        # Entry at __stack_chk_init so the canary is randomized, including in sideloads.
        target_link_options(ledger_compile_options INTERFACE
            -Wl,--wrap=__stack_chk_fail -Wl,--wrap=__stack_chk_init
            -Wl,--defsym,__entry=__stack_chk_init)
    else()
        target_link_options(ledger_compile_options INTERFACE -Wl,--defsym,__entry=main)
    endif()

    if(ENABLE_LINK_TIME_OPTIMIZATION)
        target_compile_options(ledger_compile_options INTERFACE $<$<COMPILE_LANGUAGE:C>:-flto>)
        target_compile_definitions(ledger_compile_options INTERFACE HAS_LTO)
        target_link_options(ledger_compile_options INTERFACE -flto -Wl,-mllvm,-relocation-model=ropi-rwpi)
    endif()

    target_link_options(ledger_compile_options INTERFACE
        -Wall -fno-common -ffunction-sections -fdata-sections -fwhole-program -Wl,--gc-sections)

    # Link script and arch libraries, as Makefile.rules_generic does.
    target_link_options(ledger_compile_options INTERFACE
        -T${LEDGER_SDK_ROOT}/target/${LEDGER_TARGET}/script.ld
        -L${LEDGER_SDK_ROOT}/target/${LEDGER_TARGET})
    target_link_libraries(ledger_compile_options INTERFACE clang_rt.builtins m c)
endif()
