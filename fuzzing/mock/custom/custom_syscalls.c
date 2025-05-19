#ifndef __weak
#define __weak __attribute__((weak))
#endif
#include "ox_aes.h"

#include "bolos_target.h"
#include "exceptions.h"
#include "lcx_aes.h"
#include "lcx_eddsa.h"
#include "lcx_wrappers.h"
#include "cx_errors.h"
#include "os_task.h"
#include "os_memory.h"
#include "os_registry.h"
#include "os_ux.h"
#ifdef HAVE_SE_TOUCH
#include "os_io.h"
#endif  // HAVE_SE_TOUCH
#include "ox_ec.h"
#include "ox_bn.h"
#include "syscalls.h"
#if defined(HAVE_LANGUAGE_PACK)
#include "ux.h"
#endif  // defined(HAVE_LANGUAGE_PACK)
#ifdef HAVE_NBGL
#include "nbgl_types.h"
#include "nbgl_fonts.h"
#include "os_pic.h"
#endif
#if defined(HAVE_VSS)
#include "ox_vss.h"
#endif  // HAVE_VSS
#include "os_seed.h"
#include "ox_crc.h"
#include "os_endorsement.h"
#include <string.h>

#pragma GCC diagnostic                                      ignored "-Wunused-parameter"

cx_err_t cx_ecdomain_parameters_length(cx_curve_t cv, size_t *length)
{
    *length = (size_t)32;
    return 0x00000000;
}
