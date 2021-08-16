#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#define USE_RINTERNALS
#include <R.h>
#include <Rinternals.h>

#include "../include/ufos.h"
#include "ufo_metadata.h"
#include "helpers.h"
#include "debug.h"

#include "safety_first.h"

typedef struct {
    ufo_vector_type_t type;
	bool              populate_with_na;
} data_t;

static int32_t __populate_empty(void* user_data, uintptr_t start, uintptr_t end, unsigned char* target) {
	data_t data = *((data_t*) user_data);

	if (__get_debug_mode()) {
	    REprintf("__populate\n");
	    REprintf("    vector type: %d, data address was %ld\n", data.type, (uintptr_t) user_data);
	}

	uintptr_t size = end - start;
	switch (data.type) {
	case UFO_VEC: {
		SEXP *sexp_vector = (SEXP *) target;
		if (data.populate_with_na) {
			Rf_warning("UFO cannot populate VECSXP with NAs, " 
			           "populating with NULLs instead.");
		}
		for (size_t i = 0; i < size; i++) {
			sexp_vector[i] = R_NilValue;
		}
		return 0;
	}

	case UFO_STR: {
		SEXP *sexp_vector = (SEXP *) target;
		SEXP initial_value = 
			data.populate_with_na ? NA_STRING : R_BlankString;
		for (size_t i = 0; i < size; i++) {
			sexp_vector[i] = initial_value;
		}
		return 0;
	}

	case UFO_LGL: {
		Rboolean *boolean_vector = (Rboolean *) target;
		Rboolean initial_value = 
			data.populate_with_na ? NA_LOGICAL : 0;
		for (size_t i = 0; i < size; i++) {
			boolean_vector[i] = initial_value;
		}
		return 0;
	}

	case UFO_INT: {
		int *integer_vector = (int *) target;
		int initial_value = 
			data.populate_with_na ? NA_INTEGER : 0;
		for (size_t i = 0; i < size; i++) {
			integer_vector[i] = initial_value;
		}
		return 0;
	}

	case UFO_REAL: {
		double *double_vector = (double *) target;
		double initial_value = 
			data.populate_with_na ? NA_REAL : 0;
		for (size_t i = 0; i < size; i++) {
			double_vector[i] = initial_value;
		}
		return 0;
	}

	case UFO_RAW: {
		Rbyte *byte_vector = (Rbyte *) target;
		if (data.populate_with_na) {
			Rf_warning("UFO cannot populate RAWSXP with NAs, " 
			           "populating with 0s instead.");
		}
		for (size_t i = 0; i < size; i++) {
			byte_vector[i] = 0;
		}
		return 0;
	}

	case UFO_CPLX: {
		Rcomplex *complex_vector = (Rcomplex *) target;
		Rboolean initial_value = 
			data.populate_with_na ? NA_REAL : 0;
		for (size_t i = 0; i < size; i++) {
			complex_vector[i].r = initial_value;
			complex_vector[i].i = initial_value;
		}
		return 0;
	}
	
	case UFO_CHAR: {
		Rf_error("Cannot create UFO of type CHARSXP");
	}}

	REprintf("Unknown vector type %i\n", data.type);
	return 1;
}

void __destroy_empty(void* user_data) {
    data_t *data = (data_t*) user_data;

    if (__get_debug_mode()) {
        REprintf("__destroy_empty\n");
        REprintf("    vector type: %d\n", data->type);
    }

    free(data);
}

SEXP ufo_empty(ufo_vector_type_t type, R_xlen_t size, bool populate_with_na, int32_t min_load_count) {
    ufo_source_t* source = (ufo_source_t*) malloc(sizeof(ufo_source_t));
    if(source == NULL) {
    	Rf_error("Cannot allocate ufo_source_t");
    }

    source->population_function = &__populate_empty;
    source->destructor_function = &__destroy_empty;
    source->vector_type = type;
    source->element_size = __get_element_size(type);
    source->vector_size = size;
    source->dimensions = NULL;
    source->dimensions_length = 0;
    source->min_load_count = __select_min_load_count(min_load_count, source->element_size);
	source->read_only = false;

    make_sure(sizeof(ufo_vector_type_t) <= sizeof(int64_t), "Cannot fit vector type information into ufUserData pointer.");
    data_t *data = (data_t *) malloc(sizeof(data_t));
    data->type = type;
	data->populate_with_na = populate_with_na;
    source->data = (void*) data;

    ufo_new_t ufo_new = (ufo_new_t) R_GetCCallable("ufos", "ufo_new");
    SEXP result = ufo_new(source);
    return result;
}

SEXP ufo_intsxp_empty(SEXP/*REALSXP*/ size, SEXP/*LGLSXP*/ fill_with_nas, SEXP/*INTSXP*/ min_load_count) {
	return ufo_empty(INTSXP,
			__extract_R_xlen_t_or_die(size),
			__extract_boolean_or_die(fill_with_nas),
			//__extract_boolean_or_die(read_only),
			__extract_int_or_die(min_load_count));
}

SEXP ufo_realsxp_empty(SEXP/*REALSXP*/ size, SEXP/*LGLSXP*/ fill_with_nas, SEXP/*INTSXP*/ min_load_count) {
	return ufo_empty(REALSXP,
			__extract_R_xlen_t_or_die(size),
			__extract_boolean_or_die(fill_with_nas),
			//__extract_boolean_or_die(read_only),
			__extract_int_or_die(min_load_count));
}

SEXP ufo_rawsxp_empty(SEXP/*REALSXP*/ size, SEXP/*INTSXP*/ min_load_count) {
	return ufo_empty(RAWSXP,
			__extract_R_xlen_t_or_die(size),
			false,
			//__extract_boolean_or_die(read_only),
			__extract_int_or_die(min_load_count));
}

SEXP ufo_cplxsxp_empty(SEXP/*REALSXP*/ size, SEXP/*LGLSXP*/ fill_with_nas, SEXP/*INTSXP*/ min_load_count) {
	return ufo_empty(CPLXSXP,
			__extract_R_xlen_t_or_die(size),
			__extract_boolean_or_die(fill_with_nas),
			//__extract_boolean_or_die(read_only),
			__extract_int_or_die(min_load_count));
}

SEXP ufo_lglsxp_empty(SEXP/*REALSXP*/ size, SEXP/*LGLSXP*/ fill_with_nas, SEXP/*INTSXP*/ min_load_count) {
	return ufo_empty(LGLSXP,
			__extract_R_xlen_t_or_die(size),
			__extract_boolean_or_die(fill_with_nas),
			//__extract_boolean_or_die(read_only),
			__extract_int_or_die(min_load_count));
}

SEXP ufo_vecsxp_empty(SEXP/*REALSXP*/ size, SEXP/*INTSXP*/ min_load_count) {
	return ufo_empty(VECSXP,
			__extract_R_xlen_t_or_die(size),
			false,
			//__extract_boolean_or_die(read_only),
			__extract_int_or_die(min_load_count));
}

SEXP ufo_strsxp_empty(SEXP/*REALSXP*/ size, SEXP/*LGLSXP*/ fill_with_nas, SEXP/*INTSXP*/ min_load_count) {
	return ufo_empty(STRSXP,
			__extract_R_xlen_t_or_die(size),
			__extract_boolean_or_die(fill_with_nas),
			//__extract_boolean_or_die(read_only),
			__extract_int_or_die(min_load_count));
}
