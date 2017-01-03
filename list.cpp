//File: list.cpp
#include <list.h>

void list_init(list_ctl *head){
  head->prev = head;
  head->next = head;
}

void list_add_after_prev(list_ctl *new_el, list_ctl *prev, list_ctl *next){
  if_list_invalid(new_el, prev, next) {
    return;
  }
  prev->next = new_el;
  next->prev = new_el;
  new_el->next = next;
  new_el->prev = prev;
}

void list_add(list_ctl *head, list_ctl *new_el){
  list_add_after_prev(new_el, head, head->next);
}
void list_add_tail(list_ctl *head, list_ctl *new_el){
  list_add_after_prev(new_el, head->prev, head);
}

void list_del_helper(list_ctl *prev, list_ctl *next){
	next->prev = prev;
	prev->next = next;
}

void list_del_el(list_ctl *el){
  if(el == nullptr) return;
	list_del_helper(el->prev, el->next);
}

bool list_empty(list_ctl *head){
  list_ctl *next = head->next;
	return (next == head) && (next == head->prev);
}

bool list_is_last(list_ctl *head, list_ctl* last){
  return head->prev == last;
}

void list_replace(list_ctl *old, list_ctl *new_el){
	new_el->next = old->next;
	new_el->next->prev = new_el;
	new_el->prev = old->prev;
	new_el->prev->next = new_el;
}
