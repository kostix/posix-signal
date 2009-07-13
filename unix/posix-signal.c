#include <tcl.h>

static
int
Command_Trap (
	ClientData clientData,
	Tcl_Interp *interp,
	int objc,
	Tcl_Obj *const objv[]
	)
{
	return TCL_OK;
}

int
Signal_Command (
	ClientData clientData,
	Tcl_Interp *interp,
	int objc,
	Tcl_Obj *const objv[]
	)
{
	const char *cmds[] = { "trap", NULL };
	Tcl_ObjCmdProc *const procs[] = {
		Command_Trap
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

EXTERN int
PosixSignal_Init(Tcl_Interp * interp)
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

	Tcl_CreateObjCommand(interp, "signal",
			Signal_Command, clientData, NULL);

	if (Tcl_PkgProvide(interp, PACKAGE_NAME, PACKAGE_VERSION) != TCL_OK) {
		return TCL_ERROR;
	}

	return TCL_OK;
}

