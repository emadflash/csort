#include <stdio.h>
#include <stdlib.h>
#include "core.h"

void* CSort_malloc(u32 mem_size) {
    void* mem = malloc(mem_size);
    fprintf(stdout, "Allocated %d at 0x%p", mem_size, mem);
    if (! mem) {
        DEV_panic("malloc");
    }
    return mem;
}

void* CSort_realloc(void* mem, u32 mem_size) {
    void* mem2 = realloc(mem, mem_size);
    fprintf(stdout, "ReAllocated %d at 0x%p -> 0x%p", mem_size, mem, mem2);
    if (! mem) {
        DEV_panic("realloc");
    }

    return mem2;
}


typedef struct CSortMemArenaNode CSortMemArenaNode;
struct CSortMemArenaNode {
    void* mem;
    void* mem_cursor;
    u32 mem_used,
        mem_free,
        mem_size;

    void (*fill)(struct CSortMemArenaNode* arena);
    struct CSortMemArenaNode* next;
};


CSortMemArenaNode CSortMemArenaNode_mk() {
    CSortMemArenaNode node = {0};
    node.mem_size = 512;
    node.mem = (void*) CSort_malloc(node.mem_size);
    node.mem_cursor = node.mem;
    node.mem_free = 512;
    node.mem_used = 0;

    return node;
}

void CSortMemArenaNode_free(CSortMemArenaNode* node) {
    free(node->mem);
}


void CSortMemArenaNode_fill(CSortMemArenaNode* node, void* data, u32 data_size) {
    if (data_size >= node->mem_free) {
        node->mem_size += 512;
        node->mem_free += 512;
        node->mem = (void*) CSort_realloc(node->mem, node->mem_size);
    }

    memcpy(node->mem_cursor, data, data_size);
    node->mem_cursor += data_size;
    node->mem_used = data_size;
}


typedef struct CSortMemArena CSortMemArena;
struct CSortMemArena {
    CSortMemArenaNode* nodes;
    CSortMemArenaNode* head;     
};

CSortMemArena CSortMemArena_mk() {
    return (CSortMemArena) {
        .nodes = NULL,
        .head = NULL,     
    };
}

CSortMemArenaNode* CSortMemArena_alloc(CSortMemArena* arena) {
    CSortMemArenaNode* node = DEV_malloc(1, sizeof(CSortMemArenaNode));
    if (arena->head == NULL) {
        arena->head = node;
    } else {
        arena->head->next = node;
        arena->head = node;
    }
    *node =  CSortMemArenaNode_mk();
    return node;
}

void CSortMemArena_dealloc(CSortMemArenaNode* node) {
}

int main() {
    CSortMemArenaNode node = CSortMemArenaNode_mk();
    return 0;
}
