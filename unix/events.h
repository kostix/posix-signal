#ifndef __POSIX_SIGNAL_EVENTS_H

#ifdef TCL_THREADS
void
_LockEventHandlers (void);
#define LockEventHandlers _LockEventHandlers
#else
#define LockEventHandlers do {} while (0)
#endif

#ifdef TCL_THREADS
void
_UnlockEventHandlers (void);
#define UnlockEventHandlers _UnlockEventHandlers
#else
#define UnlockEventHandlers do {} while (0)
#endif

void
InitEventHandlers (void);

void
SetEventHandler (
    int signum,
    Tcl_Interp *interp,
    Tcl_Obj *newCmdObj
    );

Tcl_Obj*
GetEventHandlerCommand (
    int signum
    );

void
DeleteThreadEvents (void);

#define __POSIX_SIGNAL_EVENTS_H
#endif /* __POSIX_SIGNAL_EVENTS_H */

/* vim: set ts=8 sts=4 sw=4 sts=4 noet: */
