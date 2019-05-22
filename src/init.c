#include "ufos.h"
#include "sources.h"

#include <R_ext/Rdynload.h>
#include <R_ext/Visibility.h>

// List of functions provided by the package.
static const R_CallMethodDef CallEntries[] = {
    {"ufo_make_bin_file_source", (DL_FUNC) &ufo_make_bin_file_source, 1},

    {"ufo_new_intsxp", (DL_FUNC) &ufo_new_intsxp, 2},
    {"ufo_new_lglsxp", (DL_FUNC) &ufo_new_lglsxp, 2},

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
}


