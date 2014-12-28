/*
    This file is part of memoryLeak.

    memoryLeak is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    memoryLeak is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with memoryLeak; if not, write to the Free Software
    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>
#include <string.h>

struct _settings 
{
    char *interface; // address of interface
    uint port; // listening port
    uint memsize; // size of memory cache in bytes
    char *location; // where are our files
};

int
main(int argc, char **argv)
{
    struct _settings settings;
    char c;
    settings.port = settings.memsize = 0;
    settings.interface = settings.location = (char*) 0;

    int param_count = 0;
    
    // handle application parameters
    while ((c = getopt(argc, argv, "i:p:s:w:hv")) != -1)
    {
        switch (c)
        {
            case 'i': // interface
                settings.interface = malloc(strlen(optarg) * sizeof(char));
                strcpy(settings.interface, optarg);
                param_count++;
                break;

            case 'p': // port
                settings.port = atoi(optarg);
                param_count++;
                break;

            case 's': // size of memory cache
                settings.memsize = atoi(optarg);
                param_count++;
                break;

            case 'w': // directory with files to serve
                settings.location = malloc(strlen(optarg) * sizeof(char));
                strcpy(settings.location, optarg);
                param_count++;
                break;

            case 'v': // version
                // TODO: should be somewhere else? :X 
                printf("memoryLeak, version 1.0\n"
                "Simple memcache server for files\n\n"
                "Copyright (C) 2014\t"
                "Jakub Woźniak <jakub at wozniak dot im>\n"
                "Project for Computer Networks course\n"
                "Faculty of Computing, Poznan University of Technology\n\n"
                "License GPLv3+: GNU GPL version 3 or later "
                "<http://gnu.org/licenses/gpl.html>\nThis is free software;"
                " you are free to change and redistribute it.\nThere is "
                "NO WARRANTY, to the extent permitted by law.\n");
                param_count = -100;
                break;

            default:
                printf("[!] Unrecognized argument %c!\n", c);
        }
    }

    if (param_count == 4) // all arguments specified, run application
    {
        printf("[+] memoryLeak starting up!\n");
        printf("[+] Files directory: %s\n", settings.location);
        printf("[ ] Configured to listen on %s:%d with memory size "
               "of %d bytes\n",
               settings.interface, settings.port, settings.memsize);
    } 
    else if (param_count > 0) // show warning
    {
        printf("[!] Missing arguments\n");
    }

    // free resources
    free(settings.interface);
    free(settings.location);

    return 0;
}