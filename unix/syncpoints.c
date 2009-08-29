#include <tcl.h>
#include <assert.h>
#include "sigtables.h"
#include "sigmap.h"
#include "syncpoints.h"
#include "sigmanip.h"
#include "events.h"
#include <stdio.h>

#define WORDKEY(KEY) ((char *) (KEY))

struct SyncPoint {
#ifdef TCL_THREADS
    Tcl_ThreadId threadId;
#endif
    int signum;
    int signaled;
    struct SyncPoint *nextPtr;
};

typedef struct SyncPoint SyncPoint;

typedef struct {
    SignalEvent *headPtr;
    SignalEvent *tailPtr;
} SignalEventList;

static SignalMap syncpoints;
TCL_DECLARE_MUTEX(spointsLock);

#ifdef TCL_THREADS
static Tcl_Condition spointsCV;
static int condReady;
#else
static Tcl_AsyncHandle activator;
#endif /* TCL_THREADS */

static SyncPoint * GetSyncPoint(int signum);

static
SyncPoint*
AllocSyncPoint (
    int signum)
{
    SyncPoint *spointPtr;

    spointPtr = (SyncPoint*) ckalloc(sizeof(*spointPtr));

#ifdef TCL_THREADS
    spointPtr->threadId = Tcl_GetCurrentThread();
#endif
    spointPtr->signum   = signum;
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

static
void
EmptyEventList (
    SignalEventList *listPtr
    )
{
    listPtr->headPtr = NULL;
    listPtr->tailPtr = NULL;
}

static
void
AddEventToList (
    SignalEventList *listPtr,
    SignalEvent *evPtr
    )
{
    if (listPtr->tailPtr == NULL) {
	listPtr->headPtr = evPtr;
    } else {
	listPtr->tailPtr->event.nextPtr = (Tcl_Event*) evPtr;
    }
    listPtr->tailPtr = evPtr;
    listPtr->tailPtr->event.nextPtr = NULL;
}

#ifdef TCL_THREADS
static
void
HarvestSyncpoint (
    SyncPoint *spointPtr,
    SignalEventList *evListPtr
    )
{
    SyncPoint *nextPtr;
    int level;

    for (level = 0; spointPtr != NULL; spointPtr = nextPtr, ++level) {
	int signaled = spointPtr->signaled;
	if (signaled) {
	    do {
		AddEventToList(evListPtr,
			CreateSignalEvent(spointPtr->threadId,
				spointPtr->signum));

		--signaled;
	    } while (signaled > 0);
	    spointPtr->signaled = 0;
	}

	nextPtr = spointPtr->nextPtr;
	if (level > 0) {
	    FreeSyncPoint(spointPtr);
	}
    }
}
#endif /* TCL_THREADS */

#ifdef TCL_THREADS
static
void
ManagerThreadProc (
    ClientData clientData
    )
{
    BlockAllSignals();

    while (1) {
	SignalEventList eventList;
	SignalMapSearch iterator;
	SyncPoint *spointPtr;

	Tcl_MutexLock(&spointsLock);

	while (condReady == 0) {
	    Tcl_ConditionWait(&spointsCV, &spointsLock, NULL);
	}
	condReady = 0;

	EmptyEventList(&eventList);

	spointPtr = FirstSigMapEntry(&syncpoints, &iterator);
	while (spointPtr != NULL) {
	    HarvestSyncpoint(spointPtr, &eventList);
	    spointPtr = NextSigMapEntry(&iterator);
	}

	Tcl_MutexUnlock(&spointsLock);

	{
	    SignalEvent *evPtr;
	    Tcl_ThreadId lastId;

	    evPtr = eventList.headPtr;
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
    InitSignalMap(&syncpoints);

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
int
SetSyncPoint (
    int signum
    )
{
    SignalMapEntry *entryPtr;
    SyncPoint *spointPtr;
    int isnew;

    entryPtr = CreateSigMapEntry(&syncpoints, signum, &isnew);
    spointPtr = GetSigMapValue(entryPtr);
    if (spointPtr == NULL) {
	spointPtr = AllocSyncPoint(signum);
	SetSigMapValue(entryPtr, spointPtr);
    } else {
	if (spointPtr->signaled == 0) {
	    spointPtr->threadId = Tcl_GetCurrentThread();
	} else {
	    SyncPoint *nextPtr = spointPtr;
	    spointPtr = AllocSyncPoint(signum);
	    spointPtr->nextPtr = nextPtr;
	    SetSigMapValue(entryPtr, spointPtr);
	}
    }
    return isnew;
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

    spointPtr = GetSyncPoint(signum);

    if (spointPtr != NULL) {
	++spointPtr->signaled;
	WakeManagerThread();
    }
}

static
SyncPoint *
GetSyncPoint(
    int signum)
{
    SignalMapEntry *entryPtr;

    entryPtr = FindSigMapEntry(&syncpoints, signum);
    if (entryPtr != NULL) {
	return GetSigMapValue(entryPtr);
    } else {
	return NULL;
    }
}

/* vim: set ts=8 sts=4 sw=4 sts=4 noet: */
