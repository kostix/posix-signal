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
    ClientData clientData;
    struct SyncPoint *nextPtr;
};

typedef struct SyncPoint SyncPoint;

static SignalMap syncpoints;
static Queue danglingSpoints;
static int signalingEnabled = 0;
TCL_DECLARE_MUTEX(spointsLock);

#ifdef TCL_THREADS
static Tcl_Condition spointsCV;
static int threadReady;
static int condReady;
static int shutdownRequested = 0;
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
    int signum,
    ClientData clientData)
{
    SyncPoint *spointPtr;

    spointPtr = (SyncPoint*) ckalloc(sizeof(*spointPtr));

#ifdef TCL_THREADS
    spointPtr->threadId = Tcl_GetCurrentThread();
#endif
    spointPtr->signum     = signum;
    spointPtr->signaled   = 0;
    spointPtr->clientData = clientData;
    spointPtr->nextPtr    = NULL;

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

    Tcl_MutexLock(&spointsLock);

    /* Notify creator thread we're ready */
    threadReady = 1;
    Tcl_ConditionNotify(&spointsCV);

    while (1) {
	Queue eventQueue;
	SignalMapSearch iterator;
	SyncPoint *spointPtr;

	while (condReady == 0) {
	    Tcl_ConditionWait(&spointsCV, &spointsLock, NULL);
	}
	condReady = 0;

	if (shutdownRequested) {
	    break;
	}

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

	Tcl_MutexLock(&spointsLock);
    }

    /* Notify creator thread we're finished.
     * Note that the spointsLock is held at this point. */
    threadReady = 1;
    Tcl_ConditionNotify(&spointsCV);
    Tcl_MutexUnlock(&spointsLock);
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

/* FIXME in theory, we must not serve any signals
 * while running this block to avoid double locks
 * on spointsLock. But as this function is supposedly
 * only run when the package is being initialized
 * for the first time, and all the threads loading
 * this package are synchronized over its
 * common initialization part, we can possibly assume
 * no signals will be served by this thread while
 * it executes this function */

/*
 * Assume the mutex spointsLock is held
 */
/* FIXME also it's interesting whether the signal
 * mask is inherited by the created threads.
 * If this is true, we could call BlockAllSignals()
 * here and get the syncpoints manager thread
 * inherit this disposition, and the simply
 * call UnblockAllSignals() when the manager thread
 * reports back */
#ifdef TCL_THREADS
static
void
CreateManagerThread (void)
{
    Tcl_ThreadId threadId;
    int res;

    threadReady = 0;
    res = Tcl_CreateThread(&threadId, &ManagerThreadProc,
	    NULL,
	    TCL_THREAD_STACK_DEFAULT,
	    TCL_THREAD_NOFLAGS);
    if (res != 0) {
	Tcl_Panic(PACKAGE_NAME ": failed to create manager thread");
    }

    /* Wait for the manager thread to report back */
    while (threadReady == 0) {
	Tcl_ConditionWait(&spointsCV, &spointsLock, NULL);
    }
    threadReady = 0;
}
#endif /* TCL_THREADS */

/*
 * Assume the mutex spointsLock is held
 */
#ifdef TCL_THREADS
static
void
ShutdownManagerThread (void)
{
    /* Request the manager thread to terminate */
    shutdownRequested = 1;
    condReady = 1;
    Tcl_ConditionNotify(&spointsCV);

    /* Wait for the manager thread to report back it's finished.
     * Note that Tcl_ConditionWait unlocks spointsLock
     * before starting to wait on spointsCV
     * and locks it again before returning */
    threadReady = 0;
    while (threadReady == 0) {
	Tcl_ConditionWait(&spointsCV, &spointsLock, NULL);
    }
}
#endif /* TCL_THREADS */

void
EnableSyncpoints (void)
{
#ifdef TCL_THREADS
    CreateManagerThread();
#else
    Tcl_AsyncCreate(activator);
#endif
    signalingEnabled = 1;
}

void
DisableSyncpoints (void)
{
    signalingEnabled = 0;
#ifdef TCL_THREADS
    ShutdownManagerThread();
#else
    Tcl_AsyncDestroy(activator);
#endif
}

MODULE_SCOPE
void
InitSyncPoints (void)
{
    InitSignalMap(&syncpoints);

    InitSyncPointQueue(&danglingSpoints);
}

void
FinalizeSyncpoints (void)
{
    /* TODO free any memory allocated by InitSyncPoints().
     * With the current style of finalization
     * we can possibly assume no signals are trapped
     * at the time this function is called
     * and the syncpoints manager thread is not present,
     * so we're free to mess with the memory allocated
     * by this module. */
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
    ClientData clientData,
    int *isnewPtr)
{
    SignalMapEntry *entryPtr;
    SyncPoint *spointPtr;

    entryPtr = CreateSigMapEntry(&syncpoints, signum, isnewPtr);

    if (*isnewPtr) {
	spointPtr = AllocSyncPoint(signum, clientData);
	SetSigMapValue(entryPtr, spointPtr);
    } else {
	Tcl_ThreadId thisThreadId;
	spointPtr = GetSigMapValue(entryPtr);
	thisThreadId = Tcl_GetCurrentThread();
	if (spointPtr->signaled == 0) {
	    spointPtr->threadId   = thisThreadId;
	    spointPtr->clientData = clientData;
	} else {
	    if (spointPtr->threadId != thisThreadId) {
		SyncPoint *newPtr;
		QueuePush(&danglingSpoints, spointPtr);
		newPtr = AllocSyncPoint(signum, clientData);
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

    if (!signalingEnabled) return;

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
