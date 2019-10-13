#pragma once

#include <stddef.h>
#include <stdint.h>

typedef void slab_t;

slab_t *slab_create(uint32_t numelems, size_t elemsize);
void *slab_alloc(slab_t *slab);
void slab_free(slab_t *slab, void *ptr);
void slab_destroy(slab_t *slab);
uint32_t slab_get_avail_elems(slab_t *slab);
