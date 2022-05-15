// SPDX-License-Identifier: GPL-2.0
/*
 *  Copyright (C) 2022 LeavaTail
 */
#ifndef _LIST_H
#define _LIST_H

#include <stdlib.h>

struct list_head {
	void *data;
	struct list_head *next;
};

static inline struct list_head *init_list(void)
{
	struct list_head *new_node;

	new_node = (struct list_head *)malloc(sizeof(struct list_head));
	new_node->next = NULL;

	return new_node;
}

static inline struct list_head *init_list_head(void *data)
{
	struct list_head *new_node = init_list();

	new_node->data = data;

	return new_node;
}

static inline struct list_head* list_last(struct list_head *head)
{
	while (head->next != NULL)
		head = head->next;
	return head;
}

static inline void list_add(struct list_head *head, void *data)
{
	struct list_head *node;

	node = (struct list_head *)malloc(sizeof(struct list_head));
	node->data = data;
	node->next = head->next;
	head->next = node;
}

static inline void list_add_tail(struct list_head *head, void *data)
{
	list_add(list_last(head), data);
}

static inline void list_del(struct list_head *head)
{
	struct list_head *tmp;

	if ((tmp = head->next) != NULL) {
		head->next = tmp->next;
		free(tmp);
	} else {
		free(head);
	}
}

static inline struct list_head *list_entry(struct list_head *node, void *data)
{
	while (node->next != NULL) {
		node = node->next;
		if(data == node->data)
			return node;
	}
	return NULL;
}

#endif /*_LIST_H */
