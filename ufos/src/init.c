#include "ufos.h"

#include <R_ext/Rdynload.h>
#include <R_ext/Visibility.h>

// List of functions provided by the package.
static const R_CallMethodDef CallEntries[] __attribute__ ((unused)) = {
    // Start up and shutdown the system.
    {"ufo_initialize", (DL_FUNC) &ufo_initialize, 0},
    {"ufo_shutdown", (DL_FUNC) &ufo_shutdown, 0},
	{"is_ufo", (DL_FUNC) &is_ufo, 1},

    // Terminates the function list. Necessary.
    {NULL, NULL, 0} 
};

// Initializes the package and registers the routines with the Rdynload 
// library. Name follows the pattern: R_init_<package_name> 
void attribute_visible R_init_ufos(DllInfo *dll) {
    R_RegisterCCallable("ufos", "ufo_new", (DL_FUNC) &ufo_new);
    R_RegisterCCallable("ufos", "ufo_new_multidim", (DL_FUNC) &ufo_new_multidim);
}
