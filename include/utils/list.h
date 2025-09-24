#include <stddef.h>
#include <stdbool.h>

typedef struct {
    void* data;
    struct list_node* next;
    struct list_node* prev;
} list_node;

typedef struct {
    list_node* head;
    list_node* tail;
    size_t size;
} list;