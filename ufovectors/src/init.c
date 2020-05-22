#include "../include/ufos.h"
#include "ufo_vectors.h"
#include "ufo_csv.h"

#include <R_ext/Rdynload.h>
#include <R_ext/Visibility.h>

// List of functions provided by the package.
static const R_CallMethodDef CallEntries[] = {

    // Constructors for vectors that partially materialize on-demand from
    // binary files.
    {"ufo_vectors_intsxp_bin",      (DL_FUNC) &ufo_vectors_intsxp_bin,          2},
    {"ufo_vectors_realsxp_bin",     (DL_FUNC) &ufo_vectors_realsxp_bin,         2},
    {"ufo_vectors_cplxsxp_bin",     (DL_FUNC) &ufo_vectors_cplxsxp_bin,         2},
    {"ufo_vectors_lglsxp_bin",      (DL_FUNC) &ufo_vectors_lglsxp_bin,          2},
    {"ufo_vectors_rawsxp_bin",      (DL_FUNC) &ufo_vectors_rawsxp_bin,          2},

    // Constructors for matrices composed of the above-mentioned vectors.
    {"ufo_matrix_intsxp_bin",       (DL_FUNC) &ufo_matrix_intsxp_bin,           4},
    {"ufo_matrix_realsxp_bin",      (DL_FUNC) &ufo_matrix_realsxp_bin,          4},
    {"ufo_matrix_cplxsxp_bin",      (DL_FUNC) &ufo_matrix_cplxsxp_bin,          4},
    {"ufo_matrix_lglsxp_bin",       (DL_FUNC) &ufo_matrix_lglsxp_bin,           4},
    {"ufo_matrix_rawsxp_bin",       (DL_FUNC) &ufo_matrix_rawsxp_bin,           4},
    
    // CSV support
    {"ufo_csv",                     (DL_FUNC) &ufo_csv,                         5},

    // Storage.
    {"ufo_store_bin",               (DL_FUNC) &ufo_store_bin,                   2},

    // Turn on debug mode.
    {"ufo_vectors_set_debug_mode",  (DL_FUNC) &ufo_vectors_set_debug_mode,      1},

    // Terminates the function list. Necessary.
    {NULL,                          NULL,                                       0}
};

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


