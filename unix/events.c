#include <tcl.h>
#include <assert.h>
#include "sigtables.h"
#include "events.h"

typedef struct {
    Tcl_ThreadId threadId;
    Tcl_Interp *interp;
    Tcl_Obj *cmdObj;
} PS_SignalHandler;

typedef struct {
    PS_SignalHandler **items;
    int len;
} PS_SignalHandlers;

static PS_SignalHandlers handlers;
TCL_DECLARE_MUTEX(handlersLock);

#ifdef TCL_THREADS
MODULE_SCOPE
void
_LockEventHandlers (void)
{
    Tcl_MutexLock(&handlersLock);
}
#endif

#ifdef TCL_THREADS
MODULE_SCOPE
void
_UnlockEventHandlers (void)
{
    Tcl_MutexUnlock(&handlersLock);
}
#endif

/* NOTE this struct will possibly be a part of the
 * public API (and stubs), so it possibly must not
 * have #ifdef'ed parts. Therefore, we include
 * the threadId field event for non-threaded builds,
 * in which it is to be ignored */
typedef struct {
    Tcl_Event event;
    Tcl_ThreadId threadId;
    int signum;
} SignalEvent;

static
PS_SignalHandler*
CreateSignalHandler (void)
{
    PS_SignalHandler *handlerPtr;
    Tcl_Obj *cmdObj;

    handlerPtr = (PS_SignalHandler*) ckalloc(sizeof(PS_SignalHandler));

    handlerPtr->threadId = Tcl_GetCurrentThread();
    handlerPtr->interp = NULL;

    cmdObj = Tcl_NewObj();
    Tcl_IncrRefCount(cmdObj);
    handlerPtr->cmdObj = cmdObj;

    return handlerPtr;
}


static
int
SignalEventHandler (
    Tcl_Event *evPtr,
    int flags
    )
{
    SignalEvent *sigEvPtr;
    PS_SignalHandler *handlerPtr;
    Tcl_Interp *interp;
    Tcl_Obj *cmdObj;
    int valid, code;

    sigEvPtr = (SignalEvent*) evPtr;

    LockEventHandlers();
    handlerPtr = handlers.items[SIGOFFSET(sigEvPtr->signum)];
    valid = handlerPtr->threadId == sigEvPtr->threadId;
    if (valid) {
	interp = handlerPtr->interp;
	cmdObj = handlerPtr->cmdObj;
    }
    UnlockEventHandlers();

    if (valid) {
	code = Tcl_GlobalEvalObj(interp, cmdObj);
	if (code == TCL_ERROR) {
	    Tcl_BackgroundError(interp);
	}
    }

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


MODULE_SCOPE
void
InitEventHandlers (void)
{
    int i, len, nbytes;

    len = SIGOFFSET(max_signum);
    nbytes = sizeof(handlers.items[0]) * len;
    handlers.items = (PS_SignalHandler**) ckalloc(nbytes);
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
    PS_SignalHandler *handlerPtr;
    Tcl_ThreadId threadId;

    handlerPtr = handlers.items[SIGOFFSET(signum)];

    Tcl_DecrRefCount(handlerPtr->cmdObj);

    threadId = Tcl_GetCurrentThread();
    if (handlerPtr->threadId != threadId) {
	newCmdObj = Tcl_DuplicateObj(newCmdObj);
	handlerPtr->threadId = threadId;
	
    }

    Tcl_IncrRefCount(newCmdObj);
    handlerPtr->cmdObj = newCmdObj;
}


MODULE_SCOPE
Tcl_Obj*
GetEventHandlerCommand (
    int signum
    )
{
    PS_SignalHandler *handlerPtr;

    handlerPtr = handlers.items[SIGOFFSET(signum)];

    if (handlerPtr == NULL) {
	return NULL;
    } else {
	Tcl_ThreadId threadId = Tcl_GetCurrentThread();
	if (handlerPtr->threadId == threadId) {
	    return handlerPtr->cmdObj;
	} else {
	    return Tcl_DuplicateObj(handlerPtr->cmdObj);
	}
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
