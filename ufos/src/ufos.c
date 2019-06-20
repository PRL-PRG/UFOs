#define USE_RINTERNALS
#define UFOS_DEBUG

#include <R.h>
#include <Rinternals.h>
#include <R_ext/Rallocators.h>

#include "ufos.h"
#include "mappedMemory/userfaultCore.h"

#include <assert.h>

ufInstance_t ufo_system;

typedef SEXP (*__ufo_specific_vector_constructor)(ufo_source_t*);

void ufo_initialize () {
    ufo_system = ufMakeInstance();
    ufInit(ufo_system);
}

void ufo_shutdown () {
    ufShutdown(ufo_system, 1 /*free=true*/);
}

void __validate_status_or_die (int status) {
    switch(status) {
        case 0: return;
        default:
            Rf_error("Could not create UFO.");
    }
}

uint32_t __get_stride_from_type_or_die(ufo_vector_type_t type) {
    switch(type) {
        case UFO_CHAR:
            return strideOf(Rbyte);
        case UFO_LGL:
            return strideOf(Rboolean);
        case UFO_INT:
            return strideOf(int);
        case UFO_REAL:
            return strideOf(double);
        case UFO_CPLX:
            return strideOf(Rcomplex);
        default:
            Rf_error("Cannot derive stride for vector type: %d\n", type);
    }
}

void* __ufo_alloc(R_allocator_t *allocator, size_t size) {
    ufo_source_t* source = (ufo_source_t*) allocator->data;

    size_t sexp_header_size = sizeof(SEXPREC_ALIGN);
    size_t sexp_metadata_size = sizeof(R_allocator_t);

    assert((size - sexp_header_size - sexp_metadata_size)
           == (source->vector_size *  sizeof(int)));

    ufObjectConfig_t cfg = makeObjectConfig0(sexp_header_size + sexp_metadata_size,
                                             source->vector_size,
                                             __get_stride_from_type_or_die(source->vector_type),
                                             16);

    ufSetPopulateFunction(cfg, source->population_function);
    ufSetUserConfig(cfg, source->data);

    ufObject_t object;
    int status = ufCreateObject(ufo_system, cfg, &object);
    __validate_status_or_die(status);

    //fprintf(stderr, "header ptr %p\n", ufGetHeaderPointer(object));
    //fprintf(stderr, "value ptr  %p\n", ufGetValuePointer(object));

    return ufGetHeaderPointer(object); // FIXME
}

void __ufo_free(R_allocator_t *allocator, void * ptr) {
    // FIXME translate from ptr to object

}

SEXP __ufo_new_anysxp(SEXPTYPE type, ufo_source_t* source) {
    // Initialize an allocator.
    R_allocator_t* allocator = (R_allocator_t*) malloc(sizeof(R_allocator_t));

    // Initialize an allocator data struct.
    ufo_source_t* data = (ufo_source_t*) malloc(sizeof(ufo_source_t));

    // Configure the allocator: provide function to allocate and free memory,
    // as well as a structure to keep the allocator's data.
    allocator->mem_alloc = &__ufo_alloc;
    allocator->mem_free = &__ufo_free;
    allocator->res; /* reserved, must be NULL */
    allocator->data = source; /* custom data: used for source */

    // Create a new vector of the appropriate type using the allocator.
    return allocVector3(type, source->vector_size, allocator);
}

//SEXP/*CHARSXP*/ __ufo_new_lglsxp(ufo_source_t* source) {
//    return __ufo_new_anysxp(LGLSXP, source);
//}

SEXP/*INTSXP*/ __ufo_new_intsxp(ufo_source_t* source) {
    return __ufo_new_anysxp(INTSXP, source);
}

SEXP/*REALSXP*/ __ufo_new_realsxp(ufo_source_t* source) {
    return __ufo_new_anysxp(REALSXP, source);
}

SEXP/*LGLSXP*/ __ufo_new_lglsxp(ufo_source_t* source) {
    return __ufo_new_anysxp(LGLSXP, source);
}

SEXP/*CPLXSXP*/ __ufo_new_cplxsxp(ufo_source_t* source) {
    return __ufo_new_anysxp(CPLXSXP, source);
}

SEXPTYPE ufo_type_to_vector_type (ufo_vector_type_t ufo_type) {
    switch (ufo_type) {
//        case UFO_CHAR:
//            return &__ufo_new_charsxp;
        case UFO_LGL:
            return LGLSXP;
        case UFO_INT:
            return INTSXP;
        case UFO_REAL:
            return REALSXP;
        case UFO_CPLX:
            return CPLXSXP;
        default:
            return -1;
    }
}

SEXP ufo_new(ufo_source_t* source) {

    SEXPTYPE type = ufo_type_to_vector_type(source->vector_type);
    if (type < 0) {
        Rf_error("No available vector constructor for this type.");
    }
    return __ufo_new_anysxp(type, source);
}

