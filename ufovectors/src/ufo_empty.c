#include <stdio.h>
#include <stdlib.h>

#define USE_RINTERNALS
#include <R.h>
#include <Rinternals.h>

#include "../include/ufos.h"
#include "ufo_metadata.h"
#include "helpers.h"
#include "debug.h"

#include "../include/mappedMemory/userfaultCore.h"

#include "make_sure.h"

typedef struct {
    ufo_vector_type_t   vector_type;
    size_t              element_size; /* in bytes */
    size_t              vector_size;
} data_t;

int __populate_empty(uint64_t start, uint64_t end, ufPopulateCallout cf, ufUserData user_data, char* target) {
	ufo_vector_type_t type = *((ufo_vector_type_t*) user_data);

	if (__get_debug_mode()) {
	    REprintf("__populate\n");
	    REprintf("    vector type: %d\n", type);
	}

	size_t size = end - start;
	switch (type) {
	case UFO_STR:
	/* case UFO_VEC:*/ {
    		SEXP *sexp_vector = (SEXP *) target;
    		for (size_t i = 0; i < size; i++) {
    			sexp_vector[i] = R_BlankString;
    		}
    		return 0;
    	}

	case UFO_LGL: {
    		Rboolean *boolean_vector = (Rboolean *) target;
    	    for (size_t i = 0; i < size; i++) {
    	    	boolean_vector[i] = 0;
    	    }
    	    return 0;
    	}

	case UFO_INT: {
    		int *integer_vector = (int *) target;
    	    for (size_t i = 0; i < size; i++) {
    	     	integer_vector[i] = 0;
    	    }
    	    return 0;
    	}

	case UFO_REAL: {
    		double *double_vector = (double *) target;
    	    for (size_t i = 0; i < size; i++) {
    	     	double_vector[i] = 0;
    	    }
    	    return 0;
    	}

	case UFO_RAW: {
    	    Rbyte *byte_vector = (Rbyte *) target;
    	    for (size_t i = 0; i < size; i++) {
    	      	byte_vector[i] = 0;
    	    }
    		return 0;
    	}

	case UFO_CPLX: {
    		Rcomplex *complex_vector = (Rcomplex *) target;
    		for (size_t i = 0; i < size; i++) {
    		   	complex_vector[i].r = 0;
    		   	complex_vector[i].i = 0;
    		}
    		return 0;
    	}
	}

	REprintf("Unknown vector type %i", type);
	return 1;
}

void __destroy_empty(ufUserData *user_data) {
    int64_t *type = (int64_t *) user_data;

    if (__get_debug_mode()) {
        REprintf("__destroy_empty\n");
        REprintf("    vector type: %d\n", type);
    }

    free(type);
}

SEXP ufo_empty(ufo_vector_type_t type, R_xlen_t size, int32_t min_load_count) {
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

    make_sure(sizeof(ufo_vector_type_t) <= sizeof(int64_t), Rf_error, "Cannot fit vector type information into ufUserData pointer.");
    int64_t *data = (int64_t *) malloc(sizeof(int64_t));
    *data = type;
    source->data = (ufUserData *) data;

    ufo_new_t ufo_new = (ufo_new_t) R_GetCCallable("ufos", "ufo_new");
    SEXP result = ufo_new(source);
    return result;
}

SEXP ufo_intsxp_empty(SEXP/*REALSXP*/ size, SEXP/*INTSXP*/ min_load_count) {
	return ufo_empty(INTSXP,
			__extract_R_xlen_t_or_die(size),
			__extract_int_or_die(min_load_count));
}

SEXP ufo_realsxp_empty(SEXP/*REALSXP*/ size, SEXP/*INTSXP*/ min_load_count) {
	return ufo_empty(REALSXP,
			__extract_R_xlen_t_or_die(size),
			__extract_int_or_die(min_load_count));
}

SEXP ufo_rawsxp_empty(SEXP/*REALSXP*/ size, SEXP/*INTSXP*/ min_load_count) {
	return ufo_empty(RAWSXP,
			__extract_R_xlen_t_or_die(size),
			__extract_int_or_die(min_load_count));
}

SEXP ufo_cplxsxp_empty(SEXP/*REALSXP*/ size, SEXP/*INTSXP*/ min_load_count) {
	return ufo_empty(CPLXSXP,
			__extract_R_xlen_t_or_die(size),
			__extract_int_or_die(min_load_count));
}

SEXP ufo_lglsxp_empty(SEXP/*REALSXP*/ size, SEXP/*INTSXP*/ min_load_count) {
	return ufo_empty(LGLSXP,
			__extract_R_xlen_t_or_die(size),
			__extract_int_or_die(min_load_count));
}

SEXP ufo_vecsxp_empty(SEXP/*REALSXP*/ size, SEXP/*INTSXP*/ min_load_count) {
	return ufo_empty(VECSXP,
			__extract_R_xlen_t_or_die(size),
			__extract_int_or_die(min_load_count));
}

SEXP ufo_strsxp_empty(SEXP/*REALSXP*/ size, SEXP/*INTSXP*/ min_load_count) {
	return ufo_empty(STRSXP,
			__extract_R_xlen_t_or_die(size),
			__extract_int_or_die(min_load_count));
}
