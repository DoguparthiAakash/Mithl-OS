#include "list.h"
#include "memory.h" // Assuming you have a memory allocation library

// Create a new empty list
list_t *list_create(void)
{
    list_t *list = (list_t *)memory_alloc(sizeof(list_t)); // Use memory_alloc
    if (list)
    {
        list->head = NULL;
        list->tail = NULL;
        list->size = 0;
    }
    return list;
}

// Destroy the entire list, freeing all nodes
void list_destroy(list_t *list)
{
    list_node_t *current = list->head;
    while (current)
    {
        list_node_t *next = current->next;
        memory_free(current); // Use memory_free
        current = next;
    }
    memory_free(list); // Use memory_free
}
// Add an element to the end of the list
void list_append(list_t *list, void *data)
{
    list_node_t *node = (list_node_t *)memory_alloc(sizeof(list_node_t)); // Use memory_alloc
    if (!node)
        return;

    node->data = data;
    node->next = NULL;
    node->prev = list->tail;

    if (list->tail)
    {
        list->tail->next = node;
    }
    else
    {
        list->head = node;
    }
    list->tail = node;
    list->size++;
}

// Add an element to the beginning of the list
void list_prepend(list_t *list, void *data)
{
    list_node_t *node = (list_node_t *)memory_alloc(sizeof(list_node_t)); // Use memory_alloc
    if (!node)
        return;

    node->data = data;
    node->prev = NULL;
    node->next = list->head;

    if (list->head)
    {
        list->head->prev = node;
    }
    else
    {
        list->tail = node;
    }
    list->head = node;
    list->size++;
}

// Remove a specific node from the list
void list_remove_node(list_t *list, list_node_t *node)
{
    if (node->prev)
    {
        node->prev->next = node->next;
    }
    else
    {
        list->head = node->next;
    }

    if (node->next)
    {
        node->next->prev = node->prev;
    }
    else
    {
        list->tail = node->prev;
    }

    memory_free(node);
    list->size--;
}

// Remove the first element and return its data
void *list_pop_front(list_t *list)
{
    if (!list->head)
    {
        return NULL;
    }
    list_node_t *head = list->head;
    void *data = head->data;
    list_remove_node(list, head);
    return data;
}

// Get the first element's data
void *list_first(list_t *list)
{
    if (!list->head)
    {
        return NULL;
    }
    return list->head->data;
}