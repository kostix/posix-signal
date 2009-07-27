#include <tcl.h>
#include <signal.h>
#include "sigtables.h"
#include "syncpoints.h"
#include "events.h"
#include "utils.h"
#include "trap.h"


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
TrapSet (
    ClientData clientData,
    Tcl_Interp *interp,
    Tcl_Obj *sigObj,
    Tcl_Obj *newCmdObj
    )
{
    int id, res;

    id = GetSignalIdFromObj(interp, sigObj);
    if (id == -1) {
	return TCL_ERROR;
    }

    /* TODO SetEventHandler() should return some
     * flag telling us whether the new script was
     * installed/modified or deleted. In the latter
     * case we should delete the syncpoint and
     * change signal disposition to SIG_DFL.
     * Note that this looks like a ternary logic:
     * o Script is set
     * o Script is deleted
     * o No change
     */
    SetEventHandler(id, interp, newCmdObj);

    LockSyncPoints();
    SetSyncPoint(id);
    UnlockSyncPoints();

    Tcl_SetErrno(0);
    res = InstallSignalHandler(id);
    if (res != 0) {
	/* FIXME ideally we should somehow revert changes
	 * made to cmdObj */
	ReportPosixError(interp);
	return TCL_ERROR;
    }

    return TCL_OK;
}

static
int
TrapGet (
    ClientData clientData,
    Tcl_Interp *interp,
    Tcl_Obj *sigObj
    )
{
    int id;
    Tcl_Obj *cmdObj;

    id = GetSignalIdFromObj(interp, sigObj);
    if (id == -1) {
	return TCL_ERROR;
    }

    cmdObj = GetEventHandlerCommand(id);
    if (cmdObj != NULL) {
	Tcl_SetObjResult(interp, cmdObj);
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

/* vim: set ts=8 sts=4 sw=4 sts=4 noet: */
