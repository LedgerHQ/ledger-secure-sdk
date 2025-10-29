
/*******************************************************************************
 *   Ledger Nano S - Secure firmware
 *   (c) 2022 Ledger
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

#ifndef OS_H
#define OS_H

#include "bolos_target.h"

// FIXME: for backward compatibility. To be removed.
#include "os_debug.h"
#include "os_endorsement.h"
#include "os_halt.h"
#include "os_helpers.h"
#include "os_id.h"
#include "os_io.h"
#include "os_lib.h"
#include "os_math.h"
#include "os_nvm.h"
#include "os_pic.h"
#include "os_pin.h"
#include "os_random.h"
#include "os_print.h"
#include "os_registry.h"
#include "os_screen.h"
#include "os_seed.h"
#include "os_settings.h"
#include "os_task.h"
#include "os_utils.h"

// Keep these includes atm.
#include "os_types.h"

#include "syscalls.h"

/**
 * Quality development guidelines:
 * - NO header defined per arch and included in common if needed per arch, define below
 * - exception model
 * - G_ prefix for RAM vars
 * - N_ prefix for NVRAM vars (mandatory for x86 link script to operate correctly)
 * - C_ prefix for ROM   constants (mandatory for x86 link script to operate correctly)
 * - extensive use of * and arch specific C modifier
 */

#include "os_apilevel.h"

#include "arch.h"

#include "decorators.h"

/* ----------------------------------------------------------------------- */
/* -                            APPLICATION PRIVILEGES                   - */
/* ----------------------------------------------------------------------- */

#include "appflags.h"

/* ----------------------------------------------------------------------- */
/* -                            SYSCALL CRYPTO EXPORT                    - */
/* ----------------------------------------------------------------------- */

#define CXPORT_ED_DES 0x0001UL
#define CXPORT_ED_AES 0x0002UL
#define CXPORT_ED_RSA 0x0004UL

/* ----------------------------------------------------------------------- */
/* -                            ENTRY POINT                              - */
/* ----------------------------------------------------------------------- */

// os entry point
void app_main(void);

// os initialization function to be called by application entry point
void os_boot(void);

/**
 * Function takes 0 for first call. Returns 0 when timeout has occurred. Returned value is passed as
 * argument for next call, acting as a timeout context.
 */
unsigned short io_timeout(unsigned short last_timeout);

// reset the SE and wait forever
void os_reset(void) __attribute__((noreturn));

/* ----------------------------------------------------------------------- */
/* -                            EXCEPTIONS                               - */
/* ----------------------------------------------------------------------- */

#include "exceptions.h"

/* ----------------------------------------------------------------------- */
/* -                             ERROR CODES                             - */
/* ----------------------------------------------------------------------- */
#include "errors.h"

/**
BOLOS RAM LAYOUT
            msp                          psp                   psp
| bolos ram <-os stack-| bolos ux ram <-ux_stack-| app ram <-app stack-|

ux and app are seen as applications.
os is not an application (it calls ux upon user inputs)
**/

/* ----------------------------------------------------------------------- */
/* -                          DEBUG FUNCTIONS                           - */
/* ----------------------------------------------------------------------- */
// redefined if string.h not included
#ifdef HAVE_SPRINTF
#ifndef __APPLE__
int snprintf(char *str, size_t str_size, const char *format, ...);
#endif  // APPLE
#endif  // HAVE_SPRINTF

#ifndef HAVE_BOLOS
int compute_address_location(int address);
#endif

#endif  // OS_H
