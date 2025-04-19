#include "list.h"
#include "game.h"  // For Message definition
#include <stdlib.h>

void list_init(List* list) {
    list->head = NULL;
    list->tail = NULL;
    list->count = 0;
    pthread_mutex_init(&list->mutex, NULL);
}

void list_push(List* list, Message msg) {
    ListNode* new_node = malloc(sizeof(ListNode));
    new_node->data = msg;
    new_node->next = NULL;

    pthread_mutex_lock(&list->mutex);
    
    if (list->tail == NULL) {
        list->head = new_node;
        list->tail = new_node;
    } else {
        list->tail->next = new_node;
        list->tail = new_node;
    }
    list->count++;
    
    pthread_mutex_unlock(&list->mutex);
}



Message list_pop(List* list) {
    pthread_mutex_lock(&list->mutex);
    
    if (list->head == NULL) {
        pthread_mutex_unlock(&list->mutex);
        Message empty_msg = {0};
        return empty_msg;
    }
    
    ListNode* temp = list->head;
    Message msg = temp->data;
    
    list->head = list->head->next;
    if (list->head == NULL) {
        list->tail = NULL;
    }
    
    list->count--;
    pthread_mutex_unlock(&list->mutex);
    
    free(temp);
    return msg;
}

int list_count(List* list) {
    pthread_mutex_lock(&list->mutex);
    int count = list->count;
    pthread_mutex_unlock(&list->mutex);
    return count;
}

void list_clear(List* list) {
    pthread_mutex_lock(&list->mutex);
    while (list->head != NULL) {
        ListNode* temp = list->head;
        list->head = list->head->next;
        free(temp);
    }
    list->tail = NULL;
    list->count = 0;
    pthread_mutex_unlock(&list->mutex);
}

void list_free(List* list) {
    list_clear(list);
    pthread_mutex_destroy(&list->mutex);
} 