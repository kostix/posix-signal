#include <tcl.h>
#include <assert.h>
#include "sigtables.h"
#include "events.h"
#include <stdio.h>

#define WORDKEY(KEY) ((char *) (KEY))

typedef struct {
    Tcl_Interp *interp;
    Tcl_Obj *cmdObj;
} PS_SignalHandler;

typedef struct {
    int initialized;
    int len;
    struct {
	Tcl_HashTable rt;
	PS_SignalHandler *std[];
    } items;
} PS_SignalHandlers;

static Tcl_ThreadDataKey handlersKey;

static
PS_SignalHandlers*
AllocHandlers (void)
{
    int len, nbytes;
    PS_SignalHandlers *handlersPtr;

    len = max_signum;
    nbytes = sizeof(*handlersPtr)
	    + sizeof(handlersPtr->items.std[0]) * len;

    handlersPtr = Tcl_GetThreadData(&handlersKey, nbytes);
    handlersPtr->len = len;

    return handlersPtr;
}

static
PS_SignalHandlers*
GetHandlers (void)
{
    return Tcl_GetThreadData(&handlersKey, 0);
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
HandleSignalEvent (
    Tcl_Event *evPtr,
    int flags
    )
{
    SignalEvent *sigEvPtr;
    PS_SignalHandlers *handlersPtr;
    PS_SignalHandler *handlerPtr;
    Tcl_Interp *interp;
    Tcl_Obj *cmdObj;
    int signum, code;

    sigEvPtr = (SignalEvent*) evPtr;
    printf("Rcvd %d in %x\n",
	    sigEvPtr->signum, sigEvPtr->threadId);

    signum = sigEvPtr->signum;
    handlersPtr = GetHandlers();
    if (signum <= max_signum) {
	/* Standard signal */
	handlerPtr = handlersPtr->items.std[SIGOFFSET(signum)];
    } else {
	/* Real-time signal */
	Tcl_HashEntry *entryPtr;
	entryPtr = Tcl_FindHashEntry(&handlersPtr->items.rt, WORDKEY(signum));
	handlerPtr = Tcl_GetHashValue(entryPtr);
    }

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
    return evPtr->proc == HandleSignalEvent;
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
	handlerPtr = handlersPtr->items.std[i];
	if (handlerPtr != NULL) {
	    FreeSignalHandler(handlerPtr);
	}
    }

    {
	Tcl_HashEntry *entryPtr;
	Tcl_HashSearch iterator;

	entryPtr = Tcl_FirstHashEntry(&handlersPtr->items.rt, &iterator);
	while (entryPtr != NULL) {
	    handlerPtr = Tcl_GetHashValue(entryPtr);
	    FreeSignalHandler(handlerPtr);
	    entryPtr = Tcl_NextHashEntry(&iterator);
	}
    }
}


MODULE_SCOPE
void
InitEventHandlers (void)
{
    int i;
    PS_SignalHandlers *handlersPtr;

    handlersPtr = AllocHandlers();
    if (handlersPtr->initialized) return;

    for (i = 0; i < handlersPtr->len; ++i) {
	handlersPtr->items.std[i] = NULL;
    }

    /* Create handlers for known signals */
    for (i = 0; i < nsigs; ++i) {
	int index = SIGOFFSET(signals[i].signal);
	handlersPtr->items.std[index] = CreateSignalHandler();
    }

    Tcl_InitHashTable(&handlersPtr->items.rt, TCL_ONE_WORD_KEYS);

    handlersPtr->initialized = 1;

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
    if (signum <= max_signum) {
	/* Standard signal */
	handlerPtr = handlersPtr->items.std[SIGOFFSET(signum)];
    } else {
	/* Real-time signal */
	Tcl_HashEntry *entryPtr;
	int isnew;
	entryPtr = Tcl_CreateHashEntry(&handlersPtr->items.rt,
		WORDKEY(signum), &isnew);
	if (isnew) {
	    handlerPtr = CreateSignalHandler();
	    Tcl_SetHashValue(entryPtr, handlerPtr);
	} else {
	    handlerPtr = Tcl_GetHashValue(entryPtr);
	}
    }

    handlerPtr->interp = interp;

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
    if (signum <= max_signum) {
	/* Standard signal */
	handlerPtr = handlersPtr->items.std[SIGOFFSET(signum)];
    } else {
	/* Real-time signal */
	Tcl_HashEntry *entryPtr;
	entryPtr = Tcl_FindHashEntry(&handlersPtr->items.rt, WORDKEY(signum));
	if (entryPtr != NULL) {
	    handlerPtr = Tcl_GetHashValue(entryPtr);
	} else {
	    handlerPtr = NULL;
	}
    }

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

    evPtr->event.proc = HandleSignalEvent;
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
