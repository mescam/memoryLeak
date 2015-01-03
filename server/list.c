/*
    This file is part of memoryLeak.

    memoryLeak is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 3 of the License, or
    (at your option) any later version.

    memoryLeak is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with memoryLeak; if not, write to the Free Software
    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/
#include <stdlib.h>
#include <stdio.h>
#include "list.h"

void
list_insert(struct _list_el **list, void *el)
{
    struct _list_el *e = malloc(sizeof(struct _list_el));
    e->el = el;
    if (*list == NULL) {
        e->prev = e;
        e->next = e;
        *list = e;
    } else {
        e->prev = *list;
        e->next = (*list)->next;
        (*list)->next->prev = e;
        (*list)->next = e;
    }
}

void
list_delete(struct _list_el **list)
{
    if (*list == NULL) {
        return;
    } else if ((*list)->next == *list) {
        free(*list);
        *list = NULL;
    } else {
        struct _list_el *tmp = *list;
        tmp->prev->next = tmp->next;
        tmp->next->prev = tmp->prev;
        (*list) = tmp->next;
        free(tmp);
    }
}