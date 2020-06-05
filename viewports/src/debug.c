#include "debug.h"
#include "helpers.h"

int __debug_mode = 0;

SEXP/*NILSXP*/ set_debug_mode(SEXP/*LGLSXP*/ debug) {
    __debug_mode = __extract_boolean_or_die(debug);
    return R_NilValue;
}

int get_debug_mode() {
    return __debug_mode;
}