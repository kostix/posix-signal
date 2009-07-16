#include <tcl.h>
#include <signal.h>
#include <assert.h>

#define SIGDECL(SIG) { SIG, #SIG }

#define SIGOFFSET(SIG) ((SIG) - 1)

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

const int nsigs = sizeof(signals)/sizeof(signals[0]);

static
void
InitSignalLookupTables (void) {
    int i;
    for (i = 0; i < sizeof(signals)/sizeof(signals[0]); ++i) {
	printf("%s %d\n", signals[i].name, signals[i].signal);
    }
}

typedef struct {
    Tcl_Obj *cmdObj;
} PS_SignalHandler;

typedef struct {
    PS_SignalHandler **items;
    int len;
} PS_SignalHandlers;

static PS_SignalHandlers handlers;

static
PS_SignalHandler*
CreateSignalHandler (void)
{
    PS_SignalHandler *handlerPtr;
    Tcl_Obj *cmdObj;

    handlerPtr = (PS_SignalHandler*) ckalloc(sizeof(PS_SignalHandler));

    cmdObj = Tcl_NewObj();
    Tcl_IncrRefCount(cmdObj);
    handlerPtr->cmdObj = cmdObj;

    return handlerPtr;
}

static
void
InitSignalHandlers (void) {
    int i, max, len;

    /* Find maximal signal number */
    max = 0;
    for (i = 0; i < nsigs; ++i) {
	int sig = signals[i].signal;
	if (sig > max) {
	    max = sig;
	}
    }
    assert(max > 0);

    /* Create table for handlers
     * and initially set pointers to all handlers to NULL */

    len = SIGOFFSET(max);
    handlers.items = (PS_SignalHandler**) ckalloc(sizeof(handlers.items[0]) * len);
    handlers.len = len;

    for (i = 0; i < len; ++i) {
	handlers.items[i] = NULL;
    }

    /* Create handlers for known signals */
    for (i = 0; i < nsigs; ++i) {
	int index = SIGOFFSET(signals[i].signal);
	handlers.items[index] = CreateSignalHandler();
    }	
}

static
PS_SignalHandler*
GetHandlerBySignal (
    int signal
    )
{
    int index = SIGOFFSET(signal);

    if (0 <= index && index < handlers.len) {
	return handlers.items[index];
    } else {
	return NULL;
    }
}


static const char **signames;

static
void
InitLookupTable (void)
{
    int i;

    const int len = nsigs + 1; // account for the terminating NULL entry

    signames = (const char**) ckalloc(sizeof(signames[0]) * len);

    for (i = 0; i < nsigs; ++i) {
	signames[i] = signals[i].name;
    }
    signames[nsigs] = NULL;
}

static
int
GetSignalIdFromObj (
    Tcl_Interp *interp,
    Tcl_Obj *nameObj
    )
{
    int res, index;

    res = Tcl_GetIndexFromObj(interp, nameObj, signames,
	    "signal name", 0, &index);
    if (res == TCL_OK) {
	return signals[index].signal;
    } else {
	return -1;
    }
}

static
int
TrapSet (
    ClientData clientData,
    Tcl_Interp *interp,
    Tcl_Obj *sigObj,
    Tcl_Obj *cmdObj
    )
{
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

    id = GetSignalIdFromObj(interp, sigObj);
    if (id == -1) {
	return TCL_ERROR;
    }

    return TCL_OK;
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

static
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

    InitSignalLookupTables();
    InitSignalHandlers();
    InitLookupTable();

    Tcl_CreateObjCommand(interp, PACKAGE_NAME,
	    Signal_Command, clientData, NULL);

    if (Tcl_PkgProvide(interp, PACKAGE_NAME, PACKAGE_VERSION) != TCL_OK) {
	return TCL_ERROR;
    }

    return TCL_OK;
}

/* vim: set ts=8 sts=4 sw=4 sts=4 noet: */
