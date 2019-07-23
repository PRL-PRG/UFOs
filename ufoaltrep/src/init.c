#include "altrep_ufo_vectors.h"

#include <R_ext/Rdynload.h>
#include <R_ext/Visibility.h>

// List of functions provided by the package.
static const R_CallMethodDef CallEntries[] = {

    // Vectors that parially materialize on-demand from binary files.
    {"altrep_ufo_vectors_intsxp_bin",  (DL_FUNC) &altrep_ufo_vectors_intsxp_bin,  1},
    {"altrep_ufo_vectors_realsxp_bin", (DL_FUNC) &altrep_ufo_vectors_realsxp_bin, 1},
    // {"altrep_ufo_vectors_strsxp_bin",  (DL_FUNC) &altrep_ufo_vectors_strsxp_bin,  1},
    {"altrep_ufo_vectors_cplxsxp_bin", (DL_FUNC) &altrep_ufo_vectors_cplxsxp_bin, 1},
    {"altrep_ufo_vectors_lglsxp_bin",  (DL_FUNC) &altrep_ufo_vectors_lglsxp_bin,  1},
    {"altrep_ufo_vectors_rawsxp_bin",  (DL_FUNC) &altrep_ufo_vectors_rawsxp_bin,  1},

    // Turn on debug mode.
    {"altrep_ufo_vectors_set_debug_mode",  (DL_FUNC) &altrep_ufo_vectors_set_debug_mode,  1},

    // Terminates the function list. Necessary.
    {NULL, NULL, 0} 
};

void attribute_visible R_init_ufoaltrep(DllInfo *dll) {
    init_ufo_logical_altrep_class(dll);
    init_ufo_integer_altrep_class(dll);
    init_ufo_numeric_altrep_class(dll);
    init_ufo_complex_altrep_class(dll);
    init_ufo_raw_altrep_class(dll);

    R_registerRoutines(dll, NULL, CallEntries, NULL, NULL);
    R_useDynamicSymbols(dll, FALSE);
    R_forceSymbols(dll, TRUE);
}

void attribute_visible R_unload_ufoaltrep(DllInfo *dll) {
    printf("goodnight sweet prince\n");
}

