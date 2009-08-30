#include <tcl.h>
#include <assert.h>
#include "sigtables.h"
#include "sigmap.h"
#include "syncpoints.h"
#include "sigmanip.h"
#include "events.h"
#include "queue.h"
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

static SignalMap syncpoints;
static Queue danglingSpoints;
TCL_DECLARE_MUTEX(spointsLock);

#ifdef TCL_THREADS
static Tcl_Condition spointsCV;
static int condReady;
#else
static Tcl_AsyncHandle activator;
#endif /* TCL_THREADS */

static SyncPoint * GetSyncPoint(int signum);

static void SetNextSyncPoint (QueueEntry entry, QueueEntry nextEntry);
static QueueEntry GetNextSyncPoint (QueueEntry entry);
static void InitSyncPointQueue (Queue *queuePtr);

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
SetNextEvent (
    QueueEntry entry,
    QueueEntry nextEntry)
{
    SignalEvent *thisPtr, *nextPtr;

    thisPtr = entry;
    nextPtr = nextEntry;

    thisPtr->event.nextPtr = (Tcl_Event *) nextPtr;
}

static
QueueEntry
GetNextEvent (
    QueueEntry entry)
{
    SignalEvent *eventPtr;

    eventPtr = entry;

    return (QueueEntry) eventPtr->event.nextPtr;
}

static
void
InitEventList (
    Queue *queuePtr
    )
{
    InitQueue(queuePtr, SetNextEvent, GetNextEvent);
}

#ifdef TCL_THREADS
static
void
HarvestSyncpoint (
    SyncPoint *spointPtr,
    Queue *queuePtr
    )
{
    int signaled = spointPtr->signaled;
    if (signaled) {
	do {
	    QueuePush(queuePtr,
		    CreateSignalEvent(spointPtr->threadId,
			    spointPtr->signum));

	    --signaled;
	} while (signaled > 0);
	spointPtr->signaled = 0;
    }
}
#endif /* TCL_THREADS */

#ifdef TCL_THREADS
static
void
HarvestDanglingSyncpoints (
    Queue *spointsQueuePtr,
    Queue *eventsQueuePtr)
{
    SyncPoint *spointPtr;

    spointPtr = QueuePop(spointsQueuePtr);
    while (spointPtr != NULL) {
	HarvestSyncpoint(spointPtr, eventsQueuePtr);
	FreeSyncPoint(spointPtr);
	spointPtr = QueuePop(spointsQueuePtr);
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
	Queue eventQueue;
	SignalMapSearch iterator;
	SyncPoint *spointPtr;

	Tcl_MutexLock(&spointsLock);

	while (condReady == 0) {
	    Tcl_ConditionWait(&spointsCV, &spointsLock, NULL);
	}
	condReady = 0;

	InitEventList(&eventQueue);

	HarvestDanglingSyncpoints(&danglingSpoints, &eventQueue);

	spointPtr = FirstSigMapEntry(&syncpoints, &iterator);
	while (spointPtr != NULL) {
	    HarvestSyncpoint(spointPtr, &eventQueue);
	    spointPtr = NextSigMapEntry(&iterator);
	}

	Tcl_MutexUnlock(&spointsLock);

	{
	    SignalEvent *evPtr;
	    Tcl_ThreadId lastId;

	    evPtr = QueuePop(&eventQueue);
	    lastId = evPtr->threadId;
	    do {
		SignalEvent *nextEvPtr;
		Tcl_ThreadId threadId;

		nextEvPtr = QueuePop(&eventQueue);
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

    InitSyncPointQueue(&danglingSpoints);

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

SyncPointMapEntry
FindSyncPoint (
    int signum)
{
    return FindSigMapEntry(&syncpoints, signum);
}

SyncPointMapEntry
AcquireSyncPoint (
    int signum,
    int *isnewPtr)
{
    SignalMapEntry *entryPtr;
    SyncPoint *spointPtr;

    entryPtr = CreateSigMapEntry(&syncpoints, signum, isnewPtr);

    if (*isnewPtr) {
	spointPtr = AllocSyncPoint(signum);
	SetSigMapValue(entryPtr, spointPtr);
    } else {
	Tcl_ThreadId thisThreadId;
	spointPtr = GetSigMapValue(entryPtr);
	thisThreadId = Tcl_GetCurrentThread();
	if (spointPtr->signaled == 0) {
	    spointPtr->threadId = thisThreadId;
	} else {
	    if (spointPtr->threadId != thisThreadId) {
		SyncPoint *newPtr;
		QueuePush(&danglingSpoints, spointPtr);
		newPtr = AllocSyncPoint(signum);
		SetSigMapValue(entryPtr, newPtr);
	    } else {
		/* Do nothing -- the syncpoint is already ours */
	    }
	}
    }

    return entryPtr;
}

void
DeleteSyncPoint (
    SyncPointMapEntry entry)
{
    SyncPoint *spointPtr;

    spointPtr = GetSigMapValue(entry);
    if (spointPtr->signaled != 0) {
	if (spointPtr->threadId != Tcl_GetCurrentThread()) {
	    QueuePush(&danglingSpoints, spointPtr);
	    /* TODO notify the owner thread that it has just
	     * lost the syncpoint and should free any state
	     * associated with it */
	}
    }
    DeleteSigMapEntry(entry);
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

static
void
SetNextSyncPoint (
    QueueEntry entry,
    QueueEntry nextEntry)
{
    SyncPoint *thisPtr, *nextPtr;

    thisPtr = entry;
    nextPtr = nextEntry;

    thisPtr->nextPtr = nextPtr;
}

static
QueueEntry
GetNextSyncPoint (
    QueueEntry entry)
{
    SyncPoint *spointPtr;

    spointPtr = entry;

    return (QueueEntry) spointPtr->nextPtr;
}

static
void
InitSyncPointQueue (
    Queue *queuePtr
    )
{
    InitQueue(queuePtr, SetNextSyncPoint, GetNextSyncPoint);
}

/* vim: set ts=8 sts=4 sw=4 sts=4 noet: */
