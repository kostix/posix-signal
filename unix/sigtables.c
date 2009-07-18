#include <tcl.h>
#include <signal.h>
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
void
InitSignalTables (void)
{
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
