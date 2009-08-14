#ifndef __POSIX_SIGNAL_SIGMAP_H

typedef Tcl_HashTable SignalMap;

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
ClientData
GetSigMapEntry (
    SignalMap *sigmapPtr,
    int signum);

MODULE_SCOPE
int
CreateSigMapEntry (
    SignalMap *sigmapPtr,
    int signum,
    ClientData clientData);

MODULE_SCOPE
ClientData
DeleteSigMapEntry (
    SignalMap *sigmapPtr,
    int signum);

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
