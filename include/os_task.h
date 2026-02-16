#pragma once

#include "appflags.h"
#include "bolos_target.h"
#include "decorators.h"
#include "os_types.h"

typedef unsigned char bolos_task_status_t;

enum task_unsecure_id_e {
    TASK_BOLOS = 0,  // can call os
    TASK_SYSCALL,    // can call os
    TASK_USERTASKS_START,
    // disabled for now // TASK_USER_UX, // must call syscalls to reach os, locked in ux ram
    TASK_USER = TASK_USERTASKS_START,  // must call syscalls to reach os, locked in app ram
    TASK_SUBTASKS_START,
    TASK_SUBTASK_0 = TASK_SUBTASKS_START,
#ifdef TARGET_NANOX
    TASK_SUBTASK_1,  // TODO: Remove
    TASK_SUBTASK_2,
    TASK_SUBTASK_3,
#endif  // TARGET_NANOX
    TASK_BOLOS_UX,
    TASK_MAXCOUNT,  // must be last in the structure
};

// exit the current task
#ifdef HAVE_BOLOS
SYSCALL void os_sched_exit(bolos_task_status_t exit_code);
#else
SYSCALL void __attribute__((noreturn)) os_sched_exit(bolos_task_status_t exit_code);
#endif

// returns true when the given task is running, false else.
SYSCALL bolos_bool_t os_sched_is_running(unsigned int task_idx);

/**
 * Retrieve the last status issued by a task using either yield or exit.
 */
SUDOCALL bolos_task_status_t os_sched_last_status(unsigned int task_idx);

/**
 * Current task is yielding the process to another task.
 * Meta call for task_switch with 'the enxt' task idx.
 * @param status is the current task status
 */
SUDOCALL TASKSWITCH void os_sched_yield(bolos_task_status_t status);

/**
 * Perform task switching
 * @param task_idx is the task index to switch to
 * @param status of the currently executed task
 * @return the status of the previously running task
 */
SUDOCALL TASKSWITCH void os_sched_switch(unsigned int task_idx, bolos_task_status_t status);

/**
 * Function that returns the currently running task identifier.
 */
SUDOCALL unsigned int os_sched_current_task(void);
