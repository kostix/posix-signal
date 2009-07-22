#include <tcl.h>
#include <assert.h>
#include "sigtables.h"
#include "events.h"
#include <stdio.h>

typedef struct {
    Tcl_Interp *interp;
    Tcl_Obj *cmdObj;
} PS_SignalHandler;

typedef struct {
    int initialized;
    int len;
    PS_SignalHandler *items[];
} PS_SignalHandlers;

static Tcl_ThreadDataKey handlersKey;
static int handlersLen;

static
PS_SignalHandlers*
GetHandlers (void)
{
    /* FIXME this is a hack: by contract we should not
     * assume Tcl_ThreadDataKey is actually a pointer
     * which is filled during the first call to
     * Tcl_GetThreadData() */
    if (handlersKey == NULL) {
	int len = SIGOFFSET(max_signum);
	handlersLen = sizeof(PS_SignalHandlers)
		+ sizeof(PS_SignalHandler) * len;
    }
    return Tcl_GetThreadData(&handlersKey, handlersLen);
}

static
PS_SignalHandler*
CreateSignalHandler (void)
{
    PS_SignalHandler *handlerPtr;
    Tcl_Obj *cmdObj;

    handlerPtr = (PS_SignalHandler*) ckalloc(sizeof(PS_SignalHandler));

    handlerPtr->interp = NULL;

    cmdObj = Tcl_NewObj();
    Tcl_IncrRefCount(cmdObj);
    handlerPtr->cmdObj = cmdObj;

    return handlerPtr;
}

static
void
FreeSignalHandler (
    PS_SignalHandler *handlerPtr
    )
{
    Tcl_DecrRefCount(handlerPtr->cmdObj);

    ckfree((char*) handlerPtr);
}


static
int
SignalEventHandler (
    Tcl_Event *evPtr,
    int flags
    )
{
    SignalEvent *sigEvPtr;
    PS_SignalHandlers *handlersPtr;
    PS_SignalHandler *handlerPtr;
    Tcl_Interp *interp;
    Tcl_Obj *cmdObj;
    int code;

    sigEvPtr = (SignalEvent*) evPtr;
    printf("Rcvd %d in %x\n",
	    sigEvPtr->signum, sigEvPtr->threadId);

    handlersPtr = GetHandlers();
    handlerPtr = handlersPtr->items[SIGOFFSET(sigEvPtr->signum)];

    interp = handlerPtr->interp;
    cmdObj = handlerPtr->cmdObj;

    Tcl_IncrRefCount(cmdObj);
    code = Tcl_GlobalEvalObj(interp, cmdObj);
    if (code == TCL_ERROR) {
	Tcl_BackgroundError(interp);
    }
    Tcl_DecrRefCount(cmdObj);

    return 1;
}


static
int
DeleteEvent (
    Tcl_Event *evPtr,
    ClientData clientData
    )
{
    return evPtr->proc == SignalEventHandler;
}

static
void
FreeEventHandlers (
    ClientData clientData
    )
{
    int i, len;
    PS_SignalHandlers *handlersPtr;
    PS_SignalHandler *handlerPtr;

    handlersPtr = (PS_SignalHandlers*) clientData;

    len = handlersPtr->len;
    for (i = 0; i < len; ++i) {
	handlerPtr = handlersPtr->items[i];
	if (handlerPtr != NULL) {
	    FreeSignalHandler(handlerPtr);
	}
    }
}


MODULE_SCOPE
void
InitEventHandlers (void)
{
    int i, len;
    PS_SignalHandlers *handlersPtr;

    handlersPtr = GetHandlers();
    if (handlersPtr->initialized) return;

    /* TODO make sane --
     * currently we calculate this length twice:
     * once in GetHandlers() and once here */
    len = SIGOFFSET(max_signum);

    handlersPtr->initialized = 1;
    handlersPtr->len = len;

    for (i = 0; i < len; ++i) {
	handlersPtr->items[i] = NULL;
    }

    /* Create handlers for known signals */
    for (i = 0; i < nsigs; ++i) {
	int index = SIGOFFSET(signals[i].signal);
	handlersPtr->items[index] = CreateSignalHandler();
    }

    Tcl_CreateThreadExitHandler(FreeEventHandlers,
	    (ClientData) handlersPtr);
}


static
void
Original_SetEventHandler (
    PS_SignalHandler *handlerPtr,
    Tcl_Interp *interp,
    Tcl_Obj *newCmdObj
    )
{
    int len;
    Tcl_Obj *cmdObj;
    const char *newCmdPtr;
    PS_SignalHandlers *handlersPtr;

    handlersPtr = GetHandlers();

    cmdObj = handlerPtr->cmdObj;

    newCmdPtr = Tcl_GetStringFromObj(newCmdObj, &len);
    if (len == 0) {
	/* Remove bound script, if any */
	if (Tcl_IsShared(cmdObj)) {
	    Tcl_DecrRefCount(cmdObj);
	    cmdObj = Tcl_NewObj();
	    Tcl_IncrRefCount(cmdObj);
	} else {
	    Tcl_SetStringObj(cmdObj, "", 0);
	}
	handlerPtr->interp = NULL;
    } else {
	if (newCmdPtr[0] != '+') {
	    /* Owerwrite the script with the new one */
	    /* FIXME ideally we should check whether objv[3]
	    * is unshared and use it directly, if it is,
	    * to avoid copying of the script text */
	    if (Tcl_IsShared(cmdObj)) {
		Tcl_DecrRefCount(cmdObj);
		cmdObj = Tcl_DuplicateObj(newCmdObj);
		Tcl_IncrRefCount(cmdObj);
	    } else {
		Tcl_SetStringObj(cmdObj, newCmdPtr, len);
	    }
	} else {
	    /* Append to the script */
	    if (Tcl_IsShared(cmdObj)) {
		Tcl_DecrRefCount(cmdObj);
		cmdObj = Tcl_DuplicateObj(cmdObj);
		Tcl_IncrRefCount(cmdObj);
	    }
	    Tcl_AppendToObj(cmdObj, "\n", -1);
	    Tcl_AppendToObj(cmdObj, newCmdPtr + 1, len - 1);
	}
	handlerPtr->interp = interp;
    }
    handlerPtr->cmdObj = cmdObj;
}


MODULE_SCOPE
void
SetEventHandler (
    int signum,
    Tcl_Interp *interp,
    Tcl_Obj *newCmdObj
    )
{
    PS_SignalHandlers *handlersPtr;
    PS_SignalHandler *handlerPtr;

    handlersPtr = GetHandlers();
    handlerPtr = handlersPtr->items[SIGOFFSET(signum)];

    Tcl_DecrRefCount(handlerPtr->cmdObj);

    Tcl_IncrRefCount(newCmdObj);
    handlerPtr->cmdObj = newCmdObj;
}


MODULE_SCOPE
Tcl_Obj*
GetEventHandlerCommand (
    int signum
    )
{
    PS_SignalHandlers *handlersPtr;
    PS_SignalHandler *handlerPtr;

    handlersPtr = GetHandlers();
    handlerPtr = handlersPtr->items[SIGOFFSET(signum)];

    if (handlerPtr == NULL) {
	return NULL;
    } else {
	return handlerPtr->cmdObj;
    }
}


MODULE_SCOPE
SignalEvent*
CreateSignalEvent (
    Tcl_ThreadId threadId,
    int signum
    )
{
    SignalEvent *evPtr;

    evPtr = (SignalEvent*) ckalloc(sizeof(*evPtr));

    evPtr->event.proc = SignalEventHandler;
    evPtr->threadId = threadId;
    evPtr->signum = signum;

    return evPtr;
}


MODULE_SCOPE
void
DeleteThreadEvents (void)
{
    Tcl_DeleteEvents(DeleteEvent, NULL);
}

/* vim: set ts=8 sts=4 sw=4 sts=4 noet: */
