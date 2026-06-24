#pragma once

#include "appflags.h"
#include "bolos_target.h"
#include "decorators.h"
#include "os_types.h"

typedef unsigned char bolos_task_status_t;

enum task_unsecure_id_e {
    TASK_BOLOS = 0,  // can call os
    TASK_SYSCALL,    // can call os
    TASK_USER,       // must call syscalls to reach os, locked in app ram
    TASK_BOLOS_UX,   // must call syscalls to reach os
    TASK_MAXCOUNT    // must be last in the structure
};

// exit the current task
#ifdef HAVE_BOLOS
SYSCALL void os_sched_exit(bolos_task_status_t exit_code);
#else
SYSCALL void __attribute__((noreturn)) os_sched_exit(bolos_task_status_t exit_code);
#endif

/**
 * Retrieve the last status issued by a task using either yield or exit.
 */
SUDOCALL bolos_task_status_t os_sched_last_status(unsigned int task_idx);
