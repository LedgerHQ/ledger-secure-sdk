#include "os_task.h"

void os_sched_exit(__attribute__((unused)) bolos_task_status_t exit_code)
{
    return;
}

void os_longjmp(unsigned int exception)
{
#ifdef HAVE_DEBUG_THROWS

    DEBUG_THROW(exception);
    return;
#endif
    return;
}
