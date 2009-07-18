#ifndef __POSIX_SIGNAL_SIGTABLES_H

typedef struct {
    int signal;
    char *name;
} Signal;

extern const Signal signals[];
extern const int nsigs;

#define SIGOFFSET(SIG) ((SIG) - 1)

void
InitSignalTables (void);

int
GetSignalIdFromObj (
    Tcl_Interp *interp,
    Tcl_Obj *nameObj
    );

#define __POSIX_SIGNAL_SIGTABLES_H
#endif /* __POSIX_SIGNAL_SIGTABLES_H */

/* vim: set ts=8 sts=4 sw=4 sts=4 noet: */
