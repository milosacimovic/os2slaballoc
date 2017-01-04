//File: list.h
#ifndef _LIST_H_
#define _LIST_H_

#include <stddef.h>
#define if_list_invalid(ptr1, ptr2, ptr3) if(ptr1 == nullptr || ptr2 == nullptr || ptr3 == nullptr)
#define list_for_each(pos, head) \
	for (pos = (head)->next; pos != (head); pos = pos->next)
#define list_for_each_safe(pos, n, head) \
		for (pos = (head)->next, n = pos->next; pos != (head); \
		pos = n, n = pos->next)
#define container_of(ptr, type, member) ({                      \
        const decltype( ((type *)0)->member ) *__mptr = (ptr);    \
        (type *)( (char *)__mptr - offsetof(type,member) );})
/**
 * list_entry - get the struct for this entry
 * @ptr:	the &struct list_head pointer.
 * @type:	the type of the struct this is embedded in.
 * !IMPORTANT!@member:	the name of the list_head within the struct.
 */
#define list_entry(ptr, type, member) \
	container_of(ptr, type, member)

typedef struct list_el{
  struct list_el *prev;
  struct list_el *next;
} list_ctl_t;


void list_init(list_ctl_t* head);
/**
 * list_init - initialize the list
 * @head:	pointer to list head.
 */

void list_add_after_prev(list_ctl_t *new_el, list_ctl_t *prev, list_ctl_t *next);
/**
 * list_add_after_prev - adds element after previous
 * @new_el:	pointer to new element.
 * @prev:	pointer to previous element.
 * @next:	pointer to next element. 
 */

void list_add(list_ctl_t *head, list_ctl_t *new_el);
/**
 * list_add - adds element to beginning of the list.
 * @new_el:	pointer to new element.
 * @head:	pointer to head of the list.
 */

void list_add_tail(list_ctl_t *head, list_ctl_t *new_el);
/**
 * list_add_tail - adds element to end of the list.
 * @new_el:	pointer to new element.
 * @head:	pointer to head of the list.
 */

void list_del_helper(list_ctl_t *prev, list_ctl_t *next);
/**
 * list_add_helper - adds element to beginning of the list.
 * @new_el:	pointer to new element.
 * @head:	pointer to head of the list.
 */

void list_del_el(list_ctl_t *el);
/**
 * list_del_el - unlinks el element.
 * @el:	pointer to element to be deleted.
 */
bool list_empty(list_ctl_t *head);
/**
 * list_empty - returns true if list is empty, otherwise false.
 * @head:	pointer to head of the list.
 */
bool list_is_last(list_ctl_t *head, list_ctl_t* last);
/**
 * list_is_last - adds element to beginning of the list.
 * @head:	pointer to head of the list.
 * @last:	pointer to element to be tested as last.
 */
void list_replace(list_ctl_t *old, list_ctl_t *new_el);
/**
 * list_replace - replaces old with the new.
 * @old:	pointer to old element.
 * @new:	pointer to new element.
 */
#endif
