#ifndef __POSIX_SIGNAL_SYNCPOINTS_H

#ifdef TCL_THREADS
void
_LockSyncPoints (void);
#define LockSyncPoints _LockSyncPoints
#else
#define LockSyncPoints do {} while (0)
#endif

#ifdef TCL_THREADS
void
_UnlockSyncPoints (void);
#define UnlockSyncPoints _UnlockSyncPoints
#else
#define UnlockSyncPoints do {} while (0)
#endif

void
InitSyncPoints (void);

void
SetSyncPoint (
    int signum
    );

void
SignalSyncPoint (
    int signum
    );

#define __POSIX_SIGNAL_SYNCPOINTS_H
#endif /* __POSIX_SIGNAL_SYNCPOINTS_H */

/* vim: set ts=8 sts=4 sw=4 sts=4 noet: */