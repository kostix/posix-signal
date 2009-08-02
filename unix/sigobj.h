#ifndef __POSIX_SIGNAL_SIGOBJ_H

/* Temporarily turned off to not introduce the
 * dependency on sigtables.h */
#if 0
MODULE_SCOPE
Tcl_Obj *
CreatePosixSignalObj (
    const Signal *sigPtr
    );
#endif

MODULE_SCOPE
int
GetSignumFromObj (
    Tcl_Interp *interp,
    Tcl_Obj *objPtr
    );

MODULE_SCOPE
const char *
GetSignalNameFromObj (
    Tcl_Interp *interp,
    Tcl_Obj *objPtr,
    int *lengthPtr
    );

#define __POSIX_SIGNAL_SIGOBJ_H
#endif /* __POSIX_SIGNAL_SIGOBJ_H */

/* vim: set ts=8 sts=4 sw=4 sts=4 noet: */
