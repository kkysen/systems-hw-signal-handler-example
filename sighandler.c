//
// Created by kkyse on 11/8/2017.
//

#ifndef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 201711L
#endif

#include <sys/types.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>

#include "sigaction.h"
#include "stacktrace.h"

#define ERROR_MSG "Program w/ PID = %d terminated due to SIGINT\n", getpid()
#define ERROR_LOG "exit_status.txt"

void sighandler(int signal, siginfo_t *siginfo, void *context) {
    switch (signal) {
        case SIGINT:
            printf(ERROR_MSG);
            FILE *file = fopen(ERROR_LOG, "a+");
            if (!file) {
                printf("Couldn't write error message to \"%s\"", ERROR_LOG);
                perror("fopen");
                exit(EXIT_FAILURE);
            }
            fprintf(file, ERROR_MSG);
            fclose(file);
            exit(EXIT_FAILURE);
        case SIGUSR1:
            printf("Caught SIGUSR1\nParent PID: %d\n", getppid());
            break;
        default:
            break;
    }
}

void add_sigaction() {
    struct sigaction new_action = {.sa_sigaction = sighandler, .sa_flags = 0};
    new_action.sa_sigaction = sighandler;
    sigemptyset(&new_action.sa_mask);
    new_action.sa_flags = 0;
    
    sigaction(SIGINT, &new_action, NULL);
    sigaction(SIGUSR1, &new_action, NULL);
}

void loop_once() {
    printf("PID: %d\n", getpid());
    getpid();
    sleep(1);
}

void forever_loop(const int argc) {
    for (;;) {
        loop_once();
        if (argc == 0) {
            break;
        }
        if (rand() % 10 == 0) {
            print_stack_trace();
        }
    }
}

void cause_segfault() {
    memset((void *) 1, 0, 10000u);
}

void recurse(int i) {
    if ((rand() % 100) == 0) {
        print_stack_trace();
        forever_loop(1);
    }
    if (i > 0) {
        recurse(i - 1);
    }
    cause_segfault();
}

int main(const int argc, const char *const *const argv) {
    srand(1);
    set_stack_trace_signal_handler();
    //add_sigaction();
    if (argc > 0) {
        recurse(1000);
    }
    forever_loop(argc);
}