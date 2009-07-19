#include <tcl.h>
#include <signal.h>
#include <assert.h>
#include "sigtables.h"

#define SIGDECL(SIG) { SIG, #SIG }

const Signal
signals[] = {
    SIGDECL(SIGHUP),
    SIGDECL(SIGINT),
    SIGDECL(SIGQUIT),
    SIGDECL(SIGILL),
    SIGDECL(SIGABRT),
    SIGDECL(SIGFPE),
    SIGDECL(SIGKILL),
    SIGDECL(SIGSEGV),
    SIGDECL(SIGPIPE),
    SIGDECL(SIGALRM),
    SIGDECL(SIGTERM),
    SIGDECL(SIGUSR1),
    SIGDECL(SIGUSR2),
    SIGDECL(SIGCHLD),
    SIGDECL(SIGCONT),
    SIGDECL(SIGSTOP),
    SIGDECL(SIGTSTP),
    SIGDECL(SIGTTIN),
    SIGDECL(SIGTTOU),
};

const int nsigs = sizeof(signals)/sizeof(signals[0]);

/* Maximal signal number among those supported by the
 * system this package was compiled for */
int max_signum;

static const char **signames;

static
void
InitLookupTable (void)
{
    int i;

    const int len = nsigs + 1; // account for the terminating NULL entry

    signames = (const char**) ckalloc(sizeof(signames[0]) * len);

    for (i = 0; i < nsigs; ++i) {
	signames[i] = signals[i].name;
    }
    signames[nsigs] = NULL;
}

MODULE_SCOPE
SignalVector*
CreateSignalVector (void)
{
    int i, len, nbytes;
    SignalVector *svPtr;

    /* Create a vector of integers to hold the number
     * of elements one less that the maximal signal number */
    len = SIGOFFSET(max_signum);
    nbytes = sizeof(*svPtr) + sizeof(svPtr->items[0]) * len;
    svPtr = (SignalVector*) ckalloc(nbytes);
    svPtr->len = len;
    for (i = 0; i < len; ++i) {
	svPtr->items[i] = 0;
    }

    /* Create handlers for known signals */
    for (i = 0; i < nsigs; ++i) {
	int index = SIGOFFSET(signals[i].signal);
	svPtr->items[index] = 1;
    }	

    return svPtr;
}

MODULE_SCOPE
void
FreeSignalVector(
    SignalVector *svPtr
    )
{
    ckfree((char*) svPtr);
}

MODULE_SCOPE
void
InitSignalTables (void)
{
    int i, max;

    /* Find maximal signal number */
    max = 0;
    for (i = 0; i < nsigs; ++i) {
	int sig = signals[i].signal;
	if (sig > max) {
	    max = sig;
	}
    }
    assert(max > 0);
    max_signum = max;

    InitLookupTable();
}

MODULE_SCOPE
int
GetSignalIdFromObj (
    Tcl_Interp *interp,
    Tcl_Obj *nameObj
    )
{
    int res, index;

    res = Tcl_GetIndexFromObj(interp, nameObj, signames,
	    "signal name", 0, &index);
    if (res == TCL_OK) {
	return signals[index].signal;
    } else {
	return -1;
    }
}

/* vim: set ts=8 sts=4 sw=4 sts=4 noet: */
