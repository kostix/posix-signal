#include <tcl.h>
#include "sigtables.h"
#include "sigcont.h"

#define WORDKEY(KEY) ((char *) (KEY))

typedef Tcl_HashTable SigContainer;

typedef Tcl_HashEntry SigContEntry;

void
SigCont_Init (
    SigContainer *sigcontPtr
    )
{
    Tcl_InitHashTable(sigcontPtr, TCL_ONE_WORD_KEYS);
}

SigContEntry *
SigCont_FindEntry (
    SigContainer *sigcontPtr,
    int signum
    )
{
    return Tcl_FindHashEntry(sigcontPtr, WORDKEY(signum));
}

/* vim: set ts=8 sts=4 sw=4 sts=4 noet: */
