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

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>


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

int running = 1;

void
print_version()
{
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
}

void
print_help()
{
    printf("./memoryLeak -I interface -P port -s size -w directory [-c num]\n"
           "\t-I - ip address of network interface, 0.0.0.0 for all\n"
           "\t-P - port of service\n"
           "\t-s - size in bytes of cache\n"
           "\t-w - directory with files to serve\n"
           "\t-c - number of worker threads, defaults to number of cores\n"
        );
}

void
handle_arguments(int argc, char **argv, struct _settings *settings,
                 int *param_count)
{
    char c;
    while ((c = getopt(argc, argv, "I:P:s:w:c:hv")) != -1) {
        switch (c) {
            case 'I': // interface
                settings->interface = malloc(strlen(optarg) * sizeof(char));
                strcpy(settings->interface, optarg);
                (*param_count)++;
                break;

            case 'P': // port
                settings->port = atoi(optarg);
                (*param_count)++;
                break;

            case 's': // size of memory cache
                settings->memsize = atoi(optarg);
                (*param_count)++;
                break;

            case 'w': // directory with files to serve
                settings->location = malloc(strlen(optarg) * sizeof(char));
                strcpy(settings->location, optarg);
                (*param_count)++;
                break;

            case 'c': // workers count
                settings->workers_count = atoi(optarg);
                break;

            case 'v': // version
                print_version();
                (*param_count) = -100;
                break;

            case 'h': // help
                print_help();
                (*param_count) = -100;
                break;

            default:
                printf("[!] Unrecognized argument %c!\n", c);
        }
    }
}

int
create_socket(struct _settings *settings, int *socket_fd)
{
    struct sockaddr_in st_addr;
    int n_opt = 1;

    // fill sockaddr struct with data from settings
    memset(&st_addr, 0, sizeof(struct sockaddr));
    st_addr.sin_family = AF_INET;
    st_addr.sin_addr.s_addr = inet_addr(settings->interface);
    st_addr.sin_port = htons(settings->port);

    // create socket!
    *socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (*socket_fd < 0) {
        perror("[!] Error during creating socket");
        return 1;
    }
    setsockopt(*socket_fd, SOL_SOCKET, SO_REUSEADDR,
              (char*)&n_opt, sizeof(n_opt));

    if(bind(*socket_fd, (struct sockaddr*)&st_addr, sizeof(struct sockaddr))) {
        perror("[!] Binding error");
        return 1;
    }

    if(listen(*socket_fd, QUEUE_SIZE)) {
        perror("[!] listen() error");
        return 1;
    }

    return 0;
}

void
run(struct _settings *settings)
{
    // array for epoll filedescriptors for every worker
    int *workers_epoll = malloc(sizeof(int) * settings->workers_count);
    int i;
    int rr_worker = 0; // round robin worker selector
    int socket_fd;

    // create epoll for every worker
    for(i = 0; i < settings->workers_count; ++i) {
        workers_epoll[i] = epoll_create(MAX_EVENTS);
    }
    printf("[+] Spawned worker threads\n");

    if(!create_socket(settings, &socket_fd)) {
        // accept!
        int client_fd;
        struct sockaddr_in client_addr;
        socklen_t n_tmp;
        n_tmp = sizeof(struct sockaddr);

        while (running) {
            client_fd = accept(socket_fd, (struct sockaddr*) &client_addr,
                               &n_tmp);
            if (client_fd < 0) {
                printf("[!] Error when accepting incoming connection\n");
            }
            printf("[+] Incoming connection for worker %d\n", rr_worker+1);

            rr_worker = (rr_worker + 1) % settings->workers_count;
        }

    } else {
        printf("[!] Aborting\n");
    }

    // close worker's epolls
    for(i = 0; i <= settings->workers_count; ++i)
        close(workers_epoll[i]);

    free(workers_epoll);
    if (socket_fd > 0)
        close(socket_fd);
}

int
main(int argc, char **argv)
{
    struct _settings settings;
    settings.port = settings.memsize = 0;
    settings.interface = settings.location = (char*) 0;
    // get number of cores
    settings.workers_count = sysconf(_SC_NPROCESSORS_ONLN); 
    int param_count = 0;
    
    // handle application parameters
    handle_arguments(argc, argv, &settings, &param_count);
    
    if (param_count == 4) { // all arguments specified, run application
        printf("[+] memoryLeak starting up!\n");
        printf("[+] Files directory: %s\n", settings.location);
        printf("[+] Configured to listen on %s:%d with memory size "
               "of %lld bytes and %d workers\n",
               settings.interface, settings.port, settings.memsize,
               settings.workers_count);
        run(&settings);
    } else if (param_count > 0) { // show warning
        printf("[!] Missing arguments\n");
    }

    // free resources
    free(settings.interface);
    free(settings.location);

    return 0;
}