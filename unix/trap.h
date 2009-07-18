#ifndef __POSIX_SIGNAL_TRAP_H

void
InitSignalHandlers (void);

int
Command_Trap (
    ClientData clientData,
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *const objv[]
    );

#define __POSIX_SIGNAL_TRAP_H
#endif /* __POSIX_SIGNAL_TRAP_H */

/* vim: set ts=8 sts=4 sw=4 sts=4 noet: */
