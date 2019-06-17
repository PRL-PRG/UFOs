#define USE_RINTERNALS
#define UFOS_DEBUG

#include <R.h>
#include <Rinternals.h>
#include <R_ext/Rallocators.h>

#include "ufos.h"
#include "ufo_sources.h"
#include "mappedMemory/userfaultCore.h"

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

void* __ufo_alloc(R_allocator_t *allocator, size_t size) {
    ufo_source_t* source = (ufo_source_t*) allocator->data;

    ufObject_t object;
    ufObjectConfig_t cfg = makeObjectConfig(int, size, 1); // FIXME

    int status = ufCreateObject(ufo_system, cfg, &object);
    __validate_status_or_die(status);

    ufSetPopulateFunction(cfg, source->population_function);
    ufSetUserConfig(cfg, source->data);

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
    return allocVector3(INTSXP, source->length, allocator);
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

__ufo_specific_vector_constructor __select_constructor(ufo_vector_type_t type) {
    switch (type) {
//        case UFO_CHAR:
//            return &__ufo_new_charsxp;
        case UFO_LGL:
            return &__ufo_new_lglsxp;
        case UFO_INT:
            return &__ufo_new_intsxp;
        case UFO_REAL:
            return &__ufo_new_realsxp;
        case UFO_CPLX:
            return &__ufo_new_cplxsxp;
        default:
            Rf_error("No available vector constructor for this type.");
    }
}

void __validate_source_or_die(SEXP/*EXTPTRSXP*/ source) {
    if (TYPEOF(source) != EXTPTRSXP) {
        Rf_error("Invalid source type: expecting EXTPTRSXP");
    }
    //ufo_source_t* source_details = CAR(source);
}

SEXP __ufo_new(SEXP/*EXTPTRSXP*/ source) {
    ufo_source_t* source_details = (ufo_source_t*) CAR(source);
    __ufo_specific_vector_constructor constructor =
            __select_constructor(source_details->vector_type);
    return constructor(source_details);
}

SEXP ufo_new(SEXP/*EXTPTRSXP*/ source) {
    __validate_source_or_die(source);
    return __ufo_new(source);
}

