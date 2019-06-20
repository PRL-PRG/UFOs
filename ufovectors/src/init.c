#include "../include/ufos.h"
#include "ufo_vectors.h"

#include <R_ext/Rdynload.h>
#include <R_ext/Visibility.h>

// List of functions provided by the package.
static const R_CallMethodDef CallEntries[] = {

    // Initialize the system.
    {"ufo_vectors_initialize",  (DL_FUNC) &ufo_vectors_initialize,  0},

    // Vectors that parially materialize on-demand from binary files.
    {"ufo_vectors_intsxp_bin",  (DL_FUNC) &ufo_vectors_intsxp_bin,  1},
    {"ufo_vectors_realsxp_bin", (DL_FUNC) &ufo_vectors_realsxp_bin, 1},
//    {"ufo_vectors_strsxp_bin",  (DL_FUNC) &ufo_vectors_strsxp_bin,  1},
    {"ufo_vectors_cplxsxp_bin", (DL_FUNC) &ufo_vectors_cplxsxp_bin, 1},
    {"ufo_vectors_lglsxp_bin",  (DL_FUNC) &ufo_vectors_lglsxp_bin,  1},
    {"ufo_vectors_rawsxp_bin",  (DL_FUNC) &ufo_vectors_rawsxp_bin,  1},

    // Storage.
    {"ufo_store_bin", (DL_FUNC) &ufo_store_bin, 2},

    // Shutdown the system.
    {"ufo_vectors_shutdown",    (DL_FUNC) &ufo_vectors_shutdown,    0},

    // Terminates the function list. Necessary.
    {NULL, NULL, 0} 
};

//ufo_initialize_t ufo_initialize;
//ufo_new_t ufo_new;
//ufo_shutdown_t ufo_shutdown;

// Initializes the package and registers the routines with the Rdynload 
// library. Name follows the pattern: R_init_<package_name> 
void attribute_visible R_init_ufovectors(DllInfo *dll) {
//    ufo_new = R_GetCCallable("ufos", "ufo_new");
//    ufo_shutdown = R_GetCCallable("ufos", "ufo_shutdown");
    //InitUFOAltRepClass(dll);
    //R_registerRoutines(dll, NULL, CallEntries, NULL, NULL);
    //R_useDynamicSymbols(dll, FALSE);
    //R_forceSymbols(dll, TRUE);
}


