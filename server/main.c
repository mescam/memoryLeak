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
#include <pthread.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/epoll.h>
#include "structs.h"
#include "cache.h"

volatile int running = 1;

void 
signal_handler()
{
    printf("[!] Got interrupt/terminate signal!\n");
    running = 0;
}

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
                if (strlen(optarg) == 0) {
                    printf("[!] Interface not specified\n");
                    (*param_count) = -100;
                }
                settings->interface = malloc(strlen(optarg) * sizeof(char));
                strcpy(settings->interface, optarg);
                (*param_count)++;
                break;

            case 'P': // port
                if (strlen(optarg) == 0) {
                    printf("[!] Port not specified\n");
                    (*param_count) = -100;
                }
                settings->port = atoi(optarg);
                (*param_count)++;
                break;

            case 's': // size of memory cache
                if (strlen(optarg) == 0) {
                    printf("[!] Size not specified\n");
                    (*param_count) = -100;
                }
                settings->memsize = atoi(optarg);
                (*param_count)++;
                break;

            case 'w': // directory with files to serve
                if (strlen(optarg) == 0) {
                    printf("[!] Location not specified\n");
                    (*param_count) = -100;
                }
                settings->location = malloc(strlen(optarg) * sizeof(char));
                strcpy(settings->location, optarg);
                (*param_count)++;
                break;

            case 'c': // workers count
                if (strlen(optarg) == 0) {
                    printf("[!] Workers count not specified\n");
                    (*param_count) = -100;
                }
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
    fcntl(*socket_fd, F_SETFL, O_NONBLOCK);
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

void * 
worker(void *epollfd)
{
    int fds, i, n;
    struct epoll_event events[MAX_EVENTS];
    char buffer[1024];
    int fd = *((int*) epollfd);

    while (running) {
        // wait for event from my clients
        fds = epoll_wait(fd, events, MAX_EVENTS, 10);
        for (i = 0; i < fds; i++) {
            // the request is limited to max 1024 characters
            n = read(events[i].data.fd, buffer, sizeof(buffer));
            // let the magic begin!
            if (n == 0) {
                // close socket, it'll be automagically removed from epoll
                close(events[i].data.fd);
                printf("[-] Client disconnected\n");
            } else {
                cache_handle_request(events[i].data.fd, buffer);
            }
            
        }
    }

    return NULL;
}

void
run(struct _settings *settings)
{
    // array for epoll filedescriptors for every worker
    int *workers_epoll = malloc(sizeof(int) * settings->workers_count);
    // array for workers threads
    pthread_t *threads = malloc(sizeof(pthread_t) * settings->workers_count);

    int i;
    int rr_worker = 0; // round robin worker selector
    int socket_fd;
    int socket_epoll;

    socket_epoll = epoll_create(MAX_EVENTS);

    // create epoll for every worker
    for (i = 0; i < settings->workers_count; ++i) {
        workers_epoll[i] = epoll_create(MAX_EVENTS);
        pthread_create(threads + i, NULL, &worker,
                       (void *) (workers_epoll + i));
    }
    printf("[+] Spawned worker threads\n");

    if (!create_socket(settings, &socket_fd)) {
        // accept!
        int client_fd;
        struct sockaddr_in client_addr;
        socklen_t n_tmp;
        n_tmp = sizeof(struct sockaddr);
        struct epoll_event ev, events[MAX_EVENTS];
        int fds;

        // add listener to epoll
        ev.events = EPOLLIN;
        ev.data.fd = socket_fd;
        epoll_ctl(socket_epoll, EPOLL_CTL_ADD, socket_fd, &ev);
        
        // main loop
        while (running) { // the flag should be changed async
            // wait for connections on incoming socket
            fds = epoll_wait(socket_epoll, events, MAX_EVENTS, 10);
            // for every incoming connection
            for (i = 0; i < fds; ++i) {
                // accept it
                client_fd = accept(socket_fd, (struct sockaddr*) &client_addr,
                                   &n_tmp);
                fcntl(client_fd, F_SETFL, O_NONBLOCK); // set as nonblocking
                ev.events = EPOLLIN | EPOLLET;
                ev.data.fd = client_fd;
                // add to epoll of worker
                epoll_ctl(workers_epoll[rr_worker],
                          EPOLL_CTL_ADD, client_fd, &ev);
                printf("[+] Incoming connection for worker %d\n", rr_worker+1);
                // change worker
                rr_worker = (rr_worker + 1) % settings->workers_count;
            }
        } // end of while
    } else {
        printf("[!] Aborting\n");
        running = 0;
    }

    // close worker's epolls
    for (i = 0; i < settings->workers_count; ++i) {
        pthread_join(threads[i], NULL);
        close(workers_epoll[i]);
    }

    free(workers_epoll);
    close(socket_epoll);
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

    // handle signals
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    signal(SIGPIPE, SIG_IGN);

    // handle application parameters
    handle_arguments(argc, argv, &settings, &param_count);
    if (chdir(settings.location)) {
        printf("[!] Cannot change CWD to %s\n", settings.location);
        param_count = -100;
    }
    
    if (param_count == 4) { // all arguments specified, run application
        printf("[+] memoryLeak starting up!\n");
        printf("[+] Files directory: %s\n", settings.location);
        printf("[+] Configured to listen on %s:%d with memory size "
               "of %lld bytes and %d workers\n",
               settings.interface, settings.port, settings.memsize,
               settings.workers_count);
        set_cache_size(settings.memsize);
        run(&settings);
        cache_delete_all();
        printf("[-] Going down...\n");
    } else if (param_count > 0) { // show warning
        printf("[!] Missing arguments\n");
    }

    // free resources
    free(settings.interface);
    free(settings.location);

    return 0;
}