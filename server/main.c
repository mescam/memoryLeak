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
};

int
main(int argc, char **argv)
{
    struct _settings settings;
    char c;
    settings.port = settings.memsize = 0;
    settings.interface = (char*) 0;

    printf("[+] memoryLeak starting up!\n");
    
    // handle application parameters
    while ((c = getopt(argc, argv, "i:p:s:")) != -1)
    {
        switch (c)
        {
            case 'i':
                settings.interface = malloc(strlen(optarg) * sizeof(char));
                strcpy(settings.interface, optarg);
                break;
            case 'p':
                settings.port = atoi(optarg);
                break;
            case 's':
                settings.memsize = atoi(optarg);
                break;
            default:
                printf("[!] Unrecognized parameter %c!\n", c);
        }
    }

    printf("[ ] Configured to listen on %s:%d with memory size of %d bytes\n",
            settings.interface, settings.port, settings.memsize);

    return 0;
}