#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#define USE_RINTERNALS
#include <R.h>
#include <Rinternals.h>
#include <R_ext/Altrep.h>
#include <R_ext/Rallocators.h>

#include "altrep_ufo_vectors.h"
#include "R_ext.h"

int altrep_ufo_debug_mode = 0;

typedef struct {
    char const*         path;
    size_t              element_size;
    size_t              vector_size;
    SEXPTYPE            type;
    FILE*               file_handle;
    size_t              file_cursor;
} altrep_ufo_config_t;

static R_altrep_class_t ufo_integer_altrep;
static R_altrep_class_t ufo_numeric_altrep;
static R_altrep_class_t ufo_raw_altrep;
static R_altrep_class_t ufo_complex_altrep;
static R_altrep_class_t ufo_logical_altrep;

R_altrep_class_t __get_class_from_type(SEXPTYPE type) {
    switch (type) {
        case LGLSXP:
            return ufo_logical_altrep;
        case INTSXP:
            return ufo_integer_altrep;
        case REALSXP:
            return ufo_numeric_altrep;
        case CPLXSXP:
            return ufo_complex_altrep;
        case RAWSXP:
            return ufo_raw_altrep;
        default:
            Rf_error("Unrecognized vector type: %d\n", type);
    }
}

/**
 * Translates the type of UFO vector to the size of its element in bytes.
 *
 * @param vector_type The type of the vector as specified by the ufo_vector_type
 *                    enum.
 * @return Returns the size of the element vector or dies in case of an
 *         unrecognized vector type.
 */
size_t __get_element_size(SEXPTYPE vector_type) {
    switch (vector_type) {
        case CHARSXP:
            return sizeof(Rbyte);
        case LGLSXP:
            return sizeof(Rboolean);
        case INTSXP:
            return sizeof(int);
        case REALSXP:
            return sizeof(double);
        case CPLXSXP:
            return sizeof(Rcomplex);
        case RAWSXP:
            return sizeof(Rbyte);
        default:
            Rf_error("Unrecognized vector type: %d\n", vector_type);
    }
}

int __extract_boolean_or_die(SEXP/*STRSXP*/ sexp) {
    if (TYPEOF(sexp) != LGLSXP) {
        Rf_error("Invalid type for boolean vector: %d\n", TYPEOF(sexp));
    }

    if (LENGTH(sexp) == 0) {
        Rf_error("Provided a zero length vector for boolean vector\n");
    }

    if (LENGTH(sexp) > 1) {
        Rf_warning("Provided multiple values for boolean vector, "
                           "using the first one only\n");
    }

    int element = LOGICAL_ELT(sexp, 0);
    return element == 1;
}

const char* __extract_path_or_die(SEXP/*STRSXP*/ path) {
    if (TYPEOF(path) != STRSXP) {
        Rf_error("Invalid type for paths: %d\n", TYPEOF(path));
    }

    if (LENGTH(path) == 0) {
        Rf_error("Provided a zero length string for path\n");
    }

    if (TYPEOF(STRING_ELT(path, 0)) != CHARSXP) {
        Rf_error("Invalid type for path: %d\n", TYPEOF(STRING_ELT(path, 0)));
    }

    if (LENGTH(path) > 2) {
        Rf_warning("Provided multiple string values for path, "
                           "using the first one only\n");
    }

    return CHAR(STRING_ELT(path, 0));
}


long __get_vector_length_from_file_or_die(const char *path, size_t element_size) {

    // FIXME concurrency
    FILE* file = fopen(path, "rb");
    if (file == NULL) {
        Rf_error("Could not open file.\n");
    }

    int seek_status = fseek(file, 0L, SEEK_END);
    if (seek_status < 0) {
        // Could not seek in from file.
        fclose(file);
        Rf_error("Could not seek to the end of file.\n");
    }

    long file_size_in_bytes = ftell(file);
    fclose(file);

    if (file_size_in_bytes % element_size != 0) {
        Rf_error("File size not divisible by element size.\n");
    }

    return file_size_in_bytes / element_size;
}

int __load_from_file(int debug, uint64_t start, uint64_t end, altrep_ufo_config_t* cfg, char* target) {

    if (debug) {
        Rprintf("__load_from_file\n");
        Rprintf("    start index: %li\n", start);
        Rprintf("      end index: %li\n", end);
        Rprintf("  target memory: %p\n", (void *) target);
        Rprintf("    source file: %s\n", cfg->path);
        //      Rprintf("    vector type: %d\n", data->vector_type);
        Rprintf("    vector size: %li\n", cfg->vector_size);
        Rprintf("   element size: %li\n", cfg->element_size);
    }

    int initial_seek_status = fseek(cfg->file_handle, 0L, SEEK_END);
    if (initial_seek_status < 0) {
        // Could not seek in from file.
        fprintf(stderr, "Could not seek to the end of file.\n");
        return 1;
    }

    long file_size_in_bytes = ftell(cfg->file_handle);
    //fprintf(stderr, "file_size=%li\n", file_size_in_bytes);

    long start_reading_from = cfg->element_size * start;
    //fprintf(stderr, "start_reading_from=%li\n", start_reading_from);
    if (start_reading_from > file_size_in_bytes) {
        // Start index out of bounds of the file.
        fprintf(stderr, "Start index out of bounds of the file.\n");
        return 42;
    }

    long end_reading_at = cfg->element_size * end;
    //fprintf(stderr, "end_reading_at=%li\n", end_reading_at);
    if (end_reading_at > file_size_in_bytes) {
        // End index out of bounds of the file.
        fprintf(stderr, "End index out of bounds of the file.\n");
        return 43;
    }

    int rewind_seek_status = fseek(cfg->file_handle, start_reading_from, SEEK_SET);
    if (rewind_seek_status < 0) {
        // Could not seek in the file to position at start index.
        fprintf(stderr, "Could not seek in the file to position at start index.\n");
        return 2;
    }

    size_t read_status = fread(target, cfg->element_size, end - start, cfg->file_handle);
    if (debug) {
        Rprintf("    read status: %li\n", cfg->element_size);
    }
    if (read_status < end - start || read_status == 0) {
        // Read failed.
        fprintf(stderr, "Read failed. Read %li out of %li elements.\n", read_status, end - start);
        return 44;
    }

    return 0;
}

void __write_bytes_to_disk(const char *path, size_t size, const char *bytes) {
    //fprintf(stderr, "__write_bytes_to_disk(%s,%li,...)\n", path, size);

    FILE* file = fopen(path, "wb");

    if (!file) {
        fclose(file);
        Rf_error("Error opening file '%s'.", path);
    }

    size_t write_status = fwrite(bytes, sizeof(const char), size, file);
    if (write_status < size || write_status == 0) {
        fclose(file);
        Rf_error("Error writing to file '%s'. Written: %i out of %li",
                 path, write_status, size);
    }

    fclose(file);
}

FILE *__open_file_or_die(char const *path) {
    FILE * file = fopen(path, "rb");
    if (!file) {
        Rf_error("Could not open file.\n");
    }
    return file;
}

SEXP/*NILSXP*/ altrep_ufo_vectors_set_debug_mode(SEXP/*LGLSXP*/ debug) {
    altrep_ufo_debug_mode = __extract_boolean_or_die(debug);
    return R_NilValue;
}

void ufo_vector_finalize_altrep(SEXP wrapper) {
    altrep_ufo_config_t *cfg = (altrep_ufo_config_t *) EXTPTR_PTR(wrapper);
    fclose(cfg->file_handle);
}

// UFO constructors
SEXP ufo_vector_new_altrep(SEXPTYPE type, char const *path) {

    altrep_ufo_config_t *cfg =
            (altrep_ufo_config_t *) malloc(sizeof(altrep_ufo_config_t));

    cfg->type = type;
    cfg->path = path;
    cfg->element_size = __get_element_size(type);
    cfg->vector_size = __get_vector_length_from_file_or_die(cfg->path, cfg->element_size);
    cfg->file_handle = __open_file_or_die(cfg->path);
    cfg->file_cursor = 0;

    SEXP wrapper = allocSExp(EXTPTRSXP);
    EXTPTR_PTR(wrapper) = (void *) cfg;
    EXTPTR_TAG(wrapper) = Rf_install("ALTREP UFO CFG");

    SEXP ans = R_new_altrep(__get_class_from_type(type), wrapper, R_NilValue);
    EXTPTR_PROT(wrapper) = ans;

    // FIXME finalizer

    /* Finalizer */
    //SEXP extptr = PROTECT(R_MakeExternalPtr(NULL, R_NilValue, ans));
    R_MakeWeakRefC(wrapper, R_NilValue, ufo_vector_finalize_altrep, TRUE);

    return ans;
}
SEXP/*INTSXP*/ altrep_ufo_vectors_intsxp_bin(SEXP/*STRSXP*/ path) {
    return ufo_vector_new_altrep(INTSXP, __extract_path_or_die(path));
}
SEXP/*REALSXP*/ altrep_ufo_vectors_realsxp_bin(SEXP/*STRSXP*/ path) {
    return ufo_vector_new_altrep(REALSXP, __extract_path_or_die(path));
}
SEXP/*LGLSXP*/ altrep_ufo_vectors_lglsxp_bin(SEXP/*STRSXP*/ path) {
    return ufo_vector_new_altrep(LGLSXP, __extract_path_or_die(path));
}
SEXP/*CPLXSXP*/ altrep_ufo_vectors_cplxsxp_bin(SEXP/*STRSXP*/ path) {
    return ufo_vector_new_altrep(CPLXSXP, __extract_path_or_die(path));
}
SEXP/*RAWSXP*/ altrep_ufo_vectors_rawsxp_bin(SEXP/*STRSXP*/ path) {
    return ufo_vector_new_altrep(RAWSXP, __extract_path_or_die(path));
}

typedef struct {
    SEXPTYPE            type;
    /*R_len_t*/size_t   size;
    char const *        path;
    //size_t*             dimensions;
} altrep_ufo_source_t;

void* __ufo_altrep_alloc(R_allocator_t *allocator, size_t size) {
    altrep_ufo_source_t *data = (altrep_ufo_source_t*) allocator->data;
    return ufo_vector_new_altrep(data->type, data->path);
}

void* __ufo_altrep_free(R_allocator_t *allocator, void* ptr) {
    // FIXME free allocator
}

R_allocator_t* __altrep_ufo_new_allocator(char const *path) {
    // Initialize an allocator.
    R_allocator_t* allocator = (R_allocator_t*) malloc(sizeof(R_allocator_t));

    // Initialize an allocator data struct.
    altrep_ufo_source_t* data = (altrep_ufo_source_t*) malloc(sizeof(altrep_ufo_source_t));

    // Configure the allocator: provide function to allocate and free memory,
    // as well as a structure to keep the allocator's data.
    allocator->mem_alloc = &__ufo_altrep_alloc;
    allocator->mem_free = &__ufo_altrep_free;
    allocator->res; /* reserved, must be NULL */
    allocator->data = data; /* custom data: used for source */

    return allocator;
}

SEXP altrep_ufo_matrix_new_altrep(SEXPTYPE type, char const *path) {
    // Check type.
    if (type < 0) {
        Rf_error("No available vector constructor for this type.");
    }

    // Initialize an allocator.
    R_allocator_t* allocator = __altrep_ufo_new_allocator(path);

    // Create a new matrix of the appropriate type using the allocator.
    return allocMatrix3(type, dimensions[0], dimensions[1], allocator);
    }
}

static SEXP ufo_vector_duplicate(SEXP x, Rboolean deep) {

    altrep_ufo_config_t *new_cfg =
            (altrep_ufo_config_t *) malloc(sizeof(altrep_ufo_config_t));

    altrep_ufo_config_t *old_cfg =
            (altrep_ufo_config_t *) EXTPTR_PTR(R_altrep_data1(x));

    new_cfg->type = old_cfg->type;
    new_cfg->path = old_cfg->path;
    new_cfg->element_size = old_cfg->element_size;
    new_cfg->vector_size = old_cfg->vector_size;

    SEXP wrapper = allocSExp(EXTPTRSXP);
    EXTPTR_PTR(wrapper) = (void *) new_cfg;
    EXTPTR_TAG(wrapper) = Rf_install("ALTREP UFO CFG");

    if (R_altrep_data2(x) == R_NilValue) {
        return R_new_altrep(__get_class_from_type(new_cfg->type), wrapper, R_NilValue);

    } else {
        SEXP payload = PROTECT(allocVector(INTSXP, XLENGTH(R_altrep_data2(x))));

        for (int i = 0; i < XLENGTH(R_altrep_data2(x)); i++) {
            INTEGER(payload)[i] = INTEGER(R_altrep_data2(x))[i];
        }

        SEXP ans = R_new_altrep(__get_class_from_type(new_cfg->type), wrapper, payload);
        UNPROTECT(1);
        return ans;
    }
}

static Rboolean ufo_vector_inspect(SEXP x, int pre, int deep, int pvec, void (*inspect_subtree)(SEXP, int, int, int)) {

    if (R_altrep_data1(x) == R_NilValue) {
        Rprintf(" ufo_integer_altrep %s\n", type2char(TYPEOF(x)));
    } else {
        Rprintf(" ufo_integer_altrep %s (materialized)\n", type2char(TYPEOF(x)));
    }

    if (R_altrep_data1(x) != R_NilValue) {
        inspect_subtree(R_altrep_data1(x), pre, deep, pvec);
    }

    if (R_altrep_data2(x) != R_NilValue) {
        inspect_subtree(R_altrep_data2(x), pre, deep, pvec);
    }

    return FALSE;
}

static R_xlen_t ufo_vector_length(SEXP x) {
    if (altrep_ufo_debug_mode) {
        Rprintf("ufo_vector_Length\n");
        Rprintf("           SEXP: %p\n", x);
    }
    if (R_altrep_data2(x) == R_NilValue) {
        altrep_ufo_config_t *cfg =
                (altrep_ufo_config_t *) EXTPTR_PTR(R_altrep_data1(x));
        return cfg->vector_size;
    } else {
        return XLENGTH(R_altrep_data2(x));
    }
}

static void __materialize_data(SEXP x) {
    altrep_ufo_config_t *cfg =
            (altrep_ufo_config_t *) EXTPTR_PTR(R_altrep_data1(x));
    PROTECT(x);
    SEXP payload = allocVector(INTSXP, cfg->vector_size);
    int *data = INTEGER(payload);
    __load_from_file(altrep_ufo_debug_mode, 0, cfg->vector_size, cfg, (char *) data);
    R_set_altrep_data2(x, payload);
    UNPROTECT(1);
}

static void *ufo_vector_dataptr(SEXP x, Rboolean writeable) {
    if (altrep_ufo_debug_mode) {
        Rprintf("ufo_vector_Dataptr\n");
        Rprintf("           SEXP: %p\n", x);
        Rprintf("      writeable: %i\n", writeable);
    }
    if (writeable) {
        if (R_altrep_data2(x) == R_NilValue)
            __materialize_data(x);
        return DATAPTR(R_altrep_data1(x));
    } else {
        if (R_altrep_data2(x) == R_NilValue)
            __materialize_data(x);
        return DATAPTR(R_altrep_data1(x));
    }
}

static const void *ufo_vector_dataptr_or_null(SEXP x) {
    if (altrep_ufo_debug_mode) {
        Rprintf("ufo_vector_Dataptr_or_null\n");
        Rprintf("           SEXP: %p\n", x);
    }
    if (R_altrep_data2(x) == R_NilValue)
        __materialize_data(x);
    return DATAPTR_OR_NULL(R_altrep_data1(x));
}

static void ufo_vector_element(SEXP x, R_xlen_t i, void *target) {
    if (altrep_ufo_debug_mode) {
        Rprintf("ufo_vector_element\n");
        Rprintf("           SEXP: %p\n", x);
        Rprintf("          index: %li\n", i);
        Rprintf("         target: %p\n", target);
    }
    altrep_ufo_config_t *cfg =
            (altrep_ufo_config_t *) EXTPTR_PTR(R_altrep_data1(x));

    __load_from_file(altrep_ufo_debug_mode, i, i+1, cfg, (char *) target);
}

static int ufo_integer_element(SEXP x, R_xlen_t i) {
    if (R_altrep_data2(x) != R_NilValue)
        return INTEGER_ELT(R_altrep_data2(x), i);
    int ans = 0x5c5c5c5c;
    ufo_vector_element(x, i, &ans);
    return ans;
}

static double ufo_numeric_element(SEXP x, R_xlen_t i) {
    if (R_altrep_data2(x) != R_NilValue)
        return REAL_ELT(R_altrep_data2(x), i);
    double val = 0x5c5c5c5c;
    ufo_vector_element(x, i, &val);
    return val;
}

static Rbyte ufo_raw_element(SEXP x, R_xlen_t i) {
    if (R_altrep_data2(x) != R_NilValue)
        return RAW_ELT(R_altrep_data2(x), i);
    Rbyte val = 0;
    ufo_vector_element(x, i, &val);
    return val;
}

static Rcomplex ufo_complex_element(SEXP x, R_xlen_t i) {
    if (R_altrep_data2(x) != R_NilValue)
        return COMPLEX_ELT(R_altrep_data2(x), i);
    Rcomplex val = {0x5c5c5c5c, 0x5c5c5c5c};
    ufo_vector_element(x, i, &val);
    return val;
}

static int ufo_logical_element(SEXP x, R_xlen_t i) {
    if (R_altrep_data2(x) != R_NilValue)
        return LOGICAL_ELT(R_altrep_data2(x), i);
    Rboolean val = FALSE;
    ufo_vector_element(x, i, &val);
    return (int) val;
}

static R_xlen_t ufo_vector_get_region(SEXP x, R_xlen_t i, R_xlen_t n, void *buf) {
    if (altrep_ufo_debug_mode) {
        Rprintf("ufo_vector_get_region\n");
        Rprintf("           SEXP: %p\n", x);
        Rprintf("          index: %li\n", i);
        Rprintf("         length: %li\n", n);
        Rprintf("         target: %p\n", buf);
    }
    altrep_ufo_config_t *cfg =
            (altrep_ufo_config_t *) EXTPTR_PTR(R_altrep_data1(x));
    return __load_from_file(altrep_ufo_debug_mode, i, i+n, cfg, (char *) buf);
}

static R_xlen_t ufo_integer_get_region(SEXP x, R_xlen_t i, R_xlen_t n, int *buf) {
    if (R_altrep_data2(x) == R_NilValue)
        return INTEGER_GET_REGION(R_altrep_data2(x), i, n, buf);
    return ufo_vector_get_region(x, i, n, buf);
}

static R_xlen_t ufo_numeric_get_region(SEXP x, R_xlen_t i, R_xlen_t n, double *buf) {
    if (R_altrep_data2(x) == R_NilValue)
        return REAL_GET_REGION(R_altrep_data2(x), i, n, buf);
    return ufo_vector_get_region(x, i, n, buf);
}

static R_xlen_t ufo_raw_get_region(SEXP x, R_xlen_t i, R_xlen_t n, Rbyte *buf) {
    if (R_altrep_data2(x) == R_NilValue)
        return RAW_GET_REGION(R_altrep_data2(x), i, n, buf);
    return ufo_vector_get_region(x, i, n, buf);
}

static R_xlen_t ufo_complex_get_region(SEXP x, R_xlen_t i, R_xlen_t n, Rcomplex *buf) {
    if (R_altrep_data2(x) == R_NilValue)
        return COMPLEX_GET_REGION(R_altrep_data2(x), i, n, buf);
    return ufo_vector_get_region(x, i, n, buf);
}

static R_xlen_t ufo_logical_get_region(SEXP x, R_xlen_t i, R_xlen_t n, int *buf) {
    if (R_altrep_data2(x) == R_NilValue)
        return LOGICAL_GET_REGION(R_altrep_data2(x), i, n, buf);
    return ufo_vector_get_region(x, i, n, buf);
}



// UFO Inits
void init_ufo_integer_altrep_class(DllInfo * dll) {
    R_altrep_class_t cls = R_make_altinteger_class("ufo_integer_altrep",
                                                   "ufo_altrep", dll);
    ufo_integer_altrep = cls;

    /* Override ALTREP methods */
    R_set_altrep_Duplicate_method(cls, ufo_vector_duplicate);
    R_set_altrep_Inspect_method(cls, ufo_vector_inspect);
    R_set_altrep_Length_method(cls, ufo_vector_length);

    /* Override ALTVEC methods */
    R_set_altvec_Dataptr_method(cls, ufo_vector_dataptr);
    R_set_altvec_Dataptr_or_null_method(cls, ufo_vector_dataptr_or_null);

    /* Override ALTINTEGER methods */
    R_set_altinteger_Elt_method(cls, ufo_integer_element);
    R_set_altinteger_Get_region_method(cls, ufo_integer_get_region);
}

void init_ufo_numeric_altrep_class(DllInfo * dll) {
    R_altrep_class_t cls = R_make_altinteger_class("ufo_numeric_altrep",
                                                   "ufo_altrep", dll);
    ufo_numeric_altrep = cls;

    /* Override ALTREP methods */
    R_set_altrep_Duplicate_method(cls, ufo_vector_duplicate);
    R_set_altrep_Inspect_method(cls, ufo_vector_inspect);
    R_set_altrep_Length_method(cls, ufo_vector_length);

    /* Override ALTVEC methods */
    R_set_altvec_Dataptr_method(cls, ufo_vector_dataptr);
    R_set_altvec_Dataptr_or_null_method(cls, ufo_vector_dataptr_or_null);

    /* Override ALTREAL methods */
    R_set_altreal_Elt_method(cls, ufo_numeric_element);
    R_set_altreal_Get_region_method(cls, ufo_numeric_get_region);
}

void init_ufo_logical_altrep_class(DllInfo * dll) {
    R_altrep_class_t cls = R_make_altinteger_class("ufo_logical_altrep",
                                                   "ufo_altrep", dll);
    ufo_logical_altrep = cls;

    /* Override ALTREP methods */
    R_set_altrep_Duplicate_method(cls, ufo_vector_duplicate);
    R_set_altrep_Inspect_method(cls, ufo_vector_inspect);
    R_set_altrep_Length_method(cls, ufo_vector_length);

    /* Override ALTVEC methods */
    R_set_altvec_Dataptr_method(cls, ufo_vector_dataptr);
    R_set_altvec_Dataptr_or_null_method(cls, ufo_vector_dataptr_or_null);

    /* Override ALTLOGICAL methods */
    R_set_altlogical_Elt_method(cls, ufo_logical_element);
    R_set_altlogical_Get_region_method(cls, ufo_logical_get_region);
}

void init_ufo_complex_altrep_class(DllInfo * dll) {
    R_altrep_class_t cls = R_make_altinteger_class("ufo_complex_altrep",
                                                   "ufo_altrep", dll);
    ufo_complex_altrep = cls;

    /* Override ALTREP methods */
    R_set_altrep_Duplicate_method(cls, ufo_vector_duplicate);
    R_set_altrep_Inspect_method(cls, ufo_vector_inspect);
    R_set_altrep_Length_method(cls, ufo_vector_length);

    /* Override ALTVEC methods */
    R_set_altvec_Dataptr_method(cls, ufo_vector_dataptr);
    R_set_altvec_Dataptr_or_null_method(cls, ufo_vector_dataptr_or_null);

    /* Override ALTCOMPLEX methods */
    R_set_altcomplex_Elt_method(cls, ufo_complex_element);
    R_set_altcomplex_Get_region_method(cls, ufo_complex_get_region);
}

void init_ufo_raw_altrep_class(DllInfo * dll) {
    R_altrep_class_t cls = R_make_altinteger_class("ufo_raw_altrep",
                                                   "ufo_altrep", dll);
    ufo_raw_altrep = cls;

    /* Override ALTREP methods */
    R_set_altrep_Duplicate_method(cls, ufo_vector_duplicate);
    R_set_altrep_Inspect_method(cls, ufo_vector_inspect);
    R_set_altrep_Length_method(cls, ufo_vector_length);

    /* Override ALTVEC methods */
    R_set_altvec_Dataptr_method(cls, ufo_vector_dataptr);
    R_set_altvec_Dataptr_or_null_method(cls, ufo_vector_dataptr_or_null);

    /* Override ALTRAW methods */
    R_set_altraw_Elt_method(cls, ufo_raw_element);
    R_set_altraw_Get_region_method(cls, ufo_raw_get_region);
}