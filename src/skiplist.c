/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright(c) 2022 Sanpe <sanpeqf@gmail.com>
 */

#include "skiplist.h"
#include <stdlib.h>

static unsigned int random_level(struct skip_head *head)
{
    unsigned int level = 1;

    do {
        if (rand() < RAND_MAX >> 2)
            break;
        level++;
    } while (level < head->levels);

    return level;
}

static struct skip_node *
skipnode_find(struct skip_head *head, const void *key,
              skiplist_find_t find, unsigned int *plev)
{
    unsigned int level = head->curr;
    struct skip_node *walk, *match;
    struct list_head *list;
    long retval;

    if (unlikely(!level))
        return NULL;

    list = &head->nodes[level - 1];
    match = list_first_entry(list, struct skip_node, list[level - 1]);
    retval = find(match->pdata, key);

    for (; level--; --list) {
        if (retval < 0) {
            walk = match;
            list_for_each_entry_continue(walk, list, list[level]) {
                retval = find(walk->pdata, key);
                if (retval >= 0) {
                    match = walk;
                    break;
                }
            }
        } else if (retval > 0) {
            walk = match;
            list_for_each_entry_reverse_continue(walk, list, list[level]) {
                retval = find(walk->pdata, key);
                if (retval <= 0) {
                    match = walk;
                    break;
                }
            }
        }

        if (retval == 0) {
            if (plev)
                *plev = level + 1;
            return match;
        }
    }

    return NULL;
}

int skiplist_insert(struct skip_head *head, void *data, skiplist_cmp_t cmp)
{
    struct skip_node *walk, *match, *node;
    struct list_head *list;
    unsigned int level;
    long tmp, retval;

    level = random_level(head);
    head->curr = head->curr > level ? head->curr : level;

    node = malloc(sizeof(*node) + sizeof(*node->list) * level);
    if (unlikely(!node))
        return -ENOMEM;

    node->pdata = data;
    list = &head->nodes[level - 1];

    for (; unlikely(list_check_empty(list)); --list) {
        list_add(list, &node->list[--level]);
        if (!level)
            goto finish;
    }

    match = list_first_entry(list, struct skip_node, list[level - 1]);
    retval = cmp(data, match->pdata);

    for (; level--; --list) {
        if (retval > 0) {
            walk = match;
            list_for_each_entry_continue(walk, list, list[level]) {
                tmp = cmp(data, walk->pdata);
                if (tmp < 0)
                    break;
                match = walk;
                retval = tmp;
            }
        } else if (retval < 0) {
            walk = match;
            list_for_each_entry_reverse_continue(walk, list, list[level]) {
                tmp = cmp(data, walk->pdata);
                if (tmp > 0)
                    break;
                match = walk;
                retval = tmp;
            }
        }

        if (retval >= 0)
            list_add(&match->list[level], &node->list[level]);
        else
            list_add_prev(&match->list[level], &node->list[level]);
    }

finish:
    head->count++;
    return 0;
}

static struct skip_node *
skipnode_even_find(struct skip_head *head, const void *key,
                   skiplist_even_find_t find, unsigned int *plev)
{
    unsigned int level = head->curr;
    struct skip_node *walk, *match;
    struct list_head *list;
    unsigned long disparity, compare;
    bool signplus;

    if (unlikely(!level))
        return NULL;

    list = &head->nodes[level - 1];
    match = list_first_entry(list, struct skip_node, list[level - 1]);
    disparity = find(match->pdata, key, &signplus);

    for (; level--; --list) {
        if (disparity) {
            if ((walk = list_next_entry_or_null(match, list, list[level]))) {
                list_for_each_entry_from(walk, list, list[level]) {
                    compare = find(walk->pdata, key, &signplus);
                    if (disparity <= compare &&
                        (disparity == compare && signplus != false))
                        break;
                    disparity = compare;
                    match = walk;
                    if (!disparity)
                        goto finish;
                }
            }

            if ((walk = list_prev_entry_or_null(match, list, list[level]))) {
                list_for_each_entry_reverse_from(walk, list, list[level]) {
                    compare = find(walk->pdata, key, &signplus);
                    if (disparity >= compare &&
                        (disparity == compare && signplus != true))
                        break;
                    disparity = compare;
                    match = walk;
                    if (!disparity)
                        goto finish;
                }
            }
        }

finish:
        if (disparity == 0) {
            if (plev)
                *plev = level + 1;
            return match;
        }
    }

    return NULL;
}

int skiplist_even_insert(struct skip_head *head, void *data, skiplist_even_cmp_t cmp)
{
    struct skip_node *walk, *match, *node;
    struct list_head *list;
    unsigned int level;
    unsigned long disparity, compare;
    bool signplus;

    level = random_level(head);
    head->curr = head->curr > level ? head->curr : level;

    node = malloc(sizeof(*node) + sizeof(*node->list) * level);
    if (unlikely(!node))
        return -ENOMEM;

    node->pdata = data;
    list = &head->nodes[level - 1];

    for (; unlikely(list_check_empty(list)); --list) {
        list_add(list, &node->list[--level]);
        if (!level)
            goto finish;
    }

    match = list_first_entry(list, struct skip_node, list[level - 1]);
    disparity = cmp(data, match->pdata, &signplus);

    for (; level--; --list) {
        if (disparity) {
            if ((walk = list_next_entry_or_null(match, list, list[level]))) {
                list_for_each_entry_from(walk, list, list[level]) {
                    compare = cmp(node->pdata, walk->pdata, &signplus);
                    if (disparity <= compare &&
                        (disparity == compare && signplus != true))
                        break;
                    disparity = compare;
                    match = walk;
                }
            }

            if ((walk = list_prev_entry_or_null(match, list, list[level]))) {
                list_for_each_entry_reverse_from(walk, list, list[level]) {
                    compare = cmp(node->pdata, walk->pdata, &signplus);
                    if (disparity <= compare &&
                        (disparity == compare && signplus != false))
                        break;
                    disparity = compare;
                    match = walk;
                }
            }
        }

        if (signplus)
            list_add(&match->list[level], &node->list[level]);
        else
            list_add_prev(&match->list[level], &node->list[level]);
    }

finish:
    head->count++;
    return 0;
}

void skiplist_delete(struct skip_head *head, void *key, skiplist_find_t find)
{
    struct skip_node *node;
    unsigned int level;

    node = skipnode_find(head, key, find, &level);
    if (unlikely(!node))
        return;

    while (level--) {
        list_del(&node->list[level]);
        if (list_check_empty(&head->nodes[level]))
            head->curr = level;
    }

    head->count--;
    free(node);
}

void *skiplist_find(struct skip_head *head, void *key, skiplist_find_t find)
{
    struct skip_node *node;
    node = skipnode_find(head, key, find, NULL);
    return node ? node->pdata : NULL;
}

void skiplist_even_delete(struct skip_head *head, void *key, skiplist_even_find_t find)
{
    struct skip_node *node;
    unsigned int level;

    node = skipnode_even_find(head, key, find, &level);
    if (unlikely(!node))
        return;

    while (level--) {
        list_del(&node->list[level]);
        if (list_check_empty(&head->nodes[level]))
            head->curr = level;
    }

    head->count--;
    free(node);
}

void *skiplist_even_find(struct skip_head *head, void *key, skiplist_even_find_t find)
{
    struct skip_node *node;
    node = skipnode_even_find(head, key, find, NULL);
    return node ? node->pdata : NULL;
}

static void skiplist_release(struct skip_head *head)
{
    struct skip_node *node, *tmp;
    list_for_each_entry_safe(node, tmp, head->nodes, list[0])
        free(node);
}

void skiplist_reset(struct skip_head *head)
{
    unsigned int count;

    skiplist_release(head);
    for (count = 0; count < head->levels; ++count)
        list_head_init(&head->nodes[count]);

    head->count = 0;
    head->curr = 1;
}

struct skip_head *skiplist_create(unsigned int levels)
{
    struct skip_head *head;
    unsigned int count;

    head = malloc(sizeof(*head) + sizeof(*head->nodes) * levels);
    if (unlikely(!head))
        return NULL;

    for (count = 0; count < levels; ++count)
        list_head_init(&head->nodes[count]);

    head->levels = levels;
    return head;
}

void skiplist_destroy(struct skip_head *head)
{
    skiplist_release(head);
    free(head);
}
