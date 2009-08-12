#include <tcl.h>

static
int
AppInit (
    Tcl_Interp *interp)
{
    return TCL_OK;
}

int
main (int argc, char **argv)
{
    Tcl_Main(argc, argv, AppInit);

    return 0;
}

/* vim: set ts=8 sts=4 sw=4 sts=4 noet: */
