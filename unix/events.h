#ifndef __POSIX_SIGNAL_EVENTS_H

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

void
InitEventHandlers (void);

void
SetEventHandler (
    int signum,
    Tcl_Interp *interp,
    Tcl_Obj *newCmdObj
    );

void
DeleteEventHandler (
    int signum
    );

Tcl_Obj*
GetEventHandlerCommand (
    int signum
    );

SignalEvent*
CreateSignalEvent (
    Tcl_ThreadId threadId,
    int signum
    );

void
DeleteThreadEvents (void);

#define __POSIX_SIGNAL_EVENTS_H
#endif /* __POSIX_SIGNAL_EVENTS_H */

/* vim: set ts=8 sts=4 sw=4 sts=4 noet: */
