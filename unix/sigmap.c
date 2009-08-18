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

SignalMapEntry *
CreateSigMapEntry (
    SignalMap *sigmapPtr,
    int signum,
    int *isnewPtr)
{
    return Tcl_CreateHashEntry(sigmapPtr, WORDKEY(signum), isnewPtr);
}

ClientData
GetSigMapValue (
    SignalMapEntry *entryPtr)
{
    return Tcl_GetHashValue(entryPtr);
}

void
SetSigMapValue (
    SignalMapEntry *entryPtr,
    ClientData clientData)
{
    Tcl_SetHashValue(entryPtr, clientData);
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
    Tcl_HashEntry *entryPtr;

    entryPtr = Tcl_FirstHashEntry(sigmapPtr, searchPtr);
    if (entryPtr != NULL) {
	return Tcl_GetHashValue(entryPtr);
    } else {
	return NULL;
    }
}

ClientData
NextSigMapEntry (
    SignalMapSearch *searchPtr)
{
    Tcl_HashEntry *entryPtr;

    entryPtr = Tcl_NextHashEntry(searchPtr);
    if (entryPtr != NULL) {
	return Tcl_GetHashValue(entryPtr);
    } else {
	return NULL;
    }
}

/* vim: set ts=8 sts=4 sw=4 sts=4 noet: */
