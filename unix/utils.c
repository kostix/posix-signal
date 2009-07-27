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

/* vim: set ts=8 sts=4 sw=4 sts=4 noet: */
