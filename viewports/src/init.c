#include "debug.h"
#include "slices.h"

#include <R_ext/Rdynload.h>
#include <R_ext/Visibility.h>

// List of functions provided by the package.
static const R_CallMethodDef CallEntries[] = {

    // Vectors that parially materialize on-demand from binary files.
    {"slice",  (DL_FUNC) &create_slice, 3},

    // Turn on debug mode.
    {"viewport_set_debug_mode",  (DL_FUNC) &set_debug_mode,  1},

    // Terminates the function list. Necessary.
    {NULL, NULL, 0} 
};

void attribute_visible R_init_viewports(DllInfo *dll) {
    init_slice_altrep_class(dll);

    //R_registerRoutines(dll, NULL, CallEntries, NULL, NULL);
    //R_useDynamicSymbols(dll, FALSE);
    //R_forceSymbols(dll, TRUE);
}

void attribute_visible R_unload_viewports(DllInfo *dll) {
    printf("goodnight sweet prince\n");
}
