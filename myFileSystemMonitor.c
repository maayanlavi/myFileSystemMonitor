//submitters:
// maayan lavi : 313374621
// guy sharir: 310010244
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/inotify.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <poll.h>
#include <ctype.h>
#include <time.h>
#include <getopt.h>
#include <errno.h>
#include <execinfo.h>
#include <pthread.h>
#include <libcli.h>



void inotify(int argc, char **argv, char *address)
{
    char buf;
    int fd, i;
    int *wd;

    printf("Press ENTER key to terminate.\n");

    fd = inotify_init1(IN_NONBLOCK);
    if (fd == -1)
    {
        perror("inotify_init1");
        exit(EXIT_FAILURE);
    }

    wd = (int *)calloc(argc + 1, sizeof(int));
    if (wd == NULL)
    {
        perror("calloc");
        exit(EXIT_FAILURE);
    }

    for (i = 0; i < argc; i++)
    {
        wd[i] = inotify_add_watch(fd, argv[i], IN_OPEN | IN_CLOSE);
        if (wd[i] == -1)
        {
            fprintf(stderr, "Cannot watch '%s'\n", argv[i]);
            perror("inotify_add_watch");
            exit(EXIT_FAILURE);
        }
    }

}

char **split_str(char *str, size_t *size)
{
    char **array = NULL;
    char *p;
    size_t items = 0, q;
    const char *sepa = " ";

    p = str;
    for (;;)
    {
        p += strspn(p, sepa);
        if (!(q = strcspn(p, sepa)))
            break;
        if (q)
        {
            array = realloc(array, (items + 1) * sizeof(char *));
            if (array == NULL)
                exit(EXIT_FAILURE);

            array[items] = malloc(q + 1);
            if (array[items] == NULL)
                exit(EXIT_FAILURE);

            strncpy(array[items], p, q);
            array[items][q] = 0;
            items++;
            p += q;
        }
    }
    *size = items;
    return array;
}



int main(int argc, char **argv)
{
    if (argc == 5)
    {
        int option_index = 0;
        char *directory = NULL;
        char *address = NULL;
        while ((option_index = getopt(argc, argv, "d:i:")) != -1)
        {
            switch (option_index)
            {
            case 'd':
                directory = optarg;
                break;
            case 'i':
                address = optarg;
                break;
            }
        }

        if (directory != NULL && address != NULL)
        {
            size_t cnt = 0;
            char **directories = split_str(directory, &cnt);
            inotify((int)cnt, directories, address);
            exit(EXIT_SUCCESS);
        }
    }
    else
    {
        exit(EXIT_FAILURE);
    }

}