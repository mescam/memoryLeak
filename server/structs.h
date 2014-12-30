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
#ifndef STRUCTS_H
#define STRUCTS_H
#include <sys/types.h>
#include "stdint.h"
#define MAX_EVENTS  10
#define QUEUE_SIZE  5


struct _settings 
{
    char *interface; // address of interface
    ushort port; // listening port
    unsigned long long int memsize; // size of memory cache in bytes
    char *location; // where are our files
    uint workers_count; // how many workers should we spawn
};
#endif