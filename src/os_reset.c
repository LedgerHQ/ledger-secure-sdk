/* @BANNER@ */

#include "os_io_seph_cmd.h"
#include "os_reset.h"

void os_reset(void)
{
    os_io_seph_cmd_se_reset();
    for (;;) {
    }
}
