#ifndef __POSIX_SIGNAL_SIGTABLES_H

typedef struct {
    int signal;
    char *name;
    int length;
} Signal;

typedef struct {
    int len;
    int items[];
} SignalVector;

MODULE_SCOPE const Signal signals[];
MODULE_SCOPE const int nsigs;

MODULE_SCOPE int max_signum;

#define SIGOFFSET(SIG) ((SIG) - 1)

void
InitSignalTables (void);

SignalVector*
CreateSignalVector (void);

void
FreeSignalVector(
    SignalVector *svPtr
    );

MODULE_SCOPE
const char *
GetNameBySignum (
    Tcl_Interp *interp,
    int signum,
    int *lengthPtr
    );

MODULE_SCOPE
int
GetSignumByName (
    Tcl_Interp *interp,
    const char *namePtr
    );

MODULE_SCOPE
const Signal *
FindSignalBySignum (
    int signum
    );

MODULE_SCOPE
const Signal *
FindSignalByName (
    const char *namePtr
    );

MODULE_SCOPE
int
IsRealTimeSignal (
    int signum
    );

#define __POSIX_SIGNAL_SIGTABLES_H
#endif /* __POSIX_SIGNAL_SIGTABLES_H */

/* vim: set ts=8 sts=4 sw=4 sts=4 noet: */
