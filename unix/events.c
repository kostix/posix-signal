#include <tcl.h>
#include <assert.h>
#include "sigmap.h"
#include "events.h"
#include <stdio.h>

#define WORDKEY(KEY) ((char *) (KEY))

typedef struct {
    Tcl_Interp *interp;
    Tcl_Obj *cmdObj;
} EventHandler;

typedef struct {
    int initialized;
    SignalMap map;
} EventHandlers;

static Tcl_ThreadDataKey handlersKey;

static void DeleteThreadEvents (int signum);
static EventHandler * GetSignalHandler(int signum);

static
EventHandlers *
GetHandlers (void)
{
    return Tcl_GetThreadData(&handlersKey, sizeof(EventHandlers));
}

static
EventHandler*
CreateSignalHandler (void)
{
    EventHandler *handlerPtr;
    Tcl_Obj *cmdObj;

    handlerPtr = (EventHandler*) ckalloc(sizeof(EventHandler));

    handlerPtr->interp = NULL;

    cmdObj = Tcl_NewObj();
    Tcl_IncrRefCount(cmdObj);
    handlerPtr->cmdObj = cmdObj;

    return handlerPtr;
}

static
void
FreeSignalHandler (
    EventHandler *handlerPtr
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
    EventHandler *handlerPtr;
    Tcl_Interp *interp;
    Tcl_Obj *cmdObj;
    int signum, code;

    sigEvPtr = (SignalEvent*) evPtr;
    printf("Rcvd %d in %x\n",
	    sigEvPtr->signum, sigEvPtr->threadId);

    signum = sigEvPtr->signum;
    handlerPtr  = GetSignalHandler(signum);

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
IsOurEvent (
    Tcl_Event *evPtr,
    ClientData clientData
    )
{
    if (evPtr->proc == HandleSignalEvent) {
	SignalEvent *eventPtr;
	int *signumPtr;

	eventPtr  = (SignalEvent *) evPtr;
	signumPtr = (int *) clientData;

	return eventPtr->signum = *signumPtr;
    } else {
	return 0;
    }
}

static
void
FreeEventHandlers (
    ClientData clientData
    )
{
    EventHandlers *handlersPtr;
    EventHandler *handlerPtr;
    SignalMapSearch iterator;

    handlersPtr = (EventHandlers*) clientData;

    handlerPtr = FirstSigMapEntry(&handlersPtr->map, &iterator);
    while (handlerPtr != NULL) {
	FreeSignalHandler(handlerPtr);
	handlerPtr = NextSigMapEntry(&iterator);
    }
}


/* The idea behind this way of initialization is as follows.
 * Each thread can have any number of interps, each of which can
 * load this package. Loading of the package makes this function
 * run in the interp; therefore, it can be legitimately called
 * several times in the same thread while the table of event
 * handlers should be initialized only once in a given thread.
 * To provide for this, we rely on the fact the memory block
 * returned by GetHandlers() will be initialized to all zeroes
 * the first time it is created for a thread, and so the
 * "initialized" flag it contains will be reset.  If this flag
 * is not set after a call to GetHandlers(), then this is the
 * first time InitEventHandlers() is called in this thread, and
 * so we initialize the table of event handlers.  All subsequent
 * calls to this function in the same thread (supposedly made in
 * other interps) will have the "initialized" flag set in the
 * data returned by GetHandlers() and the initialization
 * will be skipped. */
MODULE_SCOPE
void
InitEventHandlers (void)
{
    EventHandlers *handlersPtr;

    handlersPtr = GetHandlers();
    if (!handlersPtr->initialized) {
	InitSignalMap(&handlersPtr->map);

	Tcl_CreateThreadExitHandler(FreeEventHandlers,
		(ClientData) handlersPtr);

	handlersPtr->initialized = 1;
    }
}


static
void
Original_SetEventHandler (
    EventHandler *handlerPtr,
    Tcl_Interp *interp,
    Tcl_Obj *newCmdObj
    )
{
    int len;
    Tcl_Obj *cmdObj;
    const char *newCmdPtr;
    EventHandlers *handlersPtr;

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
    EventHandlers *handlersPtr;
    EventHandler *handlerPtr;
    SignalMapEntry *entryPtr;
    int isnew;

    handlersPtr = GetHandlers();
    entryPtr = CreateSigMapEntry(&handlersPtr->map, signum, &isnew);
    if (isnew) {
	handlerPtr = CreateSignalHandler();
	SetSigMapValue(entryPtr, handlerPtr);
    } else {
	handlerPtr = GetSigMapValue(entryPtr);
    }

    handlerPtr->interp = interp;

    /* TODO implement appending of scripts to
     * existing commands */
    Tcl_DecrRefCount(handlerPtr->cmdObj);

    Tcl_IncrRefCount(newCmdObj);
    handlerPtr->cmdObj = newCmdObj;
}


MODULE_SCOPE
void
DeleteEventHandler (
    int signum
    )
{
    EventHandlers *handlersPtr;
    SignalMapEntry *entryPtr;

    handlersPtr = GetHandlers();
    entryPtr = FindSigMapEntry(&handlersPtr->map, signum);
    if (entryPtr != NULL) {
	DeleteThreadEvents(signum);
	DeleteSigMapEntry(entryPtr);
    }
}


MODULE_SCOPE
Tcl_Obj*
GetEventHandlerCommand (
    int signum
    )
{
    EventHandler *handlerPtr;

    handlerPtr  = GetSignalHandler(signum);
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


static
void
DeleteThreadEvents (
    int signum)
{
    Tcl_DeleteEvents(IsOurEvent, &signum);
}


static
EventHandler *
GetSignalHandler(
    int signum)
{
    EventHandlers *handlersPtr;
    SignalMapEntry *entryPtr;

    handlersPtr = GetHandlers();
    entryPtr = FindSigMapEntry(&handlersPtr->map, signum);
    if (entryPtr != NULL) {
	return GetSigMapValue(entryPtr);
    } else {
	return NULL;
    }
}

/* vim: set ts=8 sts=4 sw=4 sts=4 noet: */
