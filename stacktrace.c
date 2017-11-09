//
// Created by kkyse on 11/8/2017.
//

#include "stacktrace.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <err.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>

#ifdef __linux__
    #include <err.h>
    #include <execinfo.h>
#endif

#define arraylen(array) (sizeof(array) / sizeof(*(array)))

#define PTR_MAX_STRLEN (((sizeof(void *) * 8) / 4) + 1)

#ifndef PATH_MAX
    #define PATH_MAX 4096
#endif

#ifdef __APPLE__
    #define addr2line_base "atos -o "
#else
    #define addr2line_base "addr2line -f -s -p -e "
#endif

static char addr2line_cmd[arraylen(addr2line_base) + PATH_MAX + arraylen(" ") + PTR_MAX_STRLEN + 1] = {0};
static char *addr_start = NULL;

int addr2line(const void *const addr) {
    if (!addr_start) {
        memcpy(addr2line_cmd, addr2line_base, sizeof(addr2line_base));
//        printf("%s\n", addr2line_cmd);
        if (readlink("/proc/self/exe", addr2line_cmd + arraylen(addr2line_base) - 1, PATH_MAX) == -1) {
            perror("readlink");
        }
//        printf("%s\n", addr2line_cmd);
        addr_start = addr2line_cmd + strlen(addr2line_cmd);
        *addr_start++ = ' ';
    }
    sprintf(addr_start, "%p", addr);
//    printf("addr2line cmd: %s\n", addr2line_cmd);
    fprintf(stderr, "    ");
    fflush(stderr);
    return system(addr2line_cmd);
}

int backtrace(void **buffer, int size);

char **backtrace_symbols(void *const *buffer, int size);

#define MAX_STACK_FRAMES 64

static void *stack_traces[MAX_STACK_FRAMES];

void posix_print_stack_trace() {
    const int trace_size = backtrace(stack_traces, MAX_STACK_FRAMES);
    const char **const messages = (const char **) backtrace_symbols(stack_traces, trace_size);
    for (uint32_t i = 0; i < trace_size; ++i) {
        if (addr2line(stack_traces[i]) != 0) {
            fprintf(stderr, "\terror determining line # for: %s\n", messages[i]);
        }
    }
    free(messages);
}

#define print_signal(signal, message) fprintf(stderr, "Caught %s: %s\n", signal, message); break

#define catch_signal(signal, message) case signal: print_signal(#signal, message)
#define catch_sub(signal, subsignal, message) \
    case subsignal: fprintf(stderr, "Caught %s (%s): %s\n", #signal, #subsignal, message); break
#define catch_FPE(subsignal, message) catch_sub(SIGFPE, subsignal, message)
#define catch_ILL(subsignal, message) catch_sub(SIGILL, subsignal, message)

void stack_trace_signal_handler_posix(int signal, siginfo_t *siginfo, void *context) {
//    printf("Stacktrace:\n");
    switch (signal) {
        catch_signal(SIGSEGV, "Segmentation Fault");
        catch_signal(SIGINT, "Interrupt: Interactive attention signal, usually Ctrl+C");
        catch_signal(SIGTERM, "Termination: a termination request was sent to the program");
        catch_signal(SIGABRT, "Abort: usually caused by an abort() or assert()");
        case SIGFPE:
            switch (siginfo->si_code) {
                catch_FPE(FPE_INTDIV, "integer divide by zero");
                catch_FPE(FPE_INTOVF, "integer overflow");
                catch_FPE(FPE_FLTDIV, "floating-point divide by zero");
                catch_FPE(FPE_FLTUND, "floating-point overflow");
                catch_FPE(FPE_FLTRES, "floating-point underflow");
                catch_FPE(FPE_FLTINV, "floating-point inexact result");
                catch_FPE(FPE_FLTSUB, "subscript out of range");
                default:
                print_signal("SIGFPE", "Arithmetic Exception");
            }
            break;
        case SIGILL:
            switch (siginfo->si_code) {
                catch_ILL(ILL_ILLOPC, "illegal opcode");
                catch_ILL(ILL_ILLOPN, "illegal operand");
                catch_ILL(ILL_ILLADR, "illegal addressing mode");
                catch_ILL(ILL_ILLTRP, "illegal trap");
                catch_ILL(ILL_PRVOPC, "privileged opcode");
                catch_ILL(ILL_PRVREG, "privileged register");
                catch_ILL(ILL_COPROC, "coprocessor error");
                catch_ILL(ILL_BADSTK, "internal stack error");
                default:
                print_signal("SIGILL", "Illegal Instruction");
            }
            break;
        default:
            print_signal("Unknown Signal", "Unknown Cause");
    }
    posix_print_stack_trace();
    _Exit(1);
}

#undef catch_ILL
#undef catch_FPE
#undef catch_sub
#undef catch_signal
#undef print_signal

static uint8_t alternate_stack[SIGSTKSZ];

#define add_action(signal) if (sigaction(signal, &sig_action, NULL) != 0) err(EXIT_FAILURE, "sigaction")

void set_stack_trace_signal_handler() {
    {
        // setup alternate stack
        stack_t ss = {.ss_sp = alternate_stack, .ss_size = SIGSTKSZ, .ss_flags = 0};
        if (sigaltstack(&ss, NULL) != 0) {
            err(EXIT_FAILURE, "sigaltstack");
        }
    }
    {
        // register stack trace signal handlers
        int flags = SA_SIGINFO;
        #ifndef __APPLE__
        // for some reason backtrace() doesn't work on macOS when using alternate stack
        flags |= SA_ONSTACK;
        #endif
        struct sigaction sig_action = {.sa_sigaction = stack_trace_signal_handler_posix, .sa_flags = flags};
        sigemptyset(&sig_action.sa_mask);
        
        add_action(SIGSEGV);
        add_action(SIGFPE);
        add_action(SIGINT);
        add_action(SIGILL);
        add_action(SIGILL);
        add_action(SIGTERM);
        add_action(SIGABRT);
        
//        printf("set stacktrace signal handler\n");
    }
}

#undef add_action

//int main(const int argc, const char *const *const argv) {
//    set_stack_trace_signal_handler();
//    for (;;) {
//        sleep(1);
//        printf("PID: %d\n", getpid());
//        if (argc == 0) {
//            break;
//        }
//    }
//}