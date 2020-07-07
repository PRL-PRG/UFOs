#include "../include/ufos.h"
#include "ufo_vectors.h"
#include "ufo_empty.h"
#include "ufo_csv.h"
#include "ufo_operators.h"

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

	// Constructors for empty vectors.
	{"ufo_intsxp_empty",      		(DL_FUNC) &ufo_intsxp_empty,          		2},
	{"ufo_realsxp_empty",     		(DL_FUNC) &ufo_realsxp_empty,         		2},
	{"ufo_cplxsxp_empty",     		(DL_FUNC) &ufo_cplxsxp_empty,         		2},
	{"ufo_lglsxp_empty",      		(DL_FUNC) &ufo_lglsxp_empty,          		2},
	{"ufo_rawsxp_empty",      		(DL_FUNC) &ufo_rawsxp_empty,          		2},
	{"ufo_strsxp_empty",      		(DL_FUNC) &ufo_strsxp_empty,          		2},
	{"ufo_vecsxp_empty",      		(DL_FUNC) &ufo_vecsxp_empty,          		2},
    
    // CSV support
    {"ufo_csv",                     (DL_FUNC) &ufo_csv,                         5},

    // Storage.
    {"ufo_store_bin",               (DL_FUNC) &ufo_store_bin,                   2},

    // Turn on debug mode.
    {"ufo_vectors_set_debug_mode",  (DL_FUNC) &ufo_vectors_set_debug_mode,      1},

	// Operators.
	{"ufo_add", 					(DL_FUNC) &ufo_add,							2},
	{"ufo_sub",						(DL_FUNC) &ufo_sub,							2},
	{"ufo_mul",						(DL_FUNC) &ufo_mul,							2},
	{"ufo_div",						(DL_FUNC) &ufo_div,							2},
	{"ufo_pow",						(DL_FUNC) &ufo_pow,							2},
	{"ufo_mod",						(DL_FUNC) &ufo_mod,							2},
	{"ufo_idiv",					(DL_FUNC) &ufo_idiv,						2},
	{"ufo_lt",						(DL_FUNC) &ufo_lt,							2},
	{"ufo_le",						(DL_FUNC) &ufo_le,							2},
	{"ufo_gt",						(DL_FUNC) &ufo_gt,							2},
	{"ufo_ge",						(DL_FUNC) &ufo_ge,							2},
	{"ufo_eq",						(DL_FUNC) &ufo_eq,							2},
	{"ufo_neq",						(DL_FUNC) &ufo_neq,							2},
	{"ufo_neg",						(DL_FUNC) &ufo_neg,							1},
	{"ufo_bor",						(DL_FUNC) &ufo_bor,							2},
	{"ufo_band",					(DL_FUNC) &ufo_band,						2},
	{"ufo_or",						(DL_FUNC) &ufo_or,							2},
	{"ufo_and",						(DL_FUNC) &ufo_and,							2},
	{"ufo_subset",					(DL_FUNC) &ufo_subset,						2},
	{"ufo_subset_assign",			(DL_FUNC) &ufo_subset_assign,				3},

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


