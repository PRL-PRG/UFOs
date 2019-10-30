#include "helpers.h"

int altrep_ufo_debug_mode = 0;

SEXP/*NILSXP*/ altrep_ufo_vectors_set_debug_mode(SEXP/*LGLSXP*/ debug) {
    altrep_ufo_debug_mode = __extract_boolean_or_die(debug);
    return R_NilValue;
}

int __get_debug_mode() {
    return altrep_ufo_debug_mode;
}