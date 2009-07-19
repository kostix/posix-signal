#include <tcl.h>
#include "sigtables.h"
#include "syncpoints.h"

typedef struct {
#ifdef TCL_THREADS
    Tcl_ThreadId threadId;
#endif
    int signaled;
} SyncPoint;

static SyncPoint **syncpoints;
TCL_DECLARE_MUTEX(spointsLock);

#ifdef TCL_THREADS
static Tcl_Condition spointsCV;
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
    Tcl_ConditionNotify(&spointsCV);
#else
    Tcl_AsyncMark(activator);
#endif
}

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
    Tcl_ConditionNotify(&spointsCV);
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
	spointPtr->threadId = Tcl_GetCurrentThread();
	spointPtr->signaled = 0;
    }
}

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
