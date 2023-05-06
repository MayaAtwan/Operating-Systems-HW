#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include "concurrent_list.h"

struct node {
    int value;
    node* next;
    pthread_mutex_t locked;
};

struct list {
    node* head;
    pthread_mutex_t locked;
};

void print_node(node* node)
{
    // DO NOT DELETE
    if (node)
    {
        printf("%d ", node->value);
    }
}

// this function creates a new embty linked list, the return value:pointer to new linked list.
list* create_list()
{
    //allocate memory
    list* List = (list*)malloc(sizeof(list));
    //check if malloc failed
    if (List == NULL)
        exit(1);
    // if we are here then malloc succeded, so initializes the head to Null
    List->head = NULL;
    // initilize the lock and check it
    if (pthread_mutex_init(&List->locked, NULL) != 0)
    {
        free(List);
        exit(1);
    }
    //returns a pointer to the new List struct
    return List;
}

//Deletes a linked list and frees all of its associated memory.
void delete_list(list* list)
{
    //if the list in null,there is nothing to delete
    if (list == NULL)
        return;
    node* node_next;
    node* curr = list->head;
    // If the list is empty, free the memory associated with the list itself
    if (curr == NULL)
    {
        // Destroy the mutex associated with the list and free the memory
        pthread_mutex_destroy(&list->locked);
        free(list);
        return;
    }
    while (curr)
    {
        // Keep track of the next node before freeing the current one
        node_next = curr->next;
        // Destroy the mutex associated with the current node and free the memory
        pthread_mutex_destroy(&curr->locked);
        free(curr);
        // Update the curr
        curr = node_next;
    }
    pthread_mutex_destroy(&list->locked);
    free(list);
}


// in this function we allocating memory for the node, checking if malloc failed,if not
//initialize the fields of the node.
node* create_node(int value)
{
    node* new_node = (node*)malloc(sizeof(node));
    if (new_node == NULL)
    {
        exit(1);
    }
    new_node->value = value;
    new_node->next = NULL;
    // check if init failed
    if (pthread_mutex_init(&new_node->locked, NULL) != 0)
    {
        free(new_node);
        exit(1);
    }
    return new_node;
}


//Inserts a new node into the linked list in sorted order, with the given value.
void insert_value(list* list, int value)
{
    if (list) {/*make sure they did not pass a NULL as list*/
        // create new node through the function create_node
        node* new_node = create_node(value);
        node* prev = NULL;
        node* curr = NULL;

        // locked the list to check the head, we locked the list in 2 cases if we must to change the head
        // or if the list is empty
        pthread_mutex_lock(&list->locked);

        //if the list is empty so the insert node become the head of the list.
        if (list->head == NULL)
        {
            new_node->next = NULL;
            list->head = new_node;
            pthread_mutex_unlock((&list->locked));

        }

        //if the new node is the new head(minimal value)
        // we locked the head and the list to change the head to the new_node
        else if (value <= list->head->value)
        {
            pthread_mutex_lock((&(list->head)->locked));

            new_node->next = list->head;
            list->head = new_node;
            pthread_mutex_unlock((&new_node->next->locked));
            pthread_mutex_unlock(&list->locked);
        }
        else
        {   // if we are here then we must to insert the new node between 2 nodes or at the end
            //find where to insert the new node, we used hand over hand locking method.
            prev = list->head;
            curr = prev->next;
            pthread_mutex_lock(&(prev->locked));
            pthread_mutex_unlock(&list->locked);
            while (curr != NULL && curr->value <= value)
            {
                pthread_mutex_lock(&(curr->locked));
                pthread_mutex_unlock(&(prev->locked));
                prev = curr;
                curr = prev->next;
            }
            //insert the new node BEFORE the current node (between prev and curr)
            new_node->next = curr;
            prev->next = new_node;
            pthread_mutex_unlock(&(prev->locked));
            //If curr is not NULL, it was locked in the while loop and should be unlocked now
            if (curr != NULL) {
                pthread_mutex_unlock(&(curr->locked));
            }
        }
    }
}

void remove_value(list* list, int value)
{
    node* prev = NULL;
    node* curr = NULL;
    //if the list is NULL
    if (!list)
    {
        return;
    }
    pthread_mutex_lock(&(list->locked));
    //if the list is empty
    if (!(list->head))
    {
        pthread_mutex_unlock(&list->locked);
        return;
    }
    else{
        prev = list->head;
        curr = prev->next;
        pthread_mutex_lock(&(list->head)->locked);
        pthread_mutex_unlock(&list->locked);
        //if the wanted node is the head(minimal value)
        if (value == prev->value)
        {
            list->head = curr;
            pthread_mutex_unlock(&prev->locked);
            pthread_mutex_destroy(&prev->locked);
            free(prev);
        }
        else
        {
            //find the wanted node.
            while (curr && curr->value != value)
            {
                pthread_mutex_lock(&(curr->locked));
                pthread_mutex_unlock(&(prev->locked));
                prev = curr;
                curr = prev->next;
            }
            //delete the wanted node (curr)
            if (curr) {
                prev->next = curr->next;
                pthread_mutex_unlock(&(prev->locked));
                pthread_mutex_destroy(&curr->locked);
                free(curr);
            }//if it's not found
            else { pthread_mutex_unlock(&(prev->locked)); }

        }
    }
}

// print function ~ print the list
void print_list(list* list)
{
    //if the list is NULL
    if (list == NULL)
    {
        printf("\n");
        return;
    }
    pthread_mutex_lock(&list->locked);
    //if the list is empty
    if (list->head == NULL)
    {
        pthread_mutex_unlock(&list->locked);
        printf("\n");
        return;
    }
    // using hand over hand locking
    // each time we lock one node print it then lock the next after that,unlock the node
    //1)lock curr node.
    //2)print curr node.
    //3)lock curr->next
    //4)unlock curr node
    //
    node* curr = list->head;
    node* next;
    pthread_mutex_lock(&(curr->locked));
    pthread_mutex_unlock(&list->locked);
    while (curr != NULL) {
        print_node(curr);
        next = curr->next;
        if (next != NULL)
        {
            pthread_mutex_lock(&(next->locked));
        }
        pthread_mutex_unlock(&(curr->locked));
        curr = next;
    }

    printf("\n"); // DO NOT DELETE
    return;
}

void count_list(list* list, int (*predicate)(int))
{
    int count = 0; // DO NOT DELETE
    //if the list is NULL
    if (!list) {
        printf("%d items were counted\n", count);
        return;
    }
    pthread_mutex_lock(&list->locked);
    //if the list is empty
    if (!list->head) {
        printf("%d items were counted\n", count);
        pthread_mutex_unlock(&list->locked);
        return;

    }
    else {
        node* curr = list->head;
        node* next;
        //// using hand over hand locking
        pthread_mutex_lock(&(curr->locked));
        pthread_mutex_unlock(&list->locked);
        while (curr != NULL) {
            if (predicate(curr->value)) {
                count++;
            }
            next = curr->next;
            if (next != NULL)
            {
                pthread_mutex_lock(&(next->locked));
            }
            pthread_mutex_unlock(&(curr->locked));
            curr = next;
        }
    }

    printf("%d items were counted\n", count); // DO NOT DELETE
}
