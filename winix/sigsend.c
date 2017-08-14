#include <kernel/kernel.h>
#include <kernel/system.h>
#include <kernel/exception.h>
#include <winix/signal.h>

static unsigned int sigframe_code[2] = {0x1ee10001,0x200d0000};
//addui sp,sp, 1
//syscall

PRIVATE int build_user_stack(struct proc *who, void *src, size_t len){
    reg_t *sp = get_physical_addr(who->sp,who);
    sp -= len;
    memcpy(sp,src,len );
    who->sp = get_virtual_addr(sp,who);
    return 0;
}

PRIVATE int build_signal_ctx(struct proc *who, int signum){
    reg_t *sp;
    reg_t *ra;
    struct proc *systask;
    struct message sigret_mesg;
    struct sigframe sigframe;

    sp = who->sp;
    sp = get_physical_addr(sp,who);
    
    build_user_stack(who,who,PROCESS_CONTEXT_LEN);
    build_user_stack(who,sigframe_code,SIGFRAME_CODE_LEN);

    ra = who->sp;
    sigret_mesg.type = SYSCALL_SIGRET;
    sigret_mesg.i1 = signum;
    build_user_stack(who,&sigret_mesg,MESSAGE_LEN);

    sigframe.signum = signum;
    sigframe.operation = WINIX_SEND;
    sigframe.dest = SYSTEM_TASK;
    sigframe.pm = (struct message *)who->sp;
    build_user_stack(who,&sigframe,sizeof(struct sigframe));

    who->pc = (void (*)())who->sig_table[signum].sa_handler;
    who->ra = ra;

    //if the current process is blocked, unblock it temporarily during the signal handling
    if(who->flags)
        enqueue_schedule(who);

    who->flags = 0;//reset flags
    return OK;
}

PRIVATE bool sys_sig_handler(struct proc *who, int signum){
    struct message m;
    
    if(who->sig_table[signum].sa_handler == SIG_DFL){
        int exit_status = SIG_STATUS(signum);
        KDEBUG(("Signal %d: kill %s [%d]\n",signum,who->name,who->proc_nr));
        if(in_interrupt()){
            m.type = SYSCALL_EXIT;
            m.i1 = exit_status;
            m.src = who->proc_nr;
            wini_send(SYSTEM_TASK, &m);
        }else{
            exit_proc(who, exit_status);
        }
        return OK;
    }
    //if it's ignored
    if(who->sig_table[signum].sa_handler == SIG_IGN){
        KDEBUG(("Signal %d ignored by %d\n",signum,who->proc_nr));
        who->pc = (void (*)())((int)who->pc+1);
        return OK;
    }
    return ERR;
}

PRIVATE int sigsend_comm(struct proc *who, int signum){
    if(who->state != RUNNABLE)
        return ERR;

    if(build_signal_ctx(who,signum) != OK)
        return ERR;
    
    if(in_interrupt()){
        if(who->proc_nr == curr_mesg()->src){
            get_proc(SYSTEM_TASK)->pc = &intr_syscall;
        }
    }
    return OK;
}

int cause_sig(struct proc *who,int signum){
    if(sys_sig_handler(who,signum) == OK)
        return OK;

    return sigsend_comm(who,signum);
}

//send signal immediately
//IMPORTANT should only be called during exception
int send_sig(struct proc *who, int signum){
    if(sys_sig_handler(who,signum) == OK)
        return OK;

    if(sigsend_comm(who,signum) != OK)
        return OK;

    if(in_interrupt()){
        if(current_proc != who){
            enqueue_schedule(current_proc);
            dequeue_schedule(who);
        }

        current_proc = who;
        current_proc->ticks_left = current_proc->quantum;

        wramp_load_context();
    }
    return OK;
}
