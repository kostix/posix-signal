#include <tcl.h>
#include <signal.h>
#include "sigobj.h"
#include "syncpoints.h"
#include "events.h"
#include "utils.h"
#include "sigmanip.h"
#include "trap.h"


static void LockWorld (void);
static void UnlockWorld (void);


/* POSIX.1-2001 signal handler */
static
void
SignalAction (
    int signum,
    siginfo_t *si,
    void *uctx
    )
{
    LockSyncPoints();
    SignalSyncPoint(signum);
    UnlockSyncPoints();
}

static
int
InstallSignalHandler (
    int signum
    )
{
    struct sigaction sa;

    sa.sa_flags     = SA_RESTART | SA_SIGINFO;
    sa.sa_sigaction = &SignalAction;
    sigfillset(&sa.sa_mask);

    return sigaction(signum, &sa, NULL);
}

static
int
UninstallSignalHandler (
    int signum
    )
{
    struct sigaction sa;

    sa.sa_handler = SIG_DFL;

    return sigaction(signum, &sa, NULL);
}

static
int
TrapSet (
    ClientData clientData,
    Tcl_Interp *interp,
    Tcl_Obj *sigObj,
    Tcl_Obj *newCmdObj
    )
{
    int signum, res;

    signum = GetSignumFromObj(interp, sigObj);
    if (signum == -1) {
	return TCL_ERROR;
    }

    if (IsEmptyString(newCmdObj)) {
	/* Deletion of trap is requested */

	SyncPointMapEntry spoint;

	LockWorld();
	spoint = FindSyncPoint(signum);
	if (spoint == NULL) {
	    /* Do nothing -- syncpoint does not exist */
	    UnlockWorld();
	    return TCL_OK;
	} else {
	    DeleteSyncPoint(spoint);
	    DeleteEventHandler(signum);
	    Tcl_SetErrno(0);
	    res = UninstallSignalHandler(signum);
	    UnlockWorld();
	    if (res != 0) {
		ReportPosixError(interp);
		return TCL_OK;
	    } else {
		return TCL_OK;
	    }
	}
    } else {
	/* Trapping of the signal is requested */

	int isnew;

	LockWorld();
	AcquireSyncPoint(signum, &isnew);
	if (isnew) {
	    Tcl_SetErrno(0);
	    res = InstallSignalHandler(signum);
	    if (res != 0) {
		/* TODO delete syncpoint created above.
		 * Note that this poses another interesting problem:
		 * how to handle deletion of a syncpoint if it has
		 * pending signals on it? */
		UnlockWorld();
		ReportPosixError(interp);
		return TCL_ERROR;
	    }
	}
	SetEventHandler(signum, interp, newCmdObj);
	UnlockWorld();
	return TCL_OK;
    }
}

static
int
TrapGet (
    ClientData clientData,
    Tcl_Interp *interp,
    Tcl_Obj *sigObj
    )
{
    int signum;
    Tcl_Obj *cmdObj;

    signum = GetSignumFromObj(interp, sigObj);
    if (signum == -1) {
	return TCL_ERROR;
    }

    cmdObj = GetEventHandlerCommand(signum);
    if (cmdObj != NULL) {
	Tcl_SetObjResult(interp, cmdObj);
    } else {
	/* Do nothing in this case, as it is guaranteed
	 * that the interp result was reset to an empty
	 * string befor calling our command */
    }
    return TCL_OK;
}

MODULE_SCOPE
int
Command_Trap (
    ClientData clientData,
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *const objv[]
    )
{
    switch (objc) {
	case 3:
	    return TrapGet(clientData, interp, objv[2]);
	case 4:
	    return TrapSet(clientData, interp, objv[2], objv[3]);
	default:
	    Tcl_WrongNumArgs(interp, 2, objv, "signal ?command?");
	    return TCL_ERROR;
    }
}


static void
LockWorld (void)
{
    BlockAllSignals();
    LockSyncPoints();
}

static void
UnlockWorld(void)
{
    UnlockSyncPoints();
    UnblockAllSignals();
}

/* vim: set ts=8 sts=4 sw=4 sts=4 noet: */
