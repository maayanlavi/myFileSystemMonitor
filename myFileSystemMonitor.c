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

#define PORT 8000
#define HTML_DATA_LIMIT 1000

int flag = 0;
struct cli_def *cli;

int cmd_backtrace(struct cli_def *cli, const char *command, char *argv[], int argc)
{
    cli_print(cli, "called %s with %s\r\n", __FUNCTION__, command);
    flag = 1;
    return CLI_OK;
}

void *my_backtrace(void *arg)
{
    struct cli_def *cli = (struct cli_def *)arg;

    int j, nptrs;
    void *buffer[100];
    char **strings;

    nptrs = backtrace(buffer, 100);
    cli_print(cli, "backtrace() returned %d addresses\n", nptrs);

    strings = backtrace_symbols(buffer, nptrs);
    if (strings == NULL)
    {
        perror("backtrace_symbols");
        exit(EXIT_FAILURE);
    }

    for (j = 0; j < nptrs; j++)
        cli_print(cli, "%s\n", strings[j]);

    free(strings);
    return NULL;
}

char **str_splitter(char *str, size_t *size)
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

void str_concat(char **s1, const char *s2)
{
    int s1_len = strlen(*s1);
    int s2_len = strlen(s2);
    int len = s1_len + s2_len + 1;
    char *new_str = (char *)realloc(*s1, len); 
    if (new_str != NULL)
    {
        for (int i = 0; i < s2_len; i++)
        {
            new_str[s1_len++] = s2[i];
        }
        new_str[s1_len] = '\0';
        *s1 = new_str;
    }
    else
        exit(EXIT_FAILURE);
}

char *createCommand(char *html_data)
{
    char start[] = "<!DOCTYPE html><html lang='en'><head><meta charset='UTF-8' /><meta name='viewport' content='width=device-width, initial-scale=1.0' /><title>FileSystem Monitor</title></head><body><div id='wrapper'><h1>File System Monitor report:</h1><div id='log'>";
    char end[] =  "</div></div></body></html>";

    int data_len = strlen(html_data);
    int fsize = data_len + strlen(start) + strlen(end) + 35;

    char *copy = malloc(fsize);
    if (copy != NULL)
    {
        size_t i = 0;
        char *ptr = NULL;

        copy[i++] = 'e';
        copy[i++] = 'c';
        copy[i++] = 'h';
        copy[i++] = 'o';
        copy[i++] = ' ';
        copy[i++] = '"';

        ptr = start;
        while (*ptr)
        {
            copy[i++] = *ptr;
            ptr++;
        }

        ptr = html_data;
        while (*ptr)
        {
            copy[i++] = *ptr;
            ptr++;
        }

        ptr = end;
        while (*ptr)
        {
            copy[i++] = *ptr;
            ptr++;
        }
        copy[i++] = '"';
        copy[i++] = ' ';
        copy[i++] = '>';
        copy[i++] = ' ';
        copy[i++] = '/';
        copy[i++] = 'v';
        copy[i++] = 'a';
        copy[i++] = 'r';
        copy[i++] = '/';
        copy[i++] = 'w';
        copy[i++] = 'w';
        copy[i++] = 'w';
        copy[i++] = '/';
        copy[i++] = 'h';
        copy[i++] = 't';
        copy[i++] = 'm';
        copy[i++] = 'l';
        copy[i++] = '/';
        copy[i++] = 'i';
        copy[i++] = 'n';
        copy[i++] = 'd';
        copy[i++] = 'e';
        copy[i++] = 'x';
        copy[i++] = '.';
        copy[i++] = 'h';
        copy[i++] = 't';
        copy[i++] = 'm';
        copy[i++] = 'l';
        copy[i] = '\0';
        return copy;
    }
    else
        exit(EXIT_FAILURE);
}

void __attribute__((no_instrument_function)) __cyg_profile_func_enter(void *this_fn, void *call_site)
{ 
}

void __attribute__((no_instrument_function)) __cyg_profile_func_exit(void *this_fn, void *call_site)
{
    if (flag)
    {
        flag = 0;
        my_backtrace(cli);
    }
}

void *my_libcli(void *arg)
{
    char *address = (char *)arg;
    struct sockaddr_in servaddr;
    int on = 1, x, s;

    cli = cli_init();

    cli_set_hostname(cli, "FileSystemMonitor");

    cli_register_command(cli, NULL, "backtrace", cmd_backtrace, PRIVILEGE_UNPRIVILEGED, MODE_EXEC, NULL);

    s = socket(AF_INET, SOCK_STREAM, 0);
    setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));

    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(PORT);
    servaddr.sin_addr.s_addr = inet_addr(address);

    bind(s, (struct sockaddr *)&servaddr, sizeof(servaddr));

    listen(s, 50);

    while ((x = accept(s, NULL, 0)))
    {
        cli_loop(cli, x);
        close(x);
    }

    return NULL;
}

static void event_handler( char **html, int *html_data_count,int fd, int *wd, int argc, char *argv[], int *clientSocket, struct sockaddr_in *serverAddr)
{
    char date_time[26];

    time_t t = time(NULL);

    struct tm *tm = localtime(&t);

    char buf[4096] __attribute__((aligned(__alignof__(struct inotify_event))));
    const struct inotify_event *event;
    int i;
    ssize_t length;
    char *ptr;

    for (;;)
    {
        length = read(fd, buf, sizeof buf);
        if (length == -1 && errno != EAGAIN)
        {
            perror("error reading");
            exit(EXIT_FAILURE);
        }

        if (length <= 0)
            break;

        for (ptr = buf; ptr < buf + length; ptr += sizeof(struct inotify_event) + event->len)
        {
            event = (const struct inotify_event *)ptr;

            if (event->mask & IN_OPEN)
                continue;

            if (event->mask & IN_ISDIR)
                continue;  

            if (*html_data_count == HTML_DATA_LIMIT)
            {
                *html_data_count = 0;
                free(*html);
                *html = malloc((strlen(" ") + 1) * sizeof(char));
                if (*html == NULL)
                    exit(EXIT_FAILURE);
                strcpy(*html, " ");
            }

            char *udp_data = malloc((strlen("FILE ACCESSED: ") + 1) * sizeof(char));
            if (udp_data == NULL)
                exit(EXIT_FAILURE);

            strcpy(udp_data, "FILE ACCESSED: ");

            str_concat(&*html, "<ul> <li>FILE ACCESSED: ");

            for (i = 0; i < argc; ++i)
            {
                if (wd[i] == event->wd)
                {
                    str_concat(&udp_data, argv[i]);
                    str_concat(&udp_data, "/");

                    str_concat(&*html, argv[i]);
                    str_concat(&*html, "/");

                    break;
                }
            }

            if (event->len)
            {
                str_concat(&udp_data, event->name);
                str_concat(&udp_data, "\n");

                str_concat(&*html, event->name);
                str_concat(&*html, "</li>");
            }

            str_concat(&udp_data, "ACCESS: ");

            str_concat(&*html, "<li>ACCESS: ");

            if (event->mask & IN_CLOSE_NOWRITE)
            {
                str_concat(&udp_data, "NO_WRITE\n");
                str_concat(&*html, "NO_WRITE</li>");
            }
            else if (event->mask & IN_CLOSE_WRITE)
            {
                str_concat(&udp_data, "WRITE\n");
                str_concat(&*html, "WRITE</li>");
            }

            strftime(date_time, 26, "%d-%m-%Y %H:%M:%S\n\n", tm);

            str_concat(&udp_data, "TIME OF ACCESS: ");
            str_concat(&udp_data, date_time);

            sendto(*clientSocket, udp_data, strlen(udp_data),
                   MSG_CONFIRM, (const struct sockaddr *)&*serverAddr,
                   sizeof(*serverAddr));

            free(udp_data);

            str_concat(&*html, "<li>TIME OF ACCESS: ");
            str_concat(&*html, date_time);
            str_concat(&*html, "</li> </ul>");

            ++*html_data_count;
        }
        char *command = createCommand(*html);
        system(command);
        free(command);
    }
}

void inotify(int argc, char **argv, char *address)
{
    char buf;
    int fd, i, poll_num;
    int *wd;
    nfds_t nfds;
    struct pollfd fds[2];

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

    nfds = 2;

    fds[0].fd = STDIN_FILENO;
    fds[0].events = POLLIN;

    fds[1].fd = fd;
    fds[1].events = POLLIN;

    pthread_t thread_id;
    pthread_create(&thread_id, NULL, my_libcli, address);

    int clientSocket = socket(AF_INET, SOCK_DGRAM, 0); 
    if (clientSocket < 0)
    {
        perror("connection error\n");
        exit(EXIT_FAILURE);
    }


    struct sockaddr_in serverAddr = {'\0'};

    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(PORT);
    serverAddr.sin_addr.s_addr = inet_addr(address);

    if (connect(clientSocket, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) < 0)
    {
        perror("connection error\n\n");
        exit(EXIT_FAILURE);
    }


    printf("Waiting for events\n");

    char *html = malloc((strlen(" ") + 1) * sizeof(char));

    
    if (html == NULL)
        exit(EXIT_FAILURE);


    strcpy(html, " ");
    int html_data_count = 0;

    for(;;)
    {
        poll_num = poll(fds, nfds, -1);
        if (poll_num == -1)
        {
            if (errno == EINTR)
                continue;

            perror("poll error");
            exit(EXIT_FAILURE);
        }

        if (poll_num > 0)
        {
            if (fds[0].revents & POLLIN)
            {
                while (read(STDIN_FILENO, &buf, 1) > 0 && buf != '\n')
                    continue;

                break;
            }

            if (fds[1].revents & POLLIN)
                event_handler( &html, &html_data_count,fd, wd, argc, argv, &clientSocket, &serverAddr);
        }
    }

    printf("Stopped waiting for events .\n");

    for (int i = 0; i < argc; i++)
        free(argv[i]);

    free(argv);

    close(fd);
    free(wd);

    cli_done(cli);

    close(clientSocket);
    free(html);
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
            char **directories = str_splitter(directory, &cnt);
            inotify((int)cnt, directories, address);
            exit(EXIT_SUCCESS);
        }
    }
    else
    {
        printf("Usage: %s -d PATH -i IP\n", argv[0]);
        exit(EXIT_FAILURE);
    }
}
