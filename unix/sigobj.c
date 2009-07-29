#include <tcl.h>
#include <string.h>
#include "sigtables.h"
#include "sigobj.h"

/*
typedef struct Tcl_ObjType {
    char *name;
    Tcl_FreeInternalRepProc *freeIntRepProc;
    Tcl_DupInternalRepProc *dupIntRepProc;
    Tcl_UpdateStringProc *updateStringProc;
    Tcl_SetFromAnyProc *setFromAnyProc;
} Tcl_ObjType;
*/

static void FreeInternalRep (Tcl_Obj *objPtr);
static void DupInternalRep (Tcl_Obj *srcPtr, Tcl_Obj *dupPtr);
static int SetFromAny (Tcl_Interp *interp, Tcl_Obj *objPtr);

static Tcl_ObjType posixSignalObjType = {
    "posix-signal",      /* name */
    FreeInternalRep,     /* freeIntRepProc */
    DupInternalRep,      /* dupIntRepProc */
    NULL,                /* updateStringProc */
    SetFromAny           /* setFromAnyProc */
};

/* TODO move these procs to sigtables.c
 * and provide sensible implementations */
static const char* GetNameBySignum (Tcl_Interp *interp, int signum);
static int GetSignumByName (Tcl_Interp *interp, const char *name);

/* TODO duplication of a string into a Tcl_Obj's string rep
 * is used three times in this module -- looks like
 * a candidate for a separate proc or a macro */

/* Internal function -- does not validate neither signum,
 * nor namePtr, nor their coherency */
MODULE_SCOPE
Tcl_Obj *
CreatePosixSignalObj (
    int signum,
    const char *namePtr
    )
{
    Tcl_Obj *sigObj;
    int len;

    sigObj = Tcl_NewObj();

    len = strlen(namePtr);
    sigObj->bytes = ckalloc(len + 1);
    memcpy(sigObj->bytes, namePtr, len + 1);
    sigObj->length = len;

    sigObj->internalRep.longValue = signum;

    sigObj->typePtr = &posixSignalObjType;

    return sigObj;
}

MODULE_SCOPE
int
GetSignumFromObj (
    Tcl_Interp *interp,
    Tcl_Obj *objPtr
    )
{
    int res;

    res = Tcl_ConvertToType(interp, objPtr, &posixSignalObjType);
    if (res == TCL_OK) {
	return objPtr->internalRep.longValue;
    } else {
	return -1;
    }
}

MODULE_SCOPE
const char *
GetSignalNameFromObj (
    Tcl_Interp *interp,
    Tcl_Obj *objPtr
    )
{
    int res;

    res = Tcl_ConvertToType(interp, objPtr, &posixSignalObjType);
    if (res == TCL_OK) {
	return objPtr->bytes;
    } else {
	return NULL;
    }
}

static
void
FreeInternalRep (
    Tcl_Obj *objPtr
    )
{
    ckfree(objPtr->bytes);
}

static
void
DupInternalRep (
    Tcl_Obj *srcPtr,
    Tcl_Obj *dupPtr
    )
{
    int len;

    len = srcPtr->length;
    dupPtr->bytes = ckalloc(len + 1);
    memcpy(dupPtr->bytes, srcPtr->bytes, len + 1);
    dupPtr->length = len;

    dupPtr->internalRep.longValue = srcPtr->internalRep.longValue;
}

static
int
SetFromAny (
    Tcl_Interp *interp,
    Tcl_Obj *objPtr
    )
{
    int res, signum;
    const char *namePtr;

    res = Tcl_GetIntFromObj(NULL, objPtr, &signum);
    if (res == TCL_OK) {
	namePtr = GetNameBySignum(interp, signum);
	if (namePtr != NULL) {
	    const int len = strlen(namePtr);
	    objPtr->bytes = ckalloc(len + 1);
	    memcpy(objPtr->bytes, namePtr, len + 1);
	    objPtr->length = len;
	    /* FIXME do we need this? */
	    objPtr->internalRep.longValue = signum;
	    objPtr->typePtr = &posixSignalObjType;
	    return TCL_OK;
	} else {
	    return TCL_ERROR;
	}
    }

    namePtr = Tcl_GetStringFromObj(objPtr, NULL);
    signum = GetSignumByName(interp, namePtr);
    if (signum > 0) {
	/* The existing string rep is now verified
	 * to be a valid signal name,
	 * so we just update the integer inernal rep
	 * and patch the object's typePtr */
	objPtr->internalRep.longValue = signum;
	objPtr->typePtr   = &posixSignalObjType;
	return TCL_OK;
    } else {
	return TCL_ERROR;
    }
}

static
const char *
GetNameBySignum (
    Tcl_Interp *interp,
    int signum
    )
{
    return "SIGFAKE";
}

static
int
GetSignumByName (
    Tcl_Interp *interp,
    const char *namePtr
    )
{
    return 256;
}

/* vim: set ts=8 sts=4 sw=4 sts=4 noet: */
