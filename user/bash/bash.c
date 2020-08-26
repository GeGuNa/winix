/**
 * 
 * Simple shell for WINIX.
 *
 * @author Bruce Tan
 * @email brucetansh@gmail.com
 * 
 * @author Paul Monigatti
 * @email paulmoni@waikato.ac.nz
 * 
 * @create date 2016-09-19
 * 
*/

#include "bash.h"

// Input buffer & tokeniser
static char buf[MAX_LINE];
static pid_t pgid;

// Prototypes
CMD_PROTOTYPE(ps);
CMD_PROTOTYPE(cmd_kill);
CMD_PROTOTYPE(print_pid);
CMD_PROTOTYPE(mem_info);
CMD_PROTOTYPE(do_trace_syscall);
CMD_PROTOTYPE(help);
CMD_PROTOTYPE(cmd_exit);
CMD_PROTOTYPE(printenv);
CMD_PROTOTYPE(cmd_bash);
CMD_PROTOTYPE(do_cd);
CMD_PROTOTYPE(do_cls);


// Command handling
struct cmd_internal builtin_commands[] = {
    { printenv, "printenv" },
    { do_trace_syscall, "trace"},
    { cmd_bash, "bash"},
    { ps, "ps"},
    { cmd_kill, "kill"},
    { print_pid, "pid"},
    { mem_info, "free"},
    { cmd_exit, "exit"},
    { help, "help"},
    { do_cd, "cd"},
    { do_cls, "clear"},
    { 0, NULL}
};

void init_shell(){
    signal(SIGINT, SIG_IGN);
    signal(SIGTERM, SIG_IGN);
    signal(SIGTSTP, SIG_IGN);
    pgid = setsid();
    ioctl(STDIN_FILENO, TIOCSCTTY);
    ioctl(STDIN_FILENO, TIOCSPGRP, pgid);
}


int main() {
    int ret, len;
    char *c;

    init_shell();

    while(1) {
        printf("WINIX> ");
        ret = read(0, buf, MAX_LINE);

        if(ret == EOF){
            if(errno == EINTR){
                perror("stdin: ");
                printf("WINIX> ");
            }
            continue;
        }
        len = strlen(buf);
        buf[len - 2] = '\0'; // delete end line
        exec_cmd(buf, NULL);     
    }
    return 0;
}

int search_path(char* path, char* name){
    strcpy(path, "/bin/");
    strcat(path, name);
    return access(path, F_OK);
}

int _exec_cmd_pipe(struct cmdLine *cmd, int cmd1_idx, int cmd2_idx, int prev_fd, int next_fd){
    return 0;
}

/**
 * Handles any unknown command.
 **/
int _exec_cmd(char *line, struct cmdLine *cmd) {
    char buffer[128];
    int status;
    int ret;
    pid_t pid, second_pid;
    sigset_t sigmask = 0;
    int saved_stdin, saved_stdout;
    int sin = STDIN_FILENO, sout = STDOUT_FILENO;

    if(cmd->argc == 0)
        return -1;
    if(*line == '#')
        return 0;

    enable_syscall_tracing();

    pid = vfork();
    printf(" vfork %d\n", pid);
    if(!pid){
        
        if(cmd->outfile){ //if redirecting output
            int mode = O_WRONLY | O_CREAT;
            saved_stdout = dup(STDOUT_FILENO); //backup stdout
            if(cmd->append) //if append
                mode |= O_APPEND;
            else //else replace the original document
                mode |= O_TRUNC;
            sout = open(cmd->outfile, mode, 0644);
            dup2(sout,STDOUT_FILENO);
            close(sout);
        }

        if(cmd->infile){ //if redirecting input
            saved_stdin = dup(STDIN_FILENO); //backup stdin
            sin = open(cmd->infile, O_RDONLY);
            dup2(sin,STDIN_FILENO);
            close(sin);
        }
        
        if(search_path(buffer, cmd->argv[0]) == 0){
            second_pid = vfork();
            if(second_pid == 0){
                pid_t child_pgid;
                signal(SIGINT, SIG_DFL);
                signal(SIGTERM, SIG_DFL);
                signal(SIGTSTP, SIG_DFL);
                setpgid(0, 0);
                child_pgid = getpgid(0);
                ioctl(STDIN_FILENO, TIOCSPGRP, child_pgid);
                execv(buffer, cmd->argv);
                exit(-1);
            }else{
                ret = wait(&status);
                
                // printf("parent awaken\n");
            }
        }else{
            fprintf(stderr, "Unknown command '%s'\r\n", cmd->argv[0]);
            return -1;
        }

        if(cmd->outfile){
            dup2(saved_stdout,STDOUT_FILENO);
            close(saved_stdout);
        }

        if(cmd->infile){
            dup2(saved_stdin,STDIN_FILENO);
            close(saved_stdin);
        }
        exit(0);
    }

    printf("parent vfork\n");
    ioctl(STDIN_FILENO, TIOCENABLEECHO);
    ioctl(STDIN_FILENO, TIOCSPGRP, pgid);
    printf("\n");
    return 0;
}

int exec_cmd(char *line, int tpipe[2]){
    struct cmdLine cmd;
    int ret;
    char *p;
    char* buf;
    struct cmd_internal *handler = NULL;

    ret = parse(line,&cmd);

    if(cmd.env && cmd.env_val){ // if a new environment variable is set
        buf = malloc(MAX_LINE);
        parse_quotes(cmd.env_val, buf);
        
        if(*buf){
            printf("setenv %s=%s\n", cmd.env, buf);
            setenv(cmd.env, buf, 1);
        }
        free(buf);
        return 0;
    }

    // Decode command
    handler = builtin_commands;
    while(handler && handler->name && strcmp(cmd.argv[0], handler->name)) {
        handler++;
    }
    // Run it
    if(handler->name){
        return handler->handle(cmd.argc, cmd.argv);
    }
    return _exec_cmd(line, &cmd);
}


int printenv(int argc, char **argv){
    const char **v;
    const char *p;
    v = environ;
    while((p = *v++)){
        printf("%s\n",p);
    }
    return 0;
}

int cmd_kill(int argc, char **argv){
    pid_t pid;
    int signum = SIGTERM;
    if(argc < 2){
        printf("kill [-s signum] pid\n");
        return -1;
    }
        
    if(strcmp("-s",argv[1]) == 0){
        signum = atoi(argv[2]);
        pid = atoi(argv[3]);
    }else{
        pid = atoi(argv[1]);
    }

    if(kill(pid,signum))
        perror("kill ");
    return 0;
}

int do_cd(int argc, char** argv){
    int ret;
    if(argc < 2)
        return -1;
    ret = chdir(argv[1]);
    return ret;
}

int help(int argc, char** argv){
    struct cmd_internal* handler;
    handler = builtin_commands;
    printf("Available Commands\n");
    while(handler && handler->name != NULL) {
        printf(" * %s\n",handler->name);
        handler++;
    }
    return 0;
}

// byte stream for \e[1;1H\e[2J
char cls[] = {0x1b, 0x5b, 0x31, 0x3b, 0x31, 0x48, 0x1b, 0x5b, 0x32, 0x4a, 0};

int do_cls(int argc, char** argv){
    printf("%s", cls);
    return 0;
}

int do_trace_syscall(int argc, char** argv){
    return enable_syscall_tracing();
}

// Print the system wise memory info
int mem_info(int argc, char** argv){
    struct message m;
    m.m1_i1 = WINFO_MEM;
    return _syscall(WINFO, &m);
}

// current pid
int print_pid(int argc, char **argv){
    printf("%d\n",getpid());
    return 0;
}

// list all the processes in the system
int ps(int argc, char **argv){
    struct message m;
    m.m1_i1 = WINFO_PS;
    return _syscall(WINFO, &m);
}

// start a new bash shell, parent shell is blocked until child shell exits
int cmd_bash(int argc, char **argv){
    pid_t child_pid;
    if(child_pid = fork()){
        if(child_pid == -1){
            perror("fork failed");
            return -1;
        }
        // printf("parent shell %d waiting for child shell %d\n",getpid(),child_pid);
        child_pid = wait(NULL);
        // printf("parent shell %d awakened by child shell %d\n",getpid(),child_pid);
    }else{
        printf("Child shell %d [parent %d] start:\n",getpid(),getppid());
    }
    return 0;
}

int cmd_exit(int argc, char **argv){
    int status = 0;
    if(argc > 1)
        status = atoi(argv[1]);
    printf("Bye!\n");
    // printf("Child %d [parent %d] exits\n",getpid(),getppid());
    return exit(status);
}




