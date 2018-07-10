#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>

#ifdef NEED_MEMSET
#define LOCK_FILE "/tmp/.memex.lock"
#else
#define LOCK_FILE "/tmp/.memexno.lock"
#endif
#define ALLOC_SIZE_DEFAULT 10*1024*1024

typedef struct _Node
{
    void *mem;
    struct _Node *next;
} Node;
    
Node nodeList;
Node *head = &nodeList;

static unsigned int space = ALLOC_SIZE_DEFAULT;

int allocmem()
{
    void *m = malloc(space);
    if (!m) {
        if (space > 128) {
            space /= 2;
            printf("Changing target space down to %d\n", space);
        }

        printf("malloc() failed with space %d.\n", space);
        return 1;
    }

#ifdef NEED_MEMSET
    memset(m, 0, space);
#endif

    printf("Occupying %d bytes on %p.\n", space, m);

    Node *newNode;
    newNode = (Node *)malloc(sizeof(Node));

    newNode->mem = m;

    newNode->next = head;
    head = newNode;

    return 0;
}

int freemem()
{
    // reset allocation and memset flag
    space = ALLOC_SIZE_DEFAULT;

    if (NULL == head->mem) {
        printf("No mem info stored.\n");
        return 1;
    } else {
        while(head->next) {

            if (head->mem) {
                printf("Ready to free space on %p.\n", head->mem);
                free(head->mem);
            }

            Node *tmp = head;

            head = head->next;
            free(tmp);
        }
    }

    return 0;
}

void signal_handler(sig)
{
    switch(sig) {
        case SIGUSR1:
            printf("SIGUSR1 caught! Trying to occupy additional mem.\n");
            allocmem();
            break;
        case SIGUSR2:
            printf("SIGUSR2 caught! Free all occupied mem.\n");
            freemem();
            break;
        default:
            break;
    }
}

int main(int argc, char **argv)
{
    int lfd;
    pid_t pid, sid;
   
    pid = fork();

    if (pid < 0) {
        printf("fork() got failed!\n");
        exit (1);
    } else if (pid > 0) {
        exit (0);
    } else if (0 == pid) {
    }

    umask(0);

    sid = setsid();
    if (sid < 0) {
        printf("setsid() got failed!\n");
        exit (1);
    }

    if (chdir("/") < 0) {
        printf("chdir() got failed!\n");
        exit (1);
    }

    // only one instace is allowed
    lfd = open(LOCK_FILE, O_RDWR|O_CREAT, 0640);
    if (lfd < 0) {
        printf("Opening lock file got failed!\n");
        exit (1);
    }
    if (lockf(lfd, F_TLOCK, 0) < 0) {
        printf("Multiple instances are not allowed, program is running.\n");
        exit (0);
    }

    signal(SIGCHLD, SIG_IGN);
    signal(SIGUSR1, signal_handler);
    signal(SIGUSR2, signal_handler);

#ifdef NEED_MEMSET
    printf("Memory Exhaustion Daemon is running @%d...\n", getpid());
#else
    printf("Memory Exhaustion(w/o memset) Daemon is running @%d...\n", getpid());
#endif

    while(1) sleep(1);

    exit (0);
}
