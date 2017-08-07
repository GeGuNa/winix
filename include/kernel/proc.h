/**
 * Process Management for WINIX.
 *
 * Revision History:
 *  2016-09-19		Paul Monigatti			Original
 *  2016-11-20		Bruce Tan			Modified
 **/

#ifndef _W_PROC_H_
#define _W_PROC_H_ 1

#include <winix/kwramp.h>

//Process & Scheduling
#define PROC_NAME_LEN			20
#define NUM_PROCS				13
#define NUM_QUEUES				5
#define IDLE_PRIORITY			4
#define USER_PRIORITY			3
#define SYSTEM_PRIORITY			0

#define USER_PRIORITY_MAX		1
#define USER_PRIORITY_MIN		4

//Signal Context
#define PROCESS_REGS_LEN		16
#define PROCESS_CONTEXT_LEN		21

//Process Defaults
#define DEFAULT_FLAGS			0
#define PTABLE_LEN				32
#define DEFAULT_STACK_SIZE		1024
#define DEFAULT_HEAP_SIZE		1024
#define DEFAULT_CCTRL			0xff9
#define DEFAULT_STACK_POINTER			0x00000
#define USER_CCTRL			0x8 //OKU is set to 0
#define DEFAULT_RBASE			0x00000
#define DEFAULT_PTABLE			0x00000
#define DEFAULT_QUANTUM			100
#define DEFAULT_REG_VALUE		0xffffffff
#define DEFAULT_MEM_VALUE		0xffffffff
#define DEFAULT_RETURN_ADDR		0x00000000
#define DEFAULT_PROGRAM_COUNTER	0x00000000

//Process Flags
#define SENDING					0x0001
#define RECEIVING				0x0002
#define WAITING					0x0004

/**
 * State of a process.
 **/
typedef enum { DEAD, INITIALISING, RUNNABLE, ZOMBIE } proc_state_t;

/**
 * Process structure for use in the process table.
 *
 * Note: 	Do not update this structure without also
 * 			updating the definitions in "wramp.s"
 **/
typedef struct proc {
	/* Process State */
	reg_t regs[NUM_REGS];	//Register values
	reg_t *sp;
	reg_t *ra;
	void (*pc)();
	reg_t *rbase;
	reg_t *ptable;
	reg_t cctrl;  //len 19

	/* IPC messages */
	struct message *message;	//Message buffer;
	int flags; 

	/* Heap */
	ptr_t* heap_break;
	size_t length; //length is depreciated, do not use it

	/* Protection */
	reg_t protection_table[PTABLE_LEN];

	/* IPC queue */
	struct proc *sender_q;	//Head of process queue waiting to send to this process
	struct proc *next_sender; //Link to next sender in the queue

	/* Scheduling */
	struct proc *next;	//Next pointer
	int priority;	
	int quantum;		//Timeslice length
	int ticks_left;		//Timeslice remaining

	/* Accounting */
	int time_used;		//CPU time used

	/* Metadata */
	char name[PROC_NAME_LEN];		//Process name
	proc_state_t state;	//Current process state
	int exit_status;	//Storage for status when process exits
	int sig_status;		//Storage for siginal status when process exits
	pid_t pid;			//Process id
	pid_t procgrp;		//Pid of the process group (used for signals)
	pid_t wpid;			//pid this process is waiting for
	int parent;			//proc_index of parent

	/* Process Table Index */
	int proc_nr;		//Index in the process table
	int IN_USE;			//Whether the current slot is in use

	/* Signal Information */
	sigset_t pending_sigs;
	struct sigaction sig_table[_NSIG];

	/* Alarm */
	struct timer alarm;
} proc_t;


#define isokprocn(i)	(i>= 0 && i < NUM_PROCS)

extern struct proc proc_table[NUM_PROCS];
extern struct proc *ready_q[NUM_QUEUES][2];
extern struct proc *block_q[2];


void enqueue_tail(struct proc **q, struct proc *proc);
void enqueue_head(struct proc **q, struct proc *proc);
struct proc *dequeue(struct proc **q);
/**
 * Initialises the process table and scheduling queues.
 **/
void init_proc();
void proc_set_default(struct proc *p);
/**
 * Creates a new process and adds it to the runnable queue.
 **/
struct proc *new_proc(void (*entry)(), int priority, const char *name);
struct proc *get_free_proc_slot();
void add_to_scheduling_queue(struct proc* p);
/**
 * WINIX Scheduler.
 **/
void sched();

/**
 * Frees up a process.
 *
 * Parameters:
 *   p		The process to remove.
 **/
void end_process(struct proc *p);

/**
 * Gets a pointer to a process.
 *
 * Parameters:
 *   proc_nr		The process to retrieve.
 *
 * Returns:			The relevant process, or NULL if it does not exist.
 **/
struct proc *get_proc(int proc_nr);
struct proc *get_running_proc(int proc_nr);


//fork the next process in the ready_q, return the new pid of the forked process
//side effect: the head of the free_proc is dequeued, and added to the ready_q with all relevant values equal
//to the original process, except stack pointer.

struct proc *kexecp(struct proc *p,void (*entry)(), int priority, const char *name);
struct proc *start_system(void (*entry)(), int priority, const char *name);
struct proc* start_init(size_t *lines, size_t length, size_t entry);


void process_overview();
void printProceInfo(struct proc* curr);
char* getStateName(proc_state_t state);
struct proc *pick_proc();


/**
 * Pointer to the current process.
 **/
extern struct proc *current_proc;


#endif
