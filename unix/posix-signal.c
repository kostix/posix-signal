#include <tcl.h>
#include <signal.h>

#define SIGDECL(SIG) { SIG, #SIG }

const struct {
	int signal;
	char *name;
} signals[] = {
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
};

static
void
InitSignalLookupTables (void) {
	int i;
	for (i = 0; i < sizeof(signals)/sizeof(signals[0]); ++i) {
		printf("%s %d\n", signals[i].name, signals[i].signal);
	}
}

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

	InitSignalLookupTables();

	Tcl_CreateObjCommand(interp, PACKAGE_NAME,
			Signal_Command, clientData, NULL);

	if (Tcl_PkgProvide(interp, PACKAGE_NAME, PACKAGE_VERSION) != TCL_OK) {
		return TCL_ERROR;
	}

	return TCL_OK;
}

