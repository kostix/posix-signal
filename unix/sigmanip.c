#include <tcl.h>
#include <signal.h>
#include <errno.h>
#include "sigmanip.h"

static void SetSigmalMask(sigset_t *sigsetPtr);

void
BlockAllSignals (void)
{
    sigset_t sigset;

    sigfillset(&sigset);
    SetSigmalMask(&sigset);
}

void
UnblockAllSignals (void)
{
    sigset_t sigset;

    sigemptyset(&sigset);
    SetSigmalMask(&sigset);
}

static void
SetSigmalMask(
    sigset_t *sigsetPtr)
{
    int code;

#ifdef TCL_THREADS
    code = pthread_sigmask(SIG_SETMASK, sigsetPtr, NULL);
#else
    code = sigprocmask(SIG_SETMASK, sigsetPtr, NULL);
    if (code != 0) {
	code = errno;
    }
#endif
    if (code != 0) {
	Tcl_Panic(PACKAGE_NAME ": failed to set signal mask: %d", code);
    }
}

/* vim: set ts=8 sts=4 sw=4 sts=4 noet: */
