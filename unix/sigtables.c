#include <tcl.h>
#include <signal.h>
#include <assert.h>
#include "sigtables.h"

#define SIGDECL(SIG) { SIG, #SIG, (sizeof(#SIG) - 1) }

const Signal
signals[] = {
    /* POSIX.1-1990 signals */
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
    /* SUSv2 and POSIX.1-2001 signals */
    SIGDECL(SIGBUS),
    SIGDECL(SIGPOLL),
    SIGDECL(SIGPROF),
    SIGDECL(SIGSYS),
    SIGDECL(SIGTRAP),
    SIGDECL(SIGURG),
    SIGDECL(SIGVTALRM),
    SIGDECL(SIGXCPU),
    SIGDECL(SIGXFSZ),
    /* Non-standard signals */
#ifdef SIGIOT
    SIGDECL(SIGIOT),
#endif
#ifdef SIGEMT
    SIGDECL(SIGEMT),
#endif
#ifdef SIGSTKFLT
    SIGDECL(SIGSTKFLT),
#endif
#ifdef SIGIO
    SIGDECL(SIGIO),
#endif
#ifdef SIGCLD
    SIGDECL(SIGCLD),
#endif
#ifdef SIGPWR
    SIGDECL(SIGPWR),
#endif
#ifdef SIGINFO
    SIGDECL(SIGINFO),
#endif
#ifdef SIGLOST
    SIGDECL(SIGLOST),
#endif
#ifdef SIGWINCH
    SIGDECL(SIGWINCH),
#endif
#ifdef SIGUNUSED
    SIGDECL(SIGUNUSED),
#endif
};

const int nsigs = sizeof(signals)/sizeof(signals[0]);

/* Maximal signal number among those supported by the
 * system this package was compiled for */
int max_signum;

static struct {
    const Signal **bysignum;
    Tcl_HashTable byname;
} tables;

static
void
InitLookupTables (void)
{
    int i;

    const int nbytes = sizeof(const Signal *) * max_signum;
    tables.bysignum = (const Signal **) ckalloc(nbytes);

    Tcl_InitHashTable(&tables.byname, TCL_STRING_KEYS);

    for (i = 0; i < max_signum; ++i) {
	tables.bysignum[i] = NULL;
    }

    for (i = 0; i < nsigs; ++i) {
	const Signal *sigPtr;
	int signum, index;
	const char *namePtr;
	Tcl_HashEntry *entryPtr;
	int isnew;

	signum  = signals[i].signal;
	namePtr = signals[i].name;

	index = SIGOFFSET(signum);
	sigPtr = tables.bysignum[index];
	if (sigPtr == NULL) {
	    sigPtr = &signals[i];
	    tables.bysignum[index] = sigPtr;
	}

	entryPtr = Tcl_CreateHashEntry(&tables.byname, namePtr, &isnew);
	assert(entryPtr != NULL && isnew);
	Tcl_SetHashValue(entryPtr, sigPtr);
    }
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
	int sig = signals[i].signal;
	if (sig != SIGINT && sig != SIGKILL) {
	    int index = SIGOFFSET(sig);
	    svPtr->items[index] = 1;
	}
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

    InitLookupTables();
}

MODULE_SCOPE
const char *
GetNameBySignum (
    Tcl_Interp *interp,
    int signum,
    int *lengthPtr
    )
{
    const Signal *sigPtr;

    sigPtr = FindSignalBySignum(signum);

    if (sigPtr != NULL) {
	if (lengthPtr != NULL) {
	    *lengthPtr = sigPtr->length;
	}
	return sigPtr->name;
    } else {
	if (interp != NULL) {
	    Tcl_SetObjResult(interp,
		Tcl_NewStringObj("invalid signum", -1));
	}
	return NULL;
    }
}

MODULE_SCOPE
int
GetSignumByName (
    Tcl_Interp *interp,
    const char *namePtr
    )
{
    const Signal *sigPtr;

    sigPtr = FindSignalByName(namePtr);
    if (sigPtr != NULL) {
	return sigPtr->signal;
    } else {
	if (interp != NULL) {
	    Tcl_SetObjResult(interp,
		Tcl_NewStringObj("invalid signal name", -1));
	}
	return -1;
    }
}

const Signal *
FindSignalBySignum (
    int signum
    )
{
    const int index = SIGOFFSET(signum);

    if (0 <= index && index <= SIGOFFSET(max_signum)) {
	return tables.bysignum[index];
    } else {
	return NULL;
    }
}

const Signal *
FindSignalByName (
    const char *namePtr
    )
{
    Tcl_HashEntry *entryPtr;

    entryPtr = Tcl_FindHashEntry(&tables.byname, namePtr);
    if (entryPtr != NULL) {
	return (const Signal *) Tcl_GetHashValue(entryPtr);
    } else {
	return NULL;
    }
}

/* vim: set ts=8 sts=4 sw=4 sts=4 noet: */
