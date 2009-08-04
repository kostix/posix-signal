#include <tcl.h>
#include <signal.h>
#include "sigtables.h"
#include "sigobj.h"
#include "info.h"


static
int
GetSIGRTMIN (void)
{
    return SIGRTMIN;
}

static
int
GetSIGRTMAX (void)
{
    return SIGRTMAX;
}


static
int
CalcRTSignal (
    int base,
    int offset
    )
{
    int signum = base + offset;

    if (SIGRTMIN <= signum && signum <= SIGRTMAX) {
	return signum;
    } else {
	return -1;
    }
}


static
int
Info_RTSignal (
    ClientData clientData,
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *const objv[],
    int (*getRTSigProc) (void)
    )
{
    int res, offset, signum;

    switch (objc) {
	case 3:
	    Tcl_SetObjResult(interp, Tcl_NewIntObj(getRTSigProc()));
	    return TCL_OK;
	case 4:
	    res = Tcl_GetIntFromObj(interp, objv[3], &offset);
	    if (res != TCL_OK) {
		return TCL_ERROR;
	    }
	    signum = CalcRTSignal(getRTSigProc(), offset);
	    if (signum != -1) {
		Tcl_SetObjResult(interp, Tcl_NewIntObj(signum));
		return TCL_OK;
	    } else {
		Tcl_SetObjResult(interp,
			Tcl_NewStringObj("invalid signum", -1));
		return TCL_ERROR;
	    }
	default:
	    Tcl_WrongNumArgs(interp, 3, objv, NULL);
	    return TCL_ERROR;
    }

}


static
int
TopicCmd_Sigrtmin (
    ClientData clientData,
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *const objv[]
    )
{
    return Info_RTSignal(clientData, interp, objc, objv, GetSIGRTMIN);
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
    return Info_RTSignal(clientData, interp, objc, objv, GetSIGRTMAX);
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


static
int
TopicCmd_Name (
    ClientData clientData,
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *const objv[]
    )
{
    int res, signum, len;
    const char *namePtr;

    if (objc != 4) {
	Tcl_WrongNumArgs(interp, 3, objv, "signum");
	return TCL_ERROR;
    }

    res = Tcl_GetIntFromObj(interp, objv[3], &signum);
    if (res == TCL_OK) {
	namePtr = GetNameBySignum(interp, signum, &len);
    } else {
	namePtr = NULL;
    }

    if (namePtr != NULL) {
	Tcl_SetObjResult(interp, Tcl_NewStringObj(namePtr, len));
	return TCL_OK;
    } else {
	return TCL_ERROR;
    }
}


static
int
TopicCmd_Signum (
    ClientData clientData,
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *const objv[]
    )
{
    const char *namePtr;
    int signum;

    if (objc != 4) {
	Tcl_WrongNumArgs(interp, 3, objv, "name");
	return TCL_ERROR;
    }

    namePtr = Tcl_GetStringFromObj(objv[3], NULL);
    signum  = GetSignumByName(interp, namePtr);
    if (signum != -1) {
	Tcl_SetObjResult(interp, Tcl_NewIntObj(signum));
	return TCL_OK;
    } else {
	return TCL_ERROR;
    }
}


static
int
TopicCmd_Exists (
    ClientData clientData,
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *const objv[]
    )
{
    int exists;

    if (objc != 4) {
	Tcl_WrongNumArgs(interp, 3, objv, "signal");
	return TCL_ERROR;
    }

    exists = GetSignumFromObj(NULL, objv[3]) != -1;

    Tcl_SetObjResult(interp, Tcl_NewBooleanObj(exists));
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
    const char *topics[] = { "sigrtmin", "sigrtmax", "signals",
	    "name", "signum", "exists", NULL };
    Tcl_ObjCmdProc *const procs[] = {
	TopicCmd_Sigrtmin,
	TopicCmd_Sigrtmax,
	TopicCmd_Signals,
	TopicCmd_Name,
	TopicCmd_Signum,
	TopicCmd_Exists
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
