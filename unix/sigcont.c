#include <tcl.h>
#include "sigtables.h"
#include "sigcont.h"

#define WORDKEY(KEY) ((char *) (KEY))

typedef Tcl_HashTable SigContainer;

typedef Tcl_HashSearch SigContSearch;

void
SigCont_Init (
    SigContainer *sigcontPtr
    )
{
    Tcl_InitHashTable(sigcontPtr, TCL_ONE_WORD_KEYS);
}

void
SigCont_Free (
    SigContainer *sigcontPtr
    )
{
    Tcl_DeleteHashTable(sigcontPtr);
}

ClientData
SigCont_GetEntry (
    SigContainer *sigcontPtr,
    int signum
    )
{
    Tcl_HashEntry *entryPtr;

    entryPtr = Tcl_FindHashEntry(sigcontPtr, WORDKEY(signum));
    if (entryPtr != NULL) {
	return Tcl_GetHashValue(entryPtr);
    } else {
	return NULL;
    }
}

int
SigCont_CreateEntry (
    SigContainer *sigcontPtr,
    int signum,
    ClientData clientData
    )
{
    Tcl_HashEntry *entryPtr;
    int isnew, ok;

    entryPtr = Tcl_CreateHashEntry(sigcontPtr, WORDKEY(signum), &isnew);
    ok = !isnew;
    if (ok) {
	Tcl_SetHashValue(entryPtr, clientData);
    }
    return ok;
}

ClientData
SigCont_DeleteEntry (
    SigContainer *sigcontPtr,
    int signum
    )
{
    Tcl_HashEntry *entryPtr;

    entryPtr = Tcl_FindHashEntry(sigcontPtr, WORDKEY(signum));
    if (entryPtr != NULL) {
	ClientData clientData;

	clientData = Tcl_GetHashValue(entryPtr);
	Tcl_DeleteHashEntry(entryPtr);
	return clientData;
    } else {
	return NULL;
    }
}

ClientData
SigCont_FirstEntry (
    SigContainer *sigcontPtr,
    SigContSearch *searchPtr
    )
{
    return Tcl_FirstHashEntry(sigcontPtr, searchPtr);
}

ClientData
SigCont_NextEntry (
    SigContSearch *searchPtr
    )
{
    return Tcl_NextHashEntry(searchPtr);
}

/* vim: set ts=8 sts=4 sw=4 sts=4 noet: */
