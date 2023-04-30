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
};

void print_node(node* node)
{
    // DO NOT DELETE
    if (node)
    {
        printf("%d ", node->value);
    }
}


// this function creates a new embty linked list, the return value:pointer to  new linked list.
list* create_list()
{
    //allocate memory
    list* List = (list*)malloc(sizeof(list));
    //check if malloc failed
    if (List == NULL)
        exit(1);
    // if we are here then malloc succeded, so initializes the head to Null
    List->head = NULL;
    //returns a pointer to the new List struct

    return List;
}

void delete_list(list* list)
{
    //if the list in null
    if (list == NULL)
        return;
    node* node_next;
    node* curr = list->head;
    //if curr was removed, so check that there is element in the curr(if the list is empty)
    if (curr == NULL)
    {
        free(list);
        return;
    }
    // if we are here then the node must to be deleted so
    //locked it
    pthread_mutex_lock(&curr->locked);
    while (curr)
    {
        node_next = curr->next;
        if (node_next)
        {
            pthread_mutex_lock(&node_next->locked);
        }
        pthread_mutex_unlock(&curr->locked);
        pthread_mutex_destroy(&curr->locked);
        free(curr);
        curr = node_next;
    }
    list->head = NULL;
    free(list);

}


//the new node is not yet part of the list and there is no contention for locks.
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

void insert_value(list* list, int value)
{
    if (list) {/*make sure they did not pass a NULL as list*/
        node* new_node = create_node(value);
        node* prev = NULL;
        node* curr = NULL;

        pthread_mutex_lock(&new_node->locked);
        //if the list is empty
        if (list->head == NULL)
        {
            /*pthread_mutex_lock((&list->head->locked)); (list->head is NULL)!! we get into this block only if  list->head == NULL so list->head->locked won't work */
            new_node->next = NULL;
            list->head = new_node;
            /*pthread_mutex_unlock((&list->head->locked));*/
            pthread_mutex_unlock((&new_node->locked));
        }

        //if the new node is the new head(minimal value)
        else if (value < list->head->value)
        {
            pthread_mutex_lock((&(list->head)->locked));
            new_node->next = list->head;
            list->head = new_node;
            pthread_mutex_unlock((&(list->head)->locked));
            pthread_mutex_unlock((&new_node->next->locked));/*was before correction : pthread_mutex_unlock((&new_node->locked)); you unlocked list->head twice  */
        }
        else
        {
            //find where to insert the new node.
            prev = list->head;
            curr = prev->next;
            pthread_mutex_lock(&(prev->locked));
            while (curr != NULL && curr->value < value)
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
            pthread_mutex_unlock(&(new_node->locked));
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
    if (list && list->head) {/*make sure they did not pass a NULL or empty list as a variable*/
        prev = list->head;
        curr = prev->next;
        pthread_mutex_lock(&(list->head)->locked);
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
    //if there is no list
    if (list == NULL)
    {
        printf("\n");
        return;
    }
    //if the list is empty
    if (list->head == NULL)
    {
        printf("\n");
        return;
    }
    // each time we lock one node print it then lock the next after that unlock the node
    //1)lock curr node.
    //2)print curr node.
    //3)lock node->next
    //4)unlock curr node
    node* curr = list->head;
    node* next;
    pthread_mutex_lock(&(curr->locked));
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
    if (list && list->head) {
        node* curr = list->head;
        node* next;
        pthread_mutex_lock(&(curr->locked));
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
            curr = next;// this line was missing in your code
        }
    }

    printf("%d items were counted\n", count); // DO NOT DELETE
}
