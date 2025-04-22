typedef unsigned char bolos_task_status_t;

void os_sched_exit(bolos_task_status_t exit_code);

void os_longjmp(unsigned int exception);