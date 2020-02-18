#include "../include/ufos.h"
#include "../include/mappedMemory/userfaultCore.h"
#include "ufoseq.h"

void destroy_data(ufUserData *data) {
    ufo_seq_data_t *ufo_seq_data = (ufo_seq_data_t*) data;
    free(ufo_seq_data);
}

int populate(uint64_t startValueIdx, uint64_t endValueIdx,
             ufPopulateCallout callout, ufUserData userData, char* target) {

    ufo_seq_data_t* data = (ufo_seq_data_t*) userData;

    for (size_t i = 0; i < endValueIdx - startValueIdx; i++) {
        ((int *) target)[i] = data->from + data->by * (i + startValueIdx);
    }

    return 0;
}

SEXP/*INTXP*/ ufo_seq(SEXP/*INTXP*/ from, SEXP/*INTXP*/ to, SEXP/*INTXP*/ by) {

    ufo_source_t* source = (ufo_source_t*) malloc(sizeof(ufo_source_t));

    source->vector_type = UFO_INT;
    source->element_size = sizeof(int);

    int from_value = INTEGER_ELT(from, 0);
    int to_value = INTEGER_ELT(to, 0);
    int by_value = INTEGER_ELT(by, 0);

    source->vector_size = (to_value - from_value) / by_value + 1;

    source->dimensions = NULL;
    source->dimensions_length = 0;

    source->min_load_count = 1000000 / sizeof(int);

    ufo_seq_data_t *data = (ufo_seq_data_t*) malloc(sizeof(ufo_seq_data_t));
    data->from = from_value;
    data->to = to_value;
    data->by = by_value;
    source->data = (ufUserData) data;

    source->destructor_function = &destroy_data;
    source->population_function = &populate;

    ufo_new_t ufo_new = (ufo_new_t) R_GetCCallable("ufos", "ufo_new");
    return ufo_new(source);
}