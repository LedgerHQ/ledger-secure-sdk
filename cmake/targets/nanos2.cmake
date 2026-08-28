# Ported from the nanos2 block of Makefile.defines, USE_NBGL=1 branch: BAGL is
# legacy and the CMake build does not carry it. The BAGL_* font descriptors stay,
# Makefile.defines keeps them for both UI flavours.
set(LEDGER_TARGET_DEFS
    BAGL_HEIGHT=64
    BAGL_WIDTH=128
    HAVE_BAGL_ELLIPSIS
    HAVE_BAGL_FONT_OPEN_SANS_EXTRABOLD_11PX
    HAVE_BAGL_FONT_OPEN_SANS_LIGHT_16PX
    HAVE_BAGL_FONT_OPEN_SANS_REGULAR_11PX
    HAVE_BATTERY
    HAVE_FONTS
    HAVE_INAPP_BLE_PAIRING
    HAVE_NBGL
    HAVE_SE_BUTTON
    HAVE_SE_SCREEN
    NBGL_STEP
    NBGL_USE_CASE
    SCREEN_SIZE_NANO
    TARGET="nanos2"
    TARGET_NAME="TARGET_NANOS2"
)
