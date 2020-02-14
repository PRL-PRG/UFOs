#include "ufos.h"

#include <R_ext/Rdynload.h>
#include <R_ext/Visibility.h>

// List of functions provided by the package.
static const R_CallMethodDef CallEntries[] __attribute__ ((unused)) = {
    // Start up and shutdown the system.
    {"ufo_initialize", (DL_FUNC) &ufo_initialize, 0},
    {"ufo_shutdown", (DL_FUNC) &ufo_shutdown, 0},

    // Terminates the function list. Necessary.
    {NULL, NULL, 0} 
};

// Initializes the package and registers the routines with the Rdynload 
// library. Name follows the pattern: R_init_<package_name> 
void attribute_visible R_init_ufos(DllInfo *dll) {
    //InitUFOAltRepClass(dll);
    //R_registerRoutines(dll, NULL, CallEntries, NULL, NULL);
    //R_useDynamicSymbols(dll, FALSE);
    //R_forceSymbols(dll, TRUE);

    //R_RegisterCCallable("ufos", "ufo_initialize", (DL_FUNC) &ufo_initialize);
    R_RegisterCCallable("ufos", "ufo_new", (DL_FUNC) &ufo_new);
    R_RegisterCCallable("ufos", "ufo_new_multidim", (DL_FUNC) &ufo_new_multidim);
    //R_RegisterCCallable("ufos", "ufo_shutdown", (DL_FUNC) &ufo_shutdown);
}


