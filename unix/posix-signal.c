#include <tcl.h>
#include <signal.h>
#include <assert.h>
#include <stdio.h>
#include "sigtables.h"
#include "syncpoints.h"
#include "events.h"
#include "trap.h"
#include "send.h"

static
int
Signal_Command (
    ClientData clientData,
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *const objv[]
	)
{
    const char *cmds[] = { "trap", "send", NULL };
    Tcl_ObjCmdProc *const procs[] = {
	Command_Trap,
	Command_Send
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

int
Posixsignal_Init(Tcl_Interp * interp)
{
    ClientData clientData;

#ifdef USE_TCL_STUBS
    if (Tcl_InitStubs(interp, "8.1", 0) == NULL) {
	return TCL_ERROR;
    }
#endif
    if (Tcl_PkgRequire(interp, "Tcl", "8.1", 0) == NULL) {
	return TCL_ERROR;
    }

    /* TODO protect by a package-global mutex,
     * must be initialized once per process */
    InitSignalTables();
    InitSyncPoints();

    /* This initializer must be run each time the package
     * is loaded */
    InitEventHandlers();

    Tcl_CreateObjCommand(interp, PACKAGE_NAME,
	    Signal_Command, clientData, NULL);

    if (Tcl_PkgProvide(interp, PACKAGE_NAME, PACKAGE_VERSION) != TCL_OK) {
	return TCL_ERROR;
    }

    return TCL_OK;
}

/* vim: set ts=8 sts=4 sw=4 sts=4 noet: */
