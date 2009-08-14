#include <tcl.h>
#include "sigtables.h"
#include "sigmap.h"

#define WORDKEY(KEY) ((char *) (KEY))

void
InitSignalMap (
    SignalMap *sigmapPtr)
{
    Tcl_InitHashTable(sigmapPtr, TCL_ONE_WORD_KEYS);
}

void
FreeSignalMap (
    SignalMap *sigmapPtr)
{
    Tcl_DeleteHashTable(sigmapPtr);
}

ClientData
GetSigMapEntry (
    SignalMap *sigmapPtr,
    int signum)
{
    Tcl_HashEntry *entryPtr;

    entryPtr = Tcl_FindHashEntry(sigmapPtr, WORDKEY(signum));
    if (entryPtr != NULL) {
	return Tcl_GetHashValue(entryPtr);
    } else {
	return NULL;
    }
}

int
CreateSigMapEntry (
    SignalMap *sigmapPtr,
    int signum,
    ClientData clientData)
{
    Tcl_HashEntry *entryPtr;
    int isnew, ok;

    entryPtr = Tcl_CreateHashEntry(sigmapPtr, WORDKEY(signum), &isnew);
    ok = !isnew;
    if (ok) {
	Tcl_SetHashValue(entryPtr, clientData);
    }
    return ok;
}

ClientData
DeleteSigMapEntry (
    SignalMap *sigmapPtr,
    int signum)
{
    Tcl_HashEntry *entryPtr;

    entryPtr = Tcl_FindHashEntry(sigmapPtr, WORDKEY(signum));
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
FirstSigMapEntry (
    SignalMap *sigmapPtr,
    SignalMapSearch *searchPtr)
{
    return Tcl_FirstHashEntry(sigmapPtr, searchPtr);
}

ClientData
NextSigMapEntry (
    SignalMapSearch *searchPtr)
{
    return Tcl_NextHashEntry(searchPtr);
}

/* vim: set ts=8 sts=4 sw=4 sts=4 noet: */
