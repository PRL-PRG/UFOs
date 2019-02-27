#define USE_RINTERNALS
#include <R.h>
#include <Rinternals.h>
#include <R_ext/Rallocators.h>

#include <stdlib.h>

#include "maskirovka.h"

#define MASKIROVKA_DEBUG

typedef struct {
    size_t size;
    size_t header_size;
    size_t data_size;
    size_t length;
} cheater_allocator_data;

// Custom allocator function.
// FIXME Placeholder implementation.
void * cheater_malloc(R_allocator_t *allocator, size_t size) {
    cheater_allocator_data * allocator_data =
            ((cheater_allocator_data *)allocator->data);

    allocator_data->size = size;
    allocator_data->header_size = sizeof(SEXPREC_ALIGN);
    allocator_data->data_size = size - sizeof(SEXPREC_ALIGN);

#ifdef MASKIROVKA_DEBUG
    fprintf(stderr, "cheater alloc %ld + %ld = %ld, %ld elements\n",
           allocator_data->header_size, allocator_data->data_size,
           allocator_data->size, allocator_data->length);
#endif //MASKIROVKA_DEBUG

    return (void *) malloc(size);
}

// Custom deallocator function.
// FIXME Placeholder implementation.
void cheater_free(R_allocator_t *allocator, void * addr) {

#ifdef MASKIROVKA_DEBUG
    cheater_allocator_data * allocator_data =
            ((cheater_allocator_data *)allocator->data);

    fprintf(stderr, "cheater dealloc %ld + %ld = %ld, %ld elements\n",
           allocator_data->header_size, allocator_data->data_size,
           allocator_data->size, allocator_data->length);
#endif //MASKIROVKA_DEBUG

    free(addr);
}

// Function that creates a custom vector with a custom allocator.
SEXP/*INTSXP|VECSXP<INTSXP>*/ mask_new(SEXP/*INTSXP*/ lengths) {

    SEXP/*VECSXP<INTSXP>*/ results = allocVector(VECSXP, LENGTH(lengths)); // FIXME PROTECT

    // FIXME assuming lengths is a INTSXP for now.
    for (int i = 0; i < LENGTH(lengths); i++) {
        // Get length from the lengths vector.
        size_t length = INTEGER(lengths)[i];

        // Initialize an allocator.
        R_allocator_t *cheater_allocator =
                (R_allocator_t *) malloc(sizeof(R_allocator_t));

        // Initialize an allocator data struct.
        cheater_allocator_data *cheater_data =
                (cheater_allocator_data *) malloc(
                        sizeof(cheater_allocator_data));

        // Configure the allocator: provide function to allocate and free memory,
        // as well as a structure to keep the allocator's data.
        cheater_allocator->mem_alloc = &cheater_malloc;
        cheater_allocator->mem_free = &cheater_free;
        cheater_allocator->res; /* reserved, must be NULL */
        cheater_allocator->data = cheater_data; /* custom data */

        // Pass information to the allocator.
        cheater_data->length = length;

        // Create a new vector of the same type and length as the old vector.
        SEXP/*INTSXP*/ result =
                allocVector3(INTSXP, length, cheater_allocator);

        // If we receive only one length, then return a simple INTSXP vector.
        if (LENGTH(lengths) == 1) {
            return result;
        }

        // Add result to results;
        SET_VECTOR_ELT(results, i, result);
    }

    return results;
}

// TODO question: do we need to free the space used by the allocator and its
// paraphrenalia?



