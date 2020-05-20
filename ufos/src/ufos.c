#define USE_RINTERNALS

#include <R.h>
#include <Rinternals.h>
#include <R_ext/Rallocators.h>

#include "ufos.h"
#include "R_ext.h"
#include "mappedMemory/userfaultCore.h"

#include <assert.h>

ufInstance_t __ufo_system;
int __framework_initialized = 0;

typedef SEXP (*__ufo_specific_vector_constructor)(ufo_source_t*);

SEXP ufo_shutdown() {
    if (__framework_initialized) {
        __framework_initialized = 0;

        // Actual shutdown
        int result = ufShutdown(__ufo_system, 1 /*free=true*/);
        if (result != 0) {
            Rf_error("Error shutting down the UFO framework (%i)", result);
        }
    }
    return R_NilValue;
}

SEXP ufo_initialize() {
    if (!__framework_initialized) {
        __framework_initialized = 1;

        // Actual initialization
        __ufo_system = ufMakeInstance();
        if (__ufo_system == NULL) {
            Rf_error("Error initializing the UFO framework (null instance)");
        }
        int result = ufInit(__ufo_system);
        if (result != 0) {
            Rf_error("Error initializing the UFO framework (%i)", result);
        }
        size_t high = 200 * 1024 * 1024; // * 1024; for testing: 200MB
        size_t low = 100 * 1024 * 1024;  // * 1024; for testing: 100MB
        result = ufSetMemoryLimits(__ufo_system, high, low);
        if (result != 0) {
            Rf_error("Error setting memory limits for the UFO framework (%i)", result);
        }
    }
    return R_NilValue;
}

void __validate_status_or_die (int status) {
    switch(status) {
        case 0: return;
        default:
            Rf_error("Could not create UFO (%i)", status);
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
        case UFO_RAW:
            return strideOf(Rbyte);
        case UFO_STR:
            return strideOf(SEXP);
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
                                             source->min_load_count);

    ufSetPopulateFunction(cfg, source->population_function);
    ufSetUserConfig(cfg, source->data);

    ufObject_t object;
    int status = ufCreateObject(__ufo_system, cfg, &object);
    __validate_status_or_die(status);

    //fprintf(stderr, "header ptr %p\n", ufGetHeaderPointer(object));
    //fprintf(stderr, "value ptr  %p\n", ufGetValuePointer(object));

    return ufGetHeaderPointer(object); // FIXME
}

void __ufo_free(R_allocator_t *allocator, void* ptr) {
    ufObject_t* object = ufLookupObjectByMemberAddress(__ufo_system, ptr);
    if (object == NULL) {
        Rf_error("Tried freeing a UFO, "
                 "but the provided address is not a UFO header address.");
    }
    ufo_source_t* source = (ufo_source_t*) allocator->data;
    source->destructor_function(source->data);
    ufDestroyObject(object);
    if (source->dimensions != NULL) {
        free(source->dimensions);
    }
    free(source);
}

SEXPTYPE ufo_type_to_vector_type (ufo_vector_type_t ufo_type) {
    switch (ufo_type) {
        case UFO_CHAR:
            return CHARSXP;
        case UFO_LGL:
            return LGLSXP;
        case UFO_INT:
            return INTSXP;
        case UFO_REAL:
            return REALSXP;
        case UFO_CPLX:
            return CPLXSXP;
        case UFO_RAW:
            return RAWSXP;
        case UFO_STR:
            return STRSXP;
        default:
            printf("Cannot convert ufo_type_t=%i to SEXPTYPE", ufo_type);
            return -1;
    }
}

R_allocator_t* __ufo_new_allocator(ufo_source_t* source) {
    // Initialize an allocator.
    R_allocator_t* allocator = (R_allocator_t*) malloc(sizeof(R_allocator_t));

    // Initialize an allocator data struct.
    //ufo_source_t* data = (ufo_source_t*) malloc(sizeof(ufo_source_t));

    // Configure the allocator: provide function to allocate and free memory,
    // as well as a structure to keep the allocator's data.
    allocator->mem_alloc = &__ufo_alloc;
    allocator->mem_free = &__ufo_free;
    allocator->res = NULL; /* reserved, must be NULL */
    allocator->data = source; /* custom data: used for source */

    return allocator;
}

int __vector_will_be_scalarized(SEXPTYPE type, size_t length) {
    return length == 1 && (type == REALSXP || type == INTSXP || type == LGLSXP);
}

// FIXME This is copied from userfaultCore. Maybe userfault can expose some sort
// of callout stub function for me?
int __callout_stub(ufPopulateCalloutMsg* msg){
    switch(msg->cmd){
        case ufResolveRangeCmd:
            return 0; // Not yet implemented, but this is advisory only so no
                      // error
        case ufExpandRange:
            return ufWarnNoChange; // Not yet implemented, but callers have to
                                   // deal with this anyway, even spuriously
        default:
            return ufBadArgs;
    }
    __builtin_unreachable();
}

void __prepopulate_scala(SEXP scalar, ufo_source_t* source) {
    source->population_function(0, source->vector_size, __callout_stub,
                                source->data, DATAPTR(scalar));
}

SEXP ufo_new(ufo_source_t* source) {
    // Check type.
    SEXPTYPE type = ufo_type_to_vector_type(source->vector_type);
    if (type < 0) {
        Rf_error("No available vector constructor for this type.");
    }

    // Initialize an allocator.
    R_allocator_t* allocator = __ufo_new_allocator(source);

    // Create a new vector of the appropriate type using the allocator.
#ifdef USE_R_HACKS
    SEXP ufo = PROTECT(allocVectorIII(type, source->vector_size, allocator, 1));
#else
    SEXP ufo = PROTECT(allocVector3(type, source->vector_size, allocator));
#endif

    // Workaround for scalar vectors ignoring custom allocator:
    // Pre-load the data in, at least it'll work as read-only.
    if (__vector_will_be_scalarized(type, source->vector_size)) {
        __prepopulate_scala(ufo, source);
    }

    UNPROTECT(1);
    return ufo;
}

SEXP ufo_new_multidim(ufo_source_t* source) {
    // Check type.
    SEXPTYPE type = ufo_type_to_vector_type(source->vector_type);
    if (type < 0) {
        Rf_error("No available vector constructor for this type.");
    }

    // Initialize an allocator.
    R_allocator_t* allocator = __ufo_new_allocator(source);

    // Create a new matrix of the appropriate type using the allocator.
    SEXP ufo = PROTECT(allocMatrix3(type, source->dimensions[0],
                                    source->dimensions[1], allocator));

    // Workaround for scalar vectors ignoring custom allocator:
    // Pre-load the data in, at least it'll work as read-only.
    if (__vector_will_be_scalarized(type, source->vector_size)) {
        __prepopulate_scala(ufo, source);
    }

    UNPROTECT(1);
    return ufo;
}