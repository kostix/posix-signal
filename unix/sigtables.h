#ifndef __POSIX_SIGNAL_SIGTABLES_H

typedef struct {
    int signal;
    char *name;
} Signal;

typedef struct {
    int len;
    int items[];
} SignalVector;

extern const Signal signals[];
extern const int nsigs;

#define SIGOFFSET(SIG) ((SIG) - 1)

void
InitSignalTables (void);

SignalVector*
CreateSignalVector (void);

void
FreeSignalVector(
    SignalVector *svPtr
    );

int
GetSignalIdFromObj (
    Tcl_Interp *interp,
    Tcl_Obj *nameObj
    );

#define __POSIX_SIGNAL_SIGTABLES_H
#endif /* __POSIX_SIGNAL_SIGTABLES_H */

/* vim: set ts=8 sts=4 sw=4 sts=4 noet: */
