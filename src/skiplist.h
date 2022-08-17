/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright(c) 2022 Sanpe <sanpeqf@gmail.com>
 */

#ifndef _SKIPLIST_H_
#define _SKIPLIST_H_

#include "list.h"
#include <errno.h>

typedef long (*skiplist_find_t)(const void *data, const void *key);
typedef long (*skiplist_cmp_t)(const void *dataa, const void *datab);

struct skip_node {
    void *pdata;
    struct list_head list[0];
};

struct skip_head {
    unsigned int count;
    unsigned int curr;
    unsigned int levels;
    struct list_head nodes[0];
};

extern int skiplist_insert(struct skip_head *head, void *data, skiplist_cmp_t cmp);
extern void skiplist_delete(struct skip_head *head, void *key, skiplist_find_t find);
extern void *skiplist_find(struct skip_head *head, void *key, skiplist_find_t find);

extern void skiplist_reset(struct skip_head *head);
extern struct skip_head *skiplist_create(unsigned int levels);
extern void skiplist_destroy(struct skip_head *head);

#endif  /* _SKIPLIST_H_ */
