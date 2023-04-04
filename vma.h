#pragma once
#include <inttypes.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define STRING_MAX 400

typedef struct dll_node_t dll_node_t;
struct dll_node_t {
  void* data;
  dll_node_t *prev, *next;
};
typedef struct {
  dll_node_t* head;
  unsigned int data_size;
  unsigned int size;
} list_t;

typedef struct {
  uint64_t start_address;
  size_t size;
  void* miniblock_list;
} block_t;

typedef struct {
  uint64_t start_address;
  size_t size;
  uint8_t perm;
  void* rw_buffer;
} miniblock_t;

typedef struct {
  uint64_t arena_size;
  list_t* alloc_list;
  uint64_t used_size;
} arena_t;

list_t* dll_create(unsigned int data_size);
void dll_add_nth_node(list_t* list, unsigned int n, const void* new_data);
dll_node_t* dll_remove_nth_node(list_t* list, unsigned int n);

arena_t* alloc_arena(const uint64_t size);
void dealloc_arena(arena_t* arena);

int is_node_start(arena_t* arena, const uint64_t address);
int is_node_final(arena_t* arena, const uint64_t address);
int poz(list_t* list, const uint64_t address);
dll_node_t* get_node(list_t* list, const uint64_t address);
void dll_free(list_t** pp_list);

void alloc_block(arena_t* arena, const uint64_t address, const uint64_t size);
void free_block(arena_t* arena, const uint64_t address);

void read(arena_t* arena, uint64_t address, uint64_t size);
void write(arena_t* arena, const uint64_t address, uint64_t size);
void pmap(const arena_t* arena);
void mprotect(arena_t* arena, uint64_t address);