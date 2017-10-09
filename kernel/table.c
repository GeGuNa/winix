#include <kernel/kernel.h>
#include <kernel/table.h>

/*
    struct proc_config{
        char name[PROC_NAME_LEN];
        void (*entry)();
        int proc_nr;
        int quantum;
        int priority;
    };
*/

struct boot_image boot_table[NUM_TASKS] = {
    {"IDLE",   idle_main,           IDLE,   1,  MIN_PRIORITY,},
    {"CLOCK",  clock_main,          CLOCK,  32, MAX_PRIORITY,},
    {"SYSTEM", system_main,         SYSTEM, 64, MAX_PRIORITY,},
};

//IDLE's process number must be the lowest
BUILD_BUG_ON_FALSE(IDLE == -NUM_TASKS + 1); 

char *initial_env[] = {
    "HOME=/",
    "PATH=/bin",
    NULL
};

syscall_handler_t syscall_table[_NSYSCALL] = {{0}};



