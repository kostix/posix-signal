#include <tcl.h>
#include "utils.h"

MODULE_SCOPE
void
ReportPosixError (
    Tcl_Interp *interp
    )
{
    const char *errStrPtr = Tcl_PosixError(interp);
    Tcl_SetObjResult(interp, Tcl_NewStringObj(errStrPtr, -1));
}

int
IsEmptyString (
    Tcl_Obj *objPtr)
{
    int len;

    Tcl_GetStringFromObj(objPtr, &len);
    return len == 0;
}

/* vim: set ts=8 sts=4 sw=4 sts=4 noet: */
