#include "../include/ufos.h"
#include "../include/mappedMemory/userfaultCore.h"
#include "ufo_seq.h"
#include "make_sure.h"
#include "helpers.h"

typedef struct {
    int from;
    int to;
    int by;
} ufo_seq_data_t;

void destroy_data(ufUserData *data) {
    ufo_seq_data_t *ufo_seq_data = (ufo_seq_data_t*) data;
    free(ufo_seq_data);
}

int populate_integer_seq(uint64_t startValueIdx, uint64_t endValueIdx,
             ufPopulateCallout callout, ufUserData userData, char* target) {

    ufo_seq_data_t* data = (ufo_seq_data_t*) userData;

    for (size_t i = 0; i < endValueIdx - startValueIdx; i++) {
        ((int *) target)[i] = data->from + data->by * (i + startValueIdx);
    }

    return 0;
}

int populate_double_seq(uint64_t startValueIdx, uint64_t endValueIdx,
             ufPopulateCallout callout, ufUserData userData, char* target) {

    ufo_seq_data_t* data = (ufo_seq_data_t*) userData;

    for (size_t i = 0; i < endValueIdx - startValueIdx; i++) {
        ((double *) target)[i] = data->from + data->by * (i + startValueIdx);
    }

    return 0;
}

SEXP/*:result_type*/ ufo_seq(ufo_vector_type_t result_type, SEXP/*INTXP*/ from, SEXP/*INTXP*/ to, SEXP/*INTXP*/ by, SEXP/*INTSXP*/ min_load_count) {

    // make_sure(TYPEOF(from) == INTSXP, Rf_error, 
	// 		  "Parameter `from` must be an integer vector, but it is %s.", 
    //           type2char(TYPEOF(from)));

    // make_sure(TYPEOF(to) == INTSXP, Rf_error, 
	// 		  "Parameter `to` must be an integer vector, but it is %s.", 
    //           type2char(TYPEOF(to)));              

    // make_sure(TYPEOF(by) == INTSXP, Rf_error, 
	// 		  "Parameter `by` must be an integer vector, but it is %s.", 
    //           type2char(TYPEOF(by)));              

    ufo_source_t* source = (ufo_source_t*) malloc(sizeof(ufo_source_t));

    source->vector_type = result_type;
    source->element_size = __get_element_size(result_type);

    int from_value = __extract_int_or_die(from);
    int to_value = __extract_int_or_die(to);
    int by_value = __extract_int_or_die(by);
    int min_load_count_value = __extract_int_or_die(min_load_count);

    source->vector_size = (to_value - from_value) / by_value + 1;

    source->dimensions = NULL;
    source->dimensions_length = 0;

    source->min_load_count = __select_min_load_count(min_load_count_value, source->element_size);

    ufo_seq_data_t *data = (ufo_seq_data_t*) malloc(sizeof(ufo_seq_data_t));
    data->from = from_value;
    data->to = to_value;
    data->by = by_value;
    source->data = (ufUserData) data;

    source->destructor_function = &destroy_data;

    switch (result_type) {
    case UFO_INT:
        source->population_function = &populate_integer_seq;
        break;
    case UFO_REAL:
        source->population_function = &populate_double_seq;
        break;

    default:
        Rf_error("Cannot create sequence of type %s", type2char((SEXPTYPE) result_type));
    }

    ufo_new_t ufo_new = (ufo_new_t) R_GetCCallable("ufos", "ufo_new");
    return ufo_new(source);
}


SEXP/*INTXP*/ ufo_intsxp_seq(SEXP/*INTXP*/ from, SEXP/*INTXP*/ to, SEXP/*INTXP*/ by, SEXP/*INTSXP*/ min_load_count) {
    return ufo_seq(UFO_INT, from, to, by, min_load_count);
}


SEXP/*REALSXP*/ ufo_realsxp_seq(SEXP/*INTXP*/ from, SEXP/*INTXP*/ to, SEXP/*INTXP*/ by, SEXP/*INTSXP*/ min_load_count) {
    return ufo_seq(UFO_REAL, from, to, by, min_load_count);
}