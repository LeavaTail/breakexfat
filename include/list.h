// SPDX-License-Identifier: GPL-2.0
/*
 *  Copyright (C) 2022 LeavaTail
 */
#ifndef _LIST_H
#define _LIST_H

#include <stdlib.h>

/**
 * singly linked list
 */
struct list_head {
	void *data;
	struct list_head *next;
};

/**
 * @brief Allocate new object in list_head
 *
 * @return new object in list_head
 */
static inline struct list_head *init_list(void)
{
	struct list_head *new_node;

	new_node = (struct list_head *)malloc(sizeof(struct list_head));
	new_node->next = NULL;

	return new_node;
}

/**
 * @brief Allocate new object w/ data in list_head
 * @param [in] data any pointer
 *
 * @return new object in list_head
 */
static inline struct list_head *init_list_head(void *data)
{
	struct list_head *new_node = init_list();

	new_node->data = data;

	return new_node;
}

/**
 * @brief return last node in list_head
 * @param [in] head list_head
 *
 * @return the last of list_head
 */
static inline struct list_head* list_last(struct list_head *head)
{
	while (head->next != NULL)
		head = head->next;
	return head;
}

/**
 * @brief add node into list_head
 * @param [in] head list_head
 * @param [in] data any pointer for new node
 */
static inline void list_add(struct list_head *head, void *data)
{
	struct list_head *node;

	node = (struct list_head *)malloc(sizeof(struct list_head));
	node->data = data;
	node->next = head->next;
	head->next = node;
}

/**
 * @brief add node into the last of list_head
 * @param [in] head list_head
 * @param [in] data any pointer for new node
 */
static inline void list_add_tail(struct list_head *head, void *data)
{
	list_add(list_last(head), data);
}

/**
 * @brief remove all node in list_head
 * @param [in] head list_head
 */
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

/**
 * @brief search data from list_head
 * @param [in] node list_head
 * @param [in] data target data
 *
 * @return searched node or NULL
 */
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
