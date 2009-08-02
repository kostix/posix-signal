#include <tcl.h>
#include <string.h>
#include <assert.h>
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

static void UpdateString (Tcl_Obj *objPtr);
static int SetFromAny (Tcl_Interp *interp, Tcl_Obj *objPtr);

static Tcl_ObjType posixSignalObjType = {
    "posix-signal",      /* name */
    NULL,                /* freeIntRepProc */
    NULL,                /* dupIntRepProc */
    UpdateString,        /* updateStringProc */
    SetFromAny           /* setFromAnyProc */
};

static void InitStringRep (Tcl_Obj *objPtr, const char *bytesPtr,
	int length);

static void ReplaceIntRep (Tcl_Obj *objPtr, int signum);

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

    sigObj = Tcl_NewObj();
    InitStringRep(sigObj, namePtr, strlen(namePtr));
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
    Tcl_Obj *objPtr,
    int *lengthPtr
    )
{
    int res;

    res = Tcl_ConvertToType(interp, objPtr, &posixSignalObjType);
    if (res == TCL_OK) {
	if (lengthPtr != NULL) {
	    *lengthPtr = objPtr->length;
	}
	return objPtr->bytes;
    } else {
	return NULL;
    }
}

static
void
UpdateString (
    Tcl_Obj *objPtr
    )
{
    int signum, len;
    const char *namePtr;

    signum = objPtr->internalRep.longValue;
    namePtr = GetNameBySignum(NULL, signum, &len);
    assert(namePtr != NULL);

    InitStringRep(objPtr, namePtr, len);
}

static
int
SetFromAny (
    Tcl_Interp *interp,
    Tcl_Obj *objPtr
    )
{
    int res, signum, len;
    const char *namePtr;

    res = Tcl_GetIntFromObj(NULL, objPtr, &signum);
    if (res == TCL_OK) {
	namePtr = GetNameBySignum(interp, signum, &len);
	if (namePtr != NULL) {
	    objPtr->bytes = NULL;
	    ReplaceIntRep(objPtr, signum);
	    return TCL_OK;
	} else {
	    return TCL_ERROR;
	}
    }

    if (objPtr->bytes == NULL) {
	if (objPtr->typePtr != NULL
		&& objPtr->typePtr->updateStringProc != NULL) {
	    objPtr->typePtr->updateStringProc(objPtr);
	} else {
	    /* TODO elaborate on signal message? */
	    Tcl_SetObjResult(interp,
		    Tcl_NewStringObj("invalid signal", -1));
	    return TCL_ERROR;
	}
    }
    signum = GetSignumByName(interp, objPtr->bytes);
    if (signum > 0) {
	/* The existing string rep is now verified
	 * to be a valid signal name,
	 * so we just update the integer inernal rep
	 * and patch the object's typePtr */
	ReplaceIntRep(objPtr, signum);
	return TCL_OK;
    } else {
	return TCL_ERROR;
    }
}

static
void
InitStringRep (
    Tcl_Obj *objPtr,
    const char *bytesPtr,
    int length
    )
{
    objPtr->bytes = ckalloc(length + 1);
    memcpy(objPtr->bytes, bytesPtr, length + 1);
    objPtr->length = length;
}

static
void
ReplaceIntRep (
    Tcl_Obj *objPtr,
    int signum
    )
{
    if (objPtr->typePtr != NULL
	    && objPtr->typePtr->freeIntRepProc != NULL) {
	objPtr->typePtr->freeIntRepProc(objPtr);
    }

    objPtr->internalRep.longValue = signum;
    objPtr->typePtr = &posixSignalObjType;
}

/* vim: set ts=8 sts=4 sw=4 sts=4 noet: */
