//
// Created by kkyse on 11/8/2017.
//

#ifndef SYSTEMS_SIGACTION_H
#define SYSTEMS_SIGACTION_H

#ifndef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 201711L
#endif

#include <signal.h>
#include <sys/signal.h>

#ifndef SA_ONSTACK
// Call signal handler on alternate signal stack provided by sigaltstack(2).
    #define SA_ONSTACK   0x20000000
#endif

int sigaction(int signum, const struct sigaction *action, struct sigaction *old_action);

int sigemptyset(sigset_t *set);

#endif //SYSTEMS_SIGACTION_H
