#ifndef LIST_H
#define LIST_H

#include "mouse.h" // For standard integer types like uint8_t

// Node structure for the doubly-linked list
typedef struct list_node
{
    void *data;             // Pointer to the actual data (e.g., gui_element_t)
    struct list_node *prev; // Pointer to the previous node
    struct list_node *next; // Pointer to the next node
} list_node_t;

// List structure, holding pointers to the head and tail
typedef struct
{
    list_node_t *head;
    list_node_t *tail;
    uint32_t size; // Optional: Keep track of the number of elements
} list_t;

// Function prototypes
list_t *list_create(void);
void list_destroy(list_t *list);

// Add an element to the end of the list
void list_append(list_t *list, void *data);

// Add an element to the beginning of the list
void list_prepend(list_t *list, void *data);

// Remove a specific node from the list
void list_remove_node(list_t *list, list_node_t *node);

// Remove the first element and return its data
void *list_pop_front(list_t *list);

// Get the first node's data
void *list_first(list_t *list);

// Iterate through the list. This macro makes traversal cleaner.
#define LIST_FOR_EACH(list, node) \
    for (list_node_t *node = (list)->head; node != NULL; node = node->next)

#endif // LIST_H