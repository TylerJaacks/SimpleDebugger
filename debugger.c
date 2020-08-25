#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <signal.h>
#include <syscall.h>
#include <sys/ptrace.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/reg.h>
#include <sys/user.h>
#include <unistd.h>
#include <errno.h>

void run_target(const char* program_name);
void run_debugger(pid_t child_pid);
void procmsg(const char* format, ...);

int main(int argc, char* argv[]) {
    pid_t child_pid;

    if (argc < 2) {
        fprintf(stderr, "Expected a program name as argument.\n");

        return -1;
    }

    child_pid = fork();

    if (child_pid == 0)
        run_target(argv[1]);
    else if (child_pid > 0)
        run_debugger(child_pid);
    else {
        perror("fork");

        return -1;
    }

    return 0;
}

void procmsg(const char* format, ...) {
    va_list ap;
    fprintf(stdout, "[%d] ", getpid());
    va_start(ap, format);
    vfprintf(stdout, format, ap);
    va_end(ap);
}

void run_target(const char* program_name) {
    procmsg("target started. Will run '%s'.\n", program_name);

    if (ptrace(PT_TRACE_ME, 0, 0, 0) < 0) {
        perror("ptrace");

        return;
    }

    execl(program_name, program_name, 0);
}

void run_debugger(pid_t child_pid) {
    int wait_status;
    unsigned i = 0;

    procmsg("debugger started!\n");

    wait(&wait_status);

    while (WIFSTOPPED(wait_status)) {
        i++;

        if (ptrace(PT_STEP, child_pid, 0, 0) < 0) {
            perror("ptrace");

            return;
        }

        wait(&wait_status);
    }

    procmsg("the child executed %u instructions\n", i);
}