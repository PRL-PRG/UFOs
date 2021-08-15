#include "ufo_csv.h"

#include <stdio.h>
#include <stdlib.h>

#include <stdbool.h>
#include <stdint.h>

#define USE_RINTERNALS
#include <R.h>
#include <Rinternals.h>

#include "../include/ufos.h"
#include "helpers.h"
#include "debug.h"

#include "csv/reader.h"
#include "evil/bad_strings.h"

typedef struct {
    const char*         path;
    size_t              column;
    scan_results_t     *metadata;
    uint32_t           *references_to_metadata;
    tokenizer_t        *tokenizer;
    size_t              initial_buffer_size;
} ufo_csv_column_source_t;


int32_t load_column_from_csv(void* user_data, uintptr_t start, uintptr_t end, unsigned char* target) {

    ufo_csv_column_source_t* data = (ufo_csv_column_source_t *) user_data;
    //size_t size_of_memory_fragment = end - start + 1;

    if (__get_debug_mode()) {
        REprintf("load_column_from_csv\n");
        REprintf("    start index: %li\n", start);
        REprintf("      end index: %li\n", end);
        REprintf("  target memory: %p\n", (void *) target);
        REprintf("    source file: %s\n", data->path);
        REprintf("         column: %li (%s)\n", data->column, data->metadata->column_names[data->column]);
        REprintf("    column type: %li (%s)\n", data->metadata->column_types[data->column],
                                                token_type_to_string(data->metadata->column_types[data->column]));
        REprintf("      cell size: %li\n", token_type_size(data->metadata->column_types[data->column]));
        REprintf("           rows: %li\n", data->metadata->rows);
    }

    size_t first_row = start;
    size_t last_row = end;

    read_results_t tokens = ufo_csv_read_column(data->tokenizer, data->path, data->column, data->metadata, first_row, last_row, data->initial_buffer_size);

    switch (data->metadata->column_types[data->column]) {
        case TOKEN_INTEGER: {
            int *ints = (int *) target;
            for (size_t i = 0; i < tokens.size; i++) {
                ints[i] = token_to_integer(tokens.tokens[i]);
            }
            break;
        }

        case TOKEN_INTERNED_STRING: {
            SEXP/*CHARSXP*/ *strings = (SEXP *) target;
            for (size_t i = 0; i < tokens.size; i++) {
                strings[i] = mkChar(tokens.tokens[i]->string);
            }
            break;
        }

        case TOKEN_STRING: {
            perror("Column type should be either TOKEN_FREE_STRING or TOKEN_INTERN_STRING. "
                   "Defaulting to TOKEN_FREE_STRING");
            /* no break */
        }

        case TOKEN_FREE_STRING: {
            SEXP/*CHARSXP*/ *strings = (SEXP *) target;

            for (size_t i = 0; i < tokens.size; i++) {
                strings[i] = mkBadChar(tokens.tokens[i]->string);
            }

            if (__get_debug_mode()) {
                REprintf("           created bad strings:\n\n");
                for (size_t i = 0; i < tokens.size; i++) {
                    REprintf("               %-20s %p\n", tokens.tokens[i]->string, strings[i]);
                }
                REprintf("\n");
            }

            break;
        }

        case TOKEN_BOOLEAN: {
            Rboolean *bools = (Rboolean *) target;
            for (size_t i = 0; i < tokens.size; i++) {
                bools[i] = token_to_logical(tokens.tokens[i]);
            }
            break;
        }

        case TOKEN_DOUBLE: {
            double *doubles = (double *) target;
            for (size_t i = 0; i < tokens.size; i++) {
                doubles[i] = token_to_numeric(tokens.tokens[i]);
            }
            break;
        }

        case TOKEN_EMPTY:
        case TOKEN_NA: {
            Rboolean *bools = (Rboolean *) target;
            for (size_t i = 0; i < tokens.size; i++) {
                bools[i] = NA_LOGICAL;
            }
            break;
        }

        default: perror("Not implemented");
    }

    return 0;
}

void destroy_column(void* user_data) {
    ufo_csv_column_source_t *data = (ufo_csv_column_source_t *) user_data;
    if (__get_debug_mode()) {
        REprintf("destroy_column\n");
        REprintf("    source file: %s\n", data->path);
        REprintf("         column: %li (%s)\n", data->column, data->metadata->column_names[data->column]);
        REprintf("    column type: %li (%s)\n", data->metadata->column_types[data->column],
                                                token_type_to_string(data->metadata->column_types[data->column]));
        REprintf("      cell size: %li\n", token_type_size(data->metadata->column_types[data->column]));
        REprintf("           rows: %li\n", data->metadata->rows);
    }

    if (*(data->references_to_metadata) == 1) {
        scan_results_free(data->metadata);
        tokenizer_free(data->tokenizer);
        free(data->references_to_metadata);
    } else {
        *(data->references_to_metadata) = *(data->references_to_metadata) - 1;
    }
    
    free(data);
}

SEXPTYPE token_type_to_sexp_type(token_type_t type) {
    switch (type) {
        case TOKEN_NA:
        case TOKEN_EMPTY:
        case TOKEN_BOOLEAN:         return LGLSXP;
        case TOKEN_INTEGER:         return INTSXP;
        case TOKEN_DOUBLE:          return REALSXP;
        case TOKEN_FREE_STRING:
        case TOKEN_INTERNED_STRING:
        case TOKEN_STRING:          return STRSXP;
        default:                    return -1;
    }
}

ufo_vector_type_t token_type_to_ufo_type(token_type_t type) {
    switch (type) {
        case TOKEN_NA:
        case TOKEN_EMPTY:
        case TOKEN_BOOLEAN:         return UFO_LGL;
        case TOKEN_INTEGER:         return UFO_INT;
        case TOKEN_DOUBLE:          return UFO_REAL;
        case TOKEN_FREE_STRING:
        case TOKEN_INTERNED_STRING:
        case TOKEN_STRING:          return UFO_STR;
        default:                    return -1;
    }
}

SEXP ufo_csv(SEXP/*STRSXP*/ path_sexp, SEXP/*LGLSXP*/ read_only_sexp, SEXP/*INTSXP*/ min_load_count_sexp, SEXP/*LGLSXP*/ headers_sexp, SEXP/*INTSXP*/ record_row_offsets_at_interval_sexp, SEXP/*INTSXP*/ initial_buffer_size_sexp, SEXP/*LGLSXP*/ add_class_to_columns_sexp) {

    bool headers = __extract_boolean_or_die(headers_sexp);
    bool read_only = __extract_boolean_or_die(read_only_sexp);
    bool add_class_to_columns = __extract_boolean_or_die(add_class_to_columns_sexp);
    const char *path = __extract_path_or_die(path_sexp);
    long record_row_offsets_at_interval = __extract_int_or_die(record_row_offsets_at_interval_sexp);
    size_t initial_buffer_size = __extract_int_or_die(initial_buffer_size_sexp);

    tokenizer_t *tokenizer = new_csv_tokenizer();
    scan_results_t *csv_metadata = ufo_csv_perform_initial_scan(tokenizer, path, record_row_offsets_at_interval, headers, initial_buffer_size);
    uint32_t *references_to_metadata = (uint32_t *) malloc(sizeof(uint32_t));
    *references_to_metadata = csv_metadata->columns;

    if (__get_debug_mode()) {
        REprintf("After initial scan of %s: \n\n", path);
        REprintf("    rows: %li\n", csv_metadata->rows);
        REprintf("    cols: %li\n", csv_metadata->columns);

        REprintf("    column_names:\n\n");
        for (size_t i = 0; i < csv_metadata->columns; i++) {
            REprintf("        [%li]: %s\n", i, csv_metadata->column_names[i]);
        }
        REprintf("\n");

        REprintf("    column_types:\n\n");
        for (size_t i = 0; i < csv_metadata->columns; i++) {
            REprintf("        [%li]: %s/%i\tSEXPTYPE=%i (%s)\n",
                     i, token_type_to_string(csv_metadata->column_types[i]),
                     csv_metadata->column_types[i],
                     token_type_to_sexp_type(csv_metadata->column_types[i]),
                     type2char(token_type_to_sexp_type(csv_metadata->column_types[i])));
        }
        REprintf("\n");

        REprintf("    row_offsets:\n\n");
        for (size_t i = 0; i < csv_metadata->row_offsets->size; i++) {
            REprintf("        [%li]: (row #%li): %li\n",
                     i, offset_record_human_readable_key(csv_metadata->row_offsets, i),
                     csv_metadata->row_offsets->offsets[i]);
        }
        REprintf("\n");
    }

    // Pre-intern strings
    for (size_t column = 0; column < csv_metadata->columns; column++) {
        if (csv_metadata->column_types[column] == TOKEN_STRING) {
            size_t limit = 5; //FIXME as parameter also FIXME make work
            string_set_t *unique_values = ufo_csv_read_column_unique_values(tokenizer, path, column, csv_metadata, limit, initial_buffer_size);
            //REprintf("    ... [%li]: %li vs %li\n\n", column, unique_values->size, limit);
            if (unique_values->size == limit) {
                if (__get_debug_mode()) {
                    REprintf("    cannot pre-intern strings for column [%li]: %li (or more) unique values\n\n",
                             column, unique_values->size);
                }

                csv_metadata->column_types[column] = TOKEN_FREE_STRING;
                continue;
            }

            csv_metadata->column_types[column] = TOKEN_INTERNED_STRING;

            if (__get_debug_mode()) {
                REprintf("    pre-interning %li strings for column [%li]:\n\n", unique_values->size, column);
            }

            for (size_t i = 0; i < unique_values->size; i++) {
                SEXP pointer = mkChar(unique_values->strings[i]); // FIXME PROTECT?

//#ifdef SWITCH_TO_REFCNT
//                SET_REFCNT(pointer, 1);
//#else
//                SET_NAMED(pointer, 1);
//#endif

                if (__get_debug_mode()) {
                    REprintf("        %-20s %p\n", unique_values->strings[i], pointer);
                }
            }

            if (__get_debug_mode()) {
                REprintf("\n");
            }
        }
    }

    SEXP/*VECSXP*/ data_frame = PROTECT(allocVector(VECSXP, csv_metadata->columns));
    if (__get_debug_mode()) {
        REprintf("Creating UFOs to insert into data frame SEXP at %p\n\n", data_frame);
    }

    for (size_t column = 0; column < csv_metadata->columns; column++) {

        ufo_csv_column_source_t *data = (ufo_csv_column_source_t *) malloc(sizeof(ufo_csv_column_source_t));
        ufo_source_t *source = (ufo_source_t *) malloc(sizeof(ufo_source_t));

        source->population_function = load_column_from_csv;
        source->destructor_function = destroy_column;
        source->data = (void*) data;
        source->vector_type = token_type_to_ufo_type(csv_metadata->column_types[column]);
        source->element_size = token_type_size(source->vector_type);
        source->vector_size = csv_metadata->rows;
        source->dimensions = 0;
        source->dimensions_length = 0;
        source->read_only = read_only;

        int32_t min_load_count = __extract_int_or_die(min_load_count_sexp);
        source->min_load_count = (min_load_count == 0) ? min_load_count : __1MB_of_elements(source->element_size);

        data->path = path;
        data->column = column;
        data->metadata = csv_metadata;
        data->references_to_metadata = references_to_metadata;
        data->tokenizer = tokenizer;
        data->initial_buffer_size = initial_buffer_size;

        ufo_new_t ufo_new = (ufo_new_t) R_GetCCallable("ufos", "ufo_new");
        SEXP/*UFO*/ vector = PROTECT(ufo_new(source));
        SET_VECTOR_ELT(data_frame, column, vector);

        if(add_class_to_columns) {
            setAttrib(vector, R_ClassSymbol, mkString("ufo"));
        }

        if (__get_debug_mode()) {
            REprintf("        [%li]: SEXP at   %p\n", column, vector);
        }        
    }
    if (__get_debug_mode()) {
        REprintf("\n");
    }

    setAttrib(data_frame, R_ClassSymbol, mkString("data.frame"));

    SEXP/*INTSXP*/ row_names = PROTECT(Rf_allocVector(INTSXP, csv_metadata->rows)); // FIXME use UFO here too
    for (size_t i = 0; i < XLENGTH(row_names); i++) {                               // FIXME int size?
        SET_INTEGER_ELT(row_names, i, i + 1);
    }
    setAttrib(data_frame, R_RowNamesSymbol, row_names);

    SEXP/*STRSXP*/ names = PROTECT(Rf_allocVector(STRSXP, csv_metadata->columns));
    for (size_t i =  0; i < XLENGTH(names); i++) {
        SET_STRING_ELT(names, i, mkChar(csv_metadata->column_names[i]));
    }
    setAttrib(data_frame, R_NamesSymbol, names);

    if (__get_debug_mode()) {
        //REprintf("          class     %p\n", column, );
        REprintf("          names     %p\n", names);
        REprintf("          row.names %p\n", row_names);
    }

    UNPROTECT(1 + csv_metadata->columns + 2);
    return data_frame;
}
