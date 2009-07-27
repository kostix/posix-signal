#include <tcl.h>
#include <sys/types.h>
#include <signal.h>
#include "sigtables.h"
#include "utils.h"
#include "send.h"


MODULE_SCOPE
int
Command_Send (
    ClientData clientData,
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *const objv[]
    )
{
    int signum, pid, res;

    if (objc != 4) {
	Tcl_WrongNumArgs(interp, 2, objv, "signal pid");
	return TCL_ERROR;
    }

    signum = GetSignalIdFromObj(interp, objv[2]);
    if (signum == -1) {
	return TCL_ERROR;
    }

    res = Tcl_GetIntFromObj(interp, objv[3], &pid);
    if (res != TCL_OK) {
	return TCL_ERROR;
    }

    Tcl_SetErrno(0);
    res = kill((pid_t) pid, signum);
    if (res == -1) {
	ReportPosixError(interp);
	return TCL_ERROR;
    } else {
	return TCL_OK;
    }
}

/* vim: set ts=8 sts=4 sw=4 sts=4 noet: */
