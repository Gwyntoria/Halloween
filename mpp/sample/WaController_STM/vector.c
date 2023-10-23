#include "vector.h"
#include <stdlib.h>
#include <string.h> 

typedef unsigned char byte;

typedef struct Vector_ {
    size_t count;
    size_t max_count;
    size_t elem_size;
    byte* data;
} Vector;

#define vector_max_size(v) ((v->elem_size)*(v->max_count))

Vector* vector_new(size_t elem_size) {
    Vector* v = (Vector*)calloc(1, sizeof(Vector));
    if (v) v->elem_size = elem_size;
    return v;
}

void vector_free(Vector* v) {
    if(v->data) free(v->data);
    free(v);
}

size_t vector_length(Vector* v) {
    return v->count;
}

void vector_get(Vector* v, size_t pos, void* elem_out) {
    if (pos < v->count) {
        byte* p = v->data + v->elem_size * pos;
        memcpy(elem_out, p, v->elem_size);
    }
}

void vector_set(Vector* v, size_t pos, void* elem_in) {
    if (pos < v->count) {
        byte* p = v->data + v->elem_size * pos;
        memcpy(p, elem_in, v->elem_size);
    }
}

int vector_append(Vector* v, void* elem_in) {
    if (v->count >= v->max_count) {
        byte* data;
        v->max_count = (v->max_count)?(v->max_count*2):(4);
        data = (byte*)realloc(v->data, vector_max_size(v));
        if (!data) return -1;
        v->data = data;
    }
    vector_set(v, v->count++, elem_in);
    return 0;
}