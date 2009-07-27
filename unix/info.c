#include <tcl.h>
#include <signal.h>
#include "info.h"


static
int
TopicCmd_Sigrtmin (
    ClientData clientData,
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *const objv[]
    )
{
    if (objc != 3) {
	Tcl_WrongNumArgs(interp, 2, objv, "sigrtmin");
	return TCL_ERROR;
    }

    Tcl_SetObjResult(interp, Tcl_NewIntObj(SIGRTMIN));
    return TCL_OK;
}


static
int
TopicCmd_Sigrtmax (
    ClientData clientData,
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *const objv[]
    )
{
    if (objc != 3) {
	Tcl_WrongNumArgs(interp, 2, objv, "sigrtmax");
	return TCL_ERROR;
    }

    Tcl_SetObjResult(interp, Tcl_NewIntObj(SIGRTMAX));
    return TCL_OK;
}


MODULE_SCOPE
int
Command_Info (
    ClientData clientData,
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *const objv[]
    )
{
    const char *topics[] = { "sigrtmin", "sigrtmax", NULL };
    Tcl_ObjCmdProc *const procs[] = {
	TopicCmd_Sigrtmin,
	TopicCmd_Sigrtmax
    };

    int topic;

    if (objc < 3) {
	Tcl_WrongNumArgs(interp, 2, objv, "topic ?arg ...?");
	return TCL_ERROR;
    }

    if (Tcl_GetIndexFromObj(interp, objv[2],
	    topics, "topic", 0, &topic) != TCL_OK) {
	return TCL_ERROR;
    }

    return procs[topic](clientData, interp, objc, objv);
}

/* vim: set ts=8 sts=4 sw=4 sts=4 noet: */
