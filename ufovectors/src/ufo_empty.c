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

typedef struct {
    ufo_vector_type_t   vector_type;
    size_t              element_size; /* in bytes */
    size_t              vector_size;
} data_t;

int __populate_empty(uint64_t start, uint64_t end, ufPopulateCallout cf, ufUserData user_data, char* target) {
	data_t *data = (data_t*) user_data;

	if (__get_debug_mode()) {
	    REprintf("__populate\n");
	    REprintf("    vector type: %d\n", data->vector_type);
	    REprintf("    vector size: %li\n", data->vector_size);
	    REprintf("   element size: %li\n", data->element_size);
	}

	size_t size = end - start;
	switch (data->vector_type) {
	case UFO_STR:
	/* case UFO_VEC:*/ {
    		SEXP *sexp_vector = (SEXP *) target;
    		for (size_t i = 0; i < size; i++) {
    			sexp_vector[i] = R_BlankString;
    		}
    		return 0;
    	}

	case UFO_LGL: {
    		printf("UFO_LGL\n");
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

	return 1;
}

void __destroy_empty(ufUserData *user_data) {
    data_t *data = (data_t*) user_data;

    if (__get_debug_mode()) {
        REprintf("__destroy\n");
        REprintf("    vector type: %d\n", data->vector_type);
        REprintf("    vector size: %li\n", data->vector_size);
        REprintf("   element size: %li\n", data->element_size);
    }

    free(data);
}

SEXP __make_empty(ufo_vector_type_t type, SEXP/*REALSXP*/ size_sexp, SEXP/*INTSXP*/ min_load_count_sexp) {
    int32_t min_load_count = __extract_int_or_die(min_load_count_sexp);
    R_xlen_t size = __extract_R_xlen_t_or_die(size_sexp);

    ufo_source_t* source = (ufo_source_t*) malloc(sizeof(ufo_source_t));
    source->population_function = &__populate_empty;
    source->destructor_function = &__destroy_empty;
    source->vector_type = type;
    source->element_size = __get_element_size(type);
    source->vector_size = size;
    source->dimensions = NULL;
    source->dimensions_length = 0;
    source->min_load_count = __select_min_load_count(min_load_count, source->element_size);

    data_t *data = (data_t*) malloc(sizeof(data_t));
    data->vector_type = source->vector_type;
    data->vector_size = source->vector_size;
    data->element_size = source->element_size;

    source->data = (ufUserData*) data;

    ufo_new_t ufo_new = (ufo_new_t) R_GetCCallable("ufos", "ufo_new");
    return ufo_new(source);
}

SEXP ufo_intsxp_empty(SEXP/*REALSXP*/ size, SEXP/*INTSXP*/ min_load_count) {
	return __make_empty(INTSXP, size, min_load_count);
}

SEXP ufo_realsxp_empty(SEXP/*REALSXP*/ size, SEXP/*INTSXP*/ min_load_count) {
	return __make_empty(REALSXP, size, min_load_count);
}

SEXP ufo_rawsxp_empty(SEXP/*REALSXP*/ size, SEXP/*INTSXP*/ min_load_count) {
	return __make_empty(RAWSXP, size, min_load_count);
}

SEXP ufo_cplxsxp_empty(SEXP/*REALSXP*/ size, SEXP/*INTSXP*/ min_load_count) {
	return __make_empty(CPLXSXP, size, min_load_count);
}

SEXP ufo_lglsxp_empty(SEXP/*REALSXP*/ size, SEXP/*INTSXP*/ min_load_count) {
	return __make_empty(LGLSXP, size, min_load_count);
}

SEXP ufo_vecsxp_empty(SEXP/*REALSXP*/ size, SEXP/*INTSXP*/ min_load_count) {
	return __make_empty(VECSXP, size, min_load_count);
}

SEXP ufo_strsxp_empty(SEXP/*REALSXP*/ size, SEXP/*INTSXP*/ min_load_count) {
	return __make_empty(STRSXP, size, min_load_count);
}
