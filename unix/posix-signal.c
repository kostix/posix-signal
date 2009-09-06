#include <tcl.h>
#include <signal.h>
#include <assert.h>
#include <stdio.h>
#include "sigtables.h"
#include "syncpoints.h"
#include "sigmanip.h"
#include "events.h"
#include "trap.h"
#include "send.h"
#include "info.h"


/* Sentinel for the initialization of the package global state */
static int packageRefcount = 0;
TCL_DECLARE_MUTEX(pkgInitLock);


static
int
Signal_Command (
    ClientData clientData,
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *const objv[]
	)
{
    const char *cmds[] = { "trap", "send", "info", NULL };
    Tcl_ObjCmdProc *const procs[] = {
	Command_Trap,
	Command_Send,
	Command_Info
    };

    int cmd;

    if (objc < 2) {
	Tcl_WrongNumArgs(interp, 1, objv,
		"subcommand ?arg ...?");
	    return TCL_ERROR;
    }

    if (Tcl_GetIndexFromObj(interp, objv[1],
	    cmds, "subcommand", 0, &cmd) != TCL_OK) {
	return TCL_ERROR;
    }

    return procs[cmd](clientData, interp, objc, objv);
}


/* This handler is called when Tcl is about to terminate
 * the process. This handler is called from the thread
 * which called Tcl_Exit (or from the main thread?);
 * no guarantee is made to properly cleanup other
 * threads, in fact, they will not be cleaned up properly.
 * So our tasks is just to terminate the manager thread
 * as it uses several synchronization variables which
 * will be cleaned up by the ongoing shutdown process
 * (see commit 3b9d246).
 * Note that at this point there might exist active
 * signal handlers, and this thread can be selected
 * by the system to serve a signal any time, so we
 * must block all the signals before processing.
 */
static
void
PrepareShutdown (
    ClientData clientData)
{
    BlockAllSignals();
    LockSyncPoints();
    DisableSyncpoints();
    UnlockSyncPoints();
    UnblockAllSignals();
}


/*
 * Note that when the package usage refcount goes to 1
 * here, we can assume no signal is being handled by our
 * code yet and so we might omit blocking and unblocking
 * the signals in this thread.
 */
static
void
InitPackage (void)
{
    Tcl_MutexLock(&pkgInitLock);

    if (packageRefcount == 0) {
	InitSignalTables();
	InitSyncPoints();

	LockSyncPoints();
	EnableSyncpoints();
	UnlockSyncPoints();

	Tcl_CreateExitHandler(PrepareShutdown, NULL);
    }
    ++packageRefcount;

    Tcl_MutexUnlock(&pkgInitLock);
}


static
void
CleanupPackage (
    ClientData clientData)
{
    Tcl_MutexLock(&pkgInitLock);

    --packageRefcount;
    if (packageRefcount == 0) {
	FinalizeSyncpoints();
	/* TODO Free all allocated data structures.
	 * Possibly we can assume that no threads
	 * serve any signals at this point and have
	 * no handlers installed (this is merely
	 * a "contract", but it possibly can be
	 * enforced if we arrange for the threads'
	 * cleanup handlers to be called before this one. */
    }

    Tcl_MutexUnlock(&pkgInitLock);
}

int
Posixsignal_Init(Tcl_Interp * interp)
{
#ifdef USE_TCL_STUBS
    if (Tcl_InitStubs(interp, "8.1", 0) == NULL) {
	return TCL_ERROR;
    }
#endif
    if (Tcl_PkgRequire(interp, "Tcl", "8.1", 0) == NULL) {
	return TCL_ERROR;
    }

    /* Initialize global state which is shared by all loaded
     * instances of this package and all threads */
    InitPackage();

    /* This initializer must be run each time the package
     * is loaded */
    InitEventHandlers();

    Tcl_CreateThreadExitHandler(CleanupPackage, NULL);

    Tcl_CreateObjCommand(interp, PACKAGE_NAME,
	    Signal_Command, NULL, NULL);

    if (Tcl_PkgProvide(interp, PACKAGE_NAME, PACKAGE_VERSION) != TCL_OK) {
	return TCL_ERROR;
    }

    return TCL_OK;
}

/* vim: set ts=8 sts=4 sw=4 sts=4 noet: */
