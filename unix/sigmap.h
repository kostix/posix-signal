#ifndef __POSIX_SIGNAL_SIGMAP_H

typedef Tcl_HashTable SignalMap;

typedef Tcl_HashEntry SignalMapEntry;

typedef Tcl_HashSearch SignalMapSearch;

MODULE_SCOPE
void
InitSignalMap (
    SignalMap *sigmapPtr);

MODULE_SCOPE
void
FreeSignalMap (
    SignalMap *sigmapPtr);

MODULE_SCOPE
SignalMapEntry *
FindSigMapEntry (
    SignalMap *sigmapPtr,
    int signum);

MODULE_SCOPE
SignalMapEntry *
CreateSigMapEntry (
    SignalMap *sigmapPtr,
    int signum,
    int *isnewPtr);

MODULE_SCOPE
ClientData
GetSigMapValue (
    SignalMapEntry *entryPtr);

MODULE_SCOPE
void
SetSigMapValue (
    SignalMapEntry *entryPtr,
    ClientData clientData);

MODULE_SCOPE
void
DeleteSigMapEntry (
    SignalMapEntry *entryPtr);

MODULE_SCOPE
ClientData
FirstSigMapEntry (
    SignalMap *sigmapPtr,
    SignalMapSearch *searchPtr);

MODULE_SCOPE
ClientData
NextSigMapEntry (
    SignalMapSearch *searchPtr);

#define __POSIX_SIGNAL_SIGMAP_H
#endif /* __POSIX_SIGNAL_SIGMAP_H */

/* vim: set ts=8 sts=4 sw=4 sts=4 noet: */
