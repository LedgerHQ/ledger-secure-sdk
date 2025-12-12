#pragma once

#include <stdint.h>

// Arbitrary max size for application name and version.
#define BOLOS_APPNAME_MAX_SIZE_B    32
#define BOLOS_APPVERSION_MAX_SIZE_B 16

/* ----------------------------------------------------------------------- */
/* -                       APPLICATION FUNCTIONS                         - */
/* ----------------------------------------------------------------------- */

#define BOLOS_TAG_APPNAME    0x01
#define BOLOS_TAG_APPVERSION 0x02
#define BOLOS_TAG_ICON       0x03
#define BOLOS_TAG_DERIVEPATH 0x04
#define BOLOS_TAG_DATA_SIZE \
    0x05  // meta tag to retrieve the size of the data section for the application
// Library Dependencies are a tuple of 2 LV, the lib appname and the lib version (only exact for
// now). When lib version is not specified, it is not check, only name is asserted The DEPENDENCY
// tag may have several occurrences, one for each dependency (by name). Malformed (multiple dep to
// the same lib with different version is is not ORed but ANDed, and then considered bogus)
#define BOLOS_TAG_DEPENDENCY 0x06
#define BOLOS_TAG_RFU        0x07
// first autorised tag value for user data
#define BOLOS_TAG_USER_TAG   0x20
