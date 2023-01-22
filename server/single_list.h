#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

struct list_node {
  void *data;
  struct list_node *next;
};

struct list_node_head {
  struct list_node *next;
};

bool single_list_is_empty(struct list_node_head *list_head) {
  return list_head->next != NULL;
}

void *single_list_pop(struct list_node_head *list_head) {
  if (NULL != list_head && NULL != list_head->next) {
    struct list_node *current_item = list_head->next;
    struct list_node *next_item = current_item->next;
    list_head->next = next_item;
    void *final_data = current_item->data;
    free(current_item);
    return final_data;
  }
  return NULL;
}

void single_list_push(struct list_node_head *list_head, void *item) {
  if (NULL == list_head) {
    return;
  }
  if (NULL == list_head->next) {
    struct list_node *entry =
        (struct list_node *)(malloc(sizeof(struct list_node)));
    entry->data = item;
    entry->next = NULL;
    list_head->next = entry;
  } else {
    struct list_node *element = list_head->next;
    while (element->next != NULL) {
      element = element->next;
    }
    struct list_node *entry =
        (struct list_node *)(malloc(sizeof(struct list_node)));
    entry->data = item;
    entry->next = NULL;
    element->next = entry;
  }
}

void *single_list_get_node_data(struct list_node *entry) { return entry->data; }

void single_list_delete_node(struct list_node_head *list_head,
                             struct list_node *target) {
  // Find the parent and child of the node.
  struct list_node *parent = list_head->next;
  struct list_node *child = target->next;

  // Clean up the target node's data. We're destructively deleting it so we
  // should free it.
  free(target->data);

  // Free mem used to create this node.
  free(target);

  // Set parent to point to the target's next node. This finishes removal.
  parent->next = child;
}

void single_list_delete_all(struct list_node_head *list_head) {
  if (NULL == list_head) {
    return;
  }
  while (list_head->next != NULL) {
    single_list_pop(list_head);
  }
}
