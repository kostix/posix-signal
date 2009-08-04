#include <tcl.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>
#include "sigtables.h"
#include "sigobj.h"

/* Convenience macros to access fields of the signal object's intrep */

#define GET_SIGNUM(objPtr) \
	((int) objPtr->internalRep.ptrAndLongRep.value)
#define SET_SIGNUM(objPtr, signum) \
	(objPtr->internalRep.ptrAndLongRep.value = (unsigned long) (signum))
#define GET_SIGPTR(objPtr) \
	((const Signal *) objPtr->internalRep.ptrAndLongRep.ptr)
#define SET_SIGPTR(objPtr, sigPtr) \
	(objPtr->internalRep.ptrAndLongRep.ptr = (void *) (sigPtr))

#define IS_RTSIG(SIG) ((SIG) == NULL)

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

static void ReplaceIntRep (Tcl_Obj *objPtr, int signum,
			   const Signal *sigPtr);

MODULE_SCOPE
Tcl_Obj *
CreatePosixSignalObj (
    const Signal *sigPtr
    )
{
    Tcl_Obj *sigObj;

    sigObj = Tcl_NewObj();

    SET_SIGNUM(sigObj, sigPtr->signal);
    SET_SIGPTR(sigObj, sigPtr);
    InitStringRep(sigObj, sigPtr->name, sigPtr->length);

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
	return GET_SIGNUM(objPtr);
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
	return Tcl_GetStringFromObj(objPtr, lengthPtr);
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
    const Signal *sigPtr;

    sigPtr = GET_SIGPTR(objPtr);
    if (!IS_RTSIG(sigPtr)) {
	InitStringRep(objPtr, sigPtr->name, sigPtr->length);
    } else {
	char buf[TCL_INTEGER_SPACE];
	int len;
	len = sprintf(buf, "%d", GET_SIGNUM(objPtr));
	InitStringRep(objPtr, buf, len);
    }
}

static
int
SetFromAny (
    Tcl_Interp *interp,
    Tcl_Obj *objPtr
    )
{
    int res, signum;
    const Signal *sigPtr;
    const char *namePtr;

    res = Tcl_GetIntFromObj(NULL, objPtr, &signum);
    if (res == TCL_OK) {
	sigPtr = FindSignalBySignum(signum);
	if (sigPtr != NULL) {
	    objPtr->bytes = NULL;
	    ReplaceIntRep(objPtr, signum, sigPtr);
	    return TCL_OK;
	} else {
	    if (IsRealTimeSignal(signum)) {
		objPtr->bytes = NULL;
		ReplaceIntRep(objPtr, signum, NULL);
		return TCL_OK;
	    } else {
		Tcl_SetObjResult(interp,
			Tcl_NewStringObj("invalid signal", -1));
		return TCL_ERROR;
	    }
	}
    }

    namePtr = Tcl_GetStringFromObj(objPtr, NULL);
    sigPtr = FindSignalByName(namePtr);
    if (sigPtr != NULL) {
	/* The existing string rep is now verified
	 * to be a valid signal name,
	 * so we just update the integer inernal rep
	 * and patch the object's typePtr */
	ReplaceIntRep(objPtr, sigPtr->signal, sigPtr);
	return TCL_OK;
    } else {
	Tcl_SetObjResult(interp,
		Tcl_NewStringObj("invalid signal", -1));
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
    int signum,
    const Signal *sigPtr
    )
{
    if (objPtr->typePtr != NULL
	    && objPtr->typePtr->freeIntRepProc != NULL) {
	objPtr->typePtr->freeIntRepProc(objPtr);
    }

    SET_SIGNUM(objPtr, signum);
    SET_SIGPTR(objPtr, sigPtr);
    objPtr->typePtr = &posixSignalObjType;
}

/* vim: set ts=8 sts=4 sw=4 sts=4 noet: */
