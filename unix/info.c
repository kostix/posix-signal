#include <tcl.h>
#include <signal.h>
#include "sigtables.h"
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
	Tcl_WrongNumArgs(interp, 3, objv, NULL);
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
	Tcl_WrongNumArgs(interp, 3, objv, NULL);
	return TCL_ERROR;
    }

    Tcl_SetObjResult(interp, Tcl_NewIntObj(SIGRTMAX));
    return TCL_OK;
}


static
int
TopicCmd_Signals (
    ClientData clientData,
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *const objv[]
    )
{
    Tcl_Obj *listObj;
    int i;

    if (objc != 3) {
	Tcl_WrongNumArgs(interp, 3, objv, NULL);
	return TCL_ERROR;
    }

    listObj = Tcl_NewListObj(0, NULL);
    for (i = 0; i < nsigs; ++i) {
	/* TODO instead of a string object, there should
	 * possibly be an object of our custom type
	 * which caches the signal number */
	Tcl_ListObjAppendElement(interp, listObj,
		Tcl_NewStringObj(signals[i].name, -1));
	Tcl_ListObjAppendElement(interp, listObj,
		Tcl_NewIntObj(signals[i].signal));
    }

    Tcl_SetObjResult(interp, listObj);
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
    const char *topics[] = { "sigrtmin", "sigrtmax", "signals", NULL };
    Tcl_ObjCmdProc *const procs[] = {
	TopicCmd_Sigrtmin,
	TopicCmd_Sigrtmax,
	TopicCmd_Signals
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
