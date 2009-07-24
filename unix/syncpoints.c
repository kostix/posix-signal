#include <tcl.h>
#include "sigtables.h"
#include "syncpoints.h"
#include "events.h"
#include <stdio.h>

struct SyncPoint {
#ifdef TCL_THREADS
    Tcl_ThreadId threadId;
#endif
    int signaled;
    struct SyncPoint *nextPtr;
};

typedef struct SyncPoint SyncPoint;

static SyncPoint **syncpoints;
TCL_DECLARE_MUTEX(spointsLock);

#ifdef TCL_THREADS
static Tcl_Condition spointsCV;
static int condReady;
#else
static Tcl_AsyncHandle activator;
#endif /* TCL_THREADS */

static
SyncPoint*
CreateSyncPoint (void)
{
    SyncPoint *spointPtr;

    spointPtr = (SyncPoint*) ckalloc(sizeof(*spointPtr));

#ifdef TCL_THREADS
    spointPtr->threadId = Tcl_GetCurrentThread();
#endif
    spointPtr->signaled = 0;
    spointPtr->nextPtr  = NULL;

    return spointPtr;
}

static
void
FreeSyncPoint (
    SyncPoint *spointPtr
    )
{
    ckfree((char*) spointPtr);
}

static
void
WakeManagerThread (void)
{
#ifdef TCL_THREADS
    condReady = 1;
    Tcl_ConditionNotify(&spointsCV);
#else
    Tcl_AsyncMark(activator);
#endif
}

#if 0
#ifdef TCL_THREADS
static
SignalEvent*
HarvestSyncpoint (
    SyncPoint *spointPtr
    )
{
    SignalEvent *headEvPtr, *tailEvPtr;
    SyncPoint *nextPtr;
    int signaled;

    headEvPtr = tailEvPtr = NULL;

    if (signaled) {
	do {
	    SignalEvent *evPtr;

	    /* TODO elaborate on i + 1 */
	    evPtr = CreateSignalEvent(spointPtr->threadId,
		    i + 1);

	    if (tailEvPtr == NULL) {
		headEvPtr = evPtr;
	    } else {
		tailEvPtr->event.nextPtr = evPtr;
	    }
	    tailEvPtr = evPtr;
	    tailEvPtr->event.nextPtr = NULL;
	    --signaled;
	} while (signaled > 0);
	spointPtr->signaled = 0;
    }

    nextPtr = spointPtr->nextPtr;
    if (nextPtr != NULL) {
	SignalEvent *contEvPtr;

	contEvPtr = HarvestSyncpoint(nextPtr);
	if (contEvPtr != NULL) {
	    /* FIXME at this point tailEvPtr can easily
	     * be NULL; we should check for this,
	     * and in general it seems worthy to factor
	     * out this messing with queue pointers */
	    tailEvPtr->event.nextPtr = contEvPtr;
	    tailEvPtr = contEvPtr;
	}

	FreeSyncPoint(nextPtr);
    }

    return headEvPtr;
}
#endif /* TCL_THREADS */
#endif

#ifdef TCL_THREADS
static
void
ManagerThreadProc (
    ClientData clientData
    )
{
    /* TODO block all signals */

    while (1) {
	int i;
	SignalEvent *headEvPtr, *tailEvPtr;

	Tcl_MutexLock(&spointsLock);

	while (condReady == 0) {
	    Tcl_ConditionWait(&spointsCV, &spointsLock, NULL);
	}
	condReady = 0;

	headEvPtr = tailEvPtr = NULL;

	/* TODO keep the length of the syncpoints table
	 * in a variable and use it here */
	for (i = 0; i < SIGOFFSET(max_signum); ++i) {
	    SyncPoint *spointPtr;
	    int signaled;

	    spointPtr = syncpoints[i];
	    if (spointPtr == NULL) continue;
	    
	    signaled = spointPtr->signaled;
	    if (signaled > 0) {
		do {
		    SignalEvent *evPtr;

		    /* TODO elaborate on i + 1 */
		    evPtr = CreateSignalEvent(spointPtr->threadId,
			    i + 1);

		    if (tailEvPtr == NULL) {
			headEvPtr = evPtr;
		    } else {
			tailEvPtr->event.nextPtr = evPtr;
		    }
		    tailEvPtr = evPtr;
		    tailEvPtr->event.nextPtr = NULL;

		    --signaled;
		} while (signaled > 0);
		spointPtr->signaled = 0;
	    }
	}

	Tcl_MutexUnlock(&spointsLock);

	{
	    SignalEvent *evPtr;
	    Tcl_ThreadId lastId;

	    evPtr = headEvPtr;
	    lastId = evPtr->threadId;
	    do {
		SignalEvent *nextEvPtr;
		Tcl_ThreadId threadId;

		nextEvPtr = evPtr->event.nextPtr;
		threadId = evPtr->threadId;
		Tcl_ThreadQueueEvent(threadId,
			(Tcl_Event*) evPtr, TCL_QUEUE_TAIL);
		if (threadId != lastId) {
		    Tcl_ThreadAlert(lastId);
		}
		printf("Sent %d to %x\n",
			evPtr->signum, evPtr->threadId);
		evPtr = nextEvPtr;
		lastId = threadId;
	    } while (evPtr != NULL);
	    Tcl_ThreadAlert(lastId);
	}
    }
}
#endif /* TCL_THREADS */

MODULE_SCOPE
void
_LockSyncPoints (void)
{
    Tcl_MutexLock(&spointsLock);
}

MODULE_SCOPE
void
_UnlockSyncPoints (void)
{
    Tcl_MutexUnlock(&spointsLock);
}

MODULE_SCOPE
void
InitSyncPoints (void)
{
    int i, len;

    len = SIGOFFSET(max_signum);
    syncpoints = (SyncPoint**) ckalloc(sizeof(SyncPoint*) * len);

    for (i = 0; i < len; ++i) {
	syncpoints[i] = NULL;
    }

#ifdef TCL_THREADS
    {
	Tcl_ThreadId threadId;
	int res;

	res = Tcl_CreateThread(&threadId, &ManagerThreadProc,
		NULL,
		TCL_THREAD_STACK_DEFAULT,
		TCL_THREAD_NOFLAGS);
	if (res != 0) {
	    Tcl_Panic(PACKAGE_NAME ": failed to create manager thread");
	}
    }
#endif /* TCL_THREADS */
}

/* TODO essentially, creation of syncpoint includes
 * "resetting" it to current values.
 * Looks like we should create one special procedure
 * for this
 */
MODULE_SCOPE
void
SetSyncPoint (
    int signum
    )
{
    SyncPoint *spointPtr;

    const int index = SIGOFFSET(signum);

    spointPtr = syncpoints[index];
    if (spointPtr == NULL) {
	syncpoints[index] = CreateSyncPoint();
    } else {
	int signaled = spointPtr->signaled;
	if (signaled) {
	    SyncPoint *oldPtr = spointPtr;
	    spointPtr = CreateSyncPoint();
	    spointPtr->nextPtr = oldPtr;
	} else {
	    spointPtr->threadId = Tcl_GetCurrentThread();
	    spointPtr->signaled = 0;
	}
    }
}

/* TODO possibly we should panic if spointPtr == NULL
 * as this means we told the system we do handle the signal
 * but actually fail to do so.
 * On the other hand, queueing of RT signals should be
 * considered: if the system drops the queue of pending
 * signals if we change the disposition of a signal to
 * SIG_DFL, or it's possible to clear it by hand, this can
 * work; otherwise pending signals with default action
 * of Term will kill our application */
MODULE_SCOPE
void
SignalSyncPoint (
    int signum
    )
{
    SyncPoint *spointPtr;

    spointPtr = syncpoints[SIGOFFSET(signum)];
    if (spointPtr != NULL) {
	++spointPtr->signaled;
	WakeManagerThread();
    }
}

/* vim: set ts=8 sts=4 sw=4 sts=4 noet: */
