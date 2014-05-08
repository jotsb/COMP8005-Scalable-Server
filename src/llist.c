
#include "common.h"

/**
 * client_compare
 * 
 * compares two client sockets.
 * 
 * @param _a
 * @param _b
 * @return 
 */
bool client_compare(const void * _a, const void * _b) {
    const client *a = (const client *) _a;
    const client *b = (const client *) _b;

    if (a->fd == b->fd)
        return true;
    else
        return false;
}

/* Linked list */

llist *llist_new(void) {
    llist *l = malloc(sizeof (llist));
    if (l == NULL)
        fprintf(stderr, "llist_new: malloc() failed\n");
    l->link = NULL;
    return l;
}

void llist_free(llist *l, void (*free_func)(void *)) {
    node *n = l->link;
    while (n) {
        if (n->data != NULL)
            free_func(n->data);
        l->link = l->link->next;
        free(n);
    }
    free(l);
}

llist *llist_append(llist *l, void *data) {
    node *a = NULL;
    node *b = NULL;

    a = l->link;

    if (l == NULL)
        return l;

    if (a == NULL) {
        a = malloc(sizeof (node));
        a->data = data;
        a->next = NULL;
        l->link = a;
    } else {
        while (a->next != NULL)
            a = a->next;

        b = malloc(sizeof (node));
        b->data = data;
        b->next = NULL;
        a->next = b;
    }

    return l;
}

llist *llist_remove(llist *l, void *data,
        bool(*compare_func)(const void*, const void*)) {
    node *old = NULL;
    node *temp = NULL;

    if (l == NULL)
        fprintf(stderr, "Empty list, can't delete data\n");
    else {
        temp = l->link;
        while (temp != NULL) {
            if (compare_func(temp->data, data)) {
                if (temp == l->link) /* first Node case */
                    l->link = temp->next; /* shift the header node */
                else
                    old->next = temp->next;
                free(temp);
                return l;
            } else {
                old = temp;
                temp = temp->next;
            }
        }
    }

    return l;
}

int llist_length(llist *l) {
    int n;
    node *p = l->link;
    n = 0;
    while (p != NULL) {
        n++;
        p = p->next;
    }
    return n;
}

bool llist_is_empty(llist *l) {
    if (l == NULL || l->link == NULL)
        return true;
    return false;
}