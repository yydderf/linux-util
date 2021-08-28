#define _XOPEN_SOURCE 600
#include <sys/inotify.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ftw.h>
#include <limits.h>
#include <errno.h>
#include <getopt.h>
#include <unistd.h>
#include <ctype.h>

/* make the size of event_buffer 10 times of an event */
#define BUF_LEN (10 * (sizeof(struct inotify_event) + NAME_MAX + 1))

int inotifyfd, wd;
int pkgcnt;
char *renameName;
char *wdlist[128];

static void
usage(const char *progName, const char *msg)
{
    if (msg != NULL)
        printf("%s\n", msg);
    printf("Usage: %s [-d] [-m] [-p] directory-path\n", progName);
    printf("\t-d\tUse FTW_DEPTH flag\n");
    printf("\t-m\tUse FTW_MOUNT flag\n");
    printf("\t-p\tUse FTW_PHYS  flag\n");
    exit(EXIT_SUCCESS);
}

static void
checkCreate(struct inotify_event *i)
{
    char tmpPath[NAME_MAX];
    if (i->mask & IN_CREATE && i->mask & IN_ISDIR) {
        sprintf(tmpPath, "%s/%s", wdlist[i->wd-1], i->name);
        printf("Dir  Created: %s, adding directory to watch list.\n", tmpPath);
        wd = inotify_add_watch(inotifyfd, tmpPath, IN_ALL_EVENTS);
        if (wd == -1) {
            fprintf(stderr, "Failed to add %s to watch list.\nErrno: %d\n", tmpPath, errno);
            exit(EXIT_FAILURE);
        }
        wdlist[wd-1] = strdup(tmpPath);
    }
    else if (i->mask & IN_CREATE)
        printf("File Created: %s/%s\n", wdlist[i->wd-1], i->name);
}

static void
checkMove(struct inotify_event *i)
{
    if (i->mask & IN_MOVED_FROM) {
        printf("%s Moved:   %s/%s", (i->mask & IN_ISDIR) ? "Dir " : "File"
                                , wdlist[i->wd-1], i->name);
    }
    else if (i->mask & IN_MOVED_TO) {
        char tmpName[NAME_MAX];
        sprintf(tmpName, "%s/%s", wdlist[i->wd-1], i->name);
        if (pkgcnt == 1) {
            printf("%s Moved in: %s\n", (i->mask & IN_ISDIR) ? "Dir " : "File"
                                       , tmpName);
            wd = inotify_add_watch(inotifyfd, tmpName, IN_ALL_EVENTS);
            if (wd == -1) {
                fprintf(stderr, "Failed to add %s to watch list.\nErrno: %d\n", tmpName, errno);
                exit(EXIT_FAILURE);
            }
            wdlist[wd-1] = strdup(tmpName);
        }
        else {
            printf(" --> %s\n", tmpName);
            renameName = strdup(tmpName);
        }
    }
    else if (i->mask & IN_MOVE_SELF) {
        free(wdlist[i->wd-1]);
        wdlist[i->wd-1] = strdup(renameName);
        free(renameName);
    }
}

static void
checkDelete(struct inotify_event *i)
{
    if (i->mask & IN_DELETE)
        printf("%s Deleted: %s/%s.\n", (i->mask & IN_ISDIR) ? "Dir " : "File"
                                     , wdlist[i->wd-1], i->name);
}

static void
checkInotifyStatus(struct inotify_event *i)
{
    if (i->mask & IN_IGNORED)
        printf("Watch Descriptor: %d was removed from the watch list.\n", i->wd, i->name);
    if (i->mask & IN_Q_OVERFLOW)
        printf("Queue overflowed.\n");
    if (i->mask & IN_UNMOUNT)
        printf("Filesystem containing object: %s/%s was unmounted.\n", wdlist[i->wd-1], i->name);
}

static void
displayInotifyEvent(struct inotify_event *i)
{
    printf("    wd:%2d; ", i->wd);
    if (i->cookie > 0)
        printf("cookie:%4d; ", i->cookie);
    printf("mask: ");
    if (i->mask & IN_ACCESS)        printf("\t");
    if (i->mask & IN_ATTRIB)        printf("\t");
    if (i->mask & IN_CREATE)        printf("IN_CREATE\t");
    if (i->mask & IN_DELETE)        printf("IN_DELETE\t");
    if (i->mask & IN_DELETE_SELF)   printf("IN_DELETE_SELF\t");
    if (i->mask & IN_IGNORED)       printf("IN_IGNORED\t");
    if (i->mask & IN_ISDIR)         printf("IN_ISDIR\t");
    if (i->mask & IN_MOVE_SELF)     printf("IN_MOVE_SELF\t");
    if (i->mask & IN_MOVED_FROM)    printf("IN_MOVED_FROM\t");
    if (i->mask & IN_MOVED_TO)      printf("IN_MOVED_TO\t");
    if (i->mask & IN_OPEN)          printf("IN_OPEN\t");
    if (i->mask & IN_Q_OVERFLOW)    printf("IN_Q_OVERFLOW\t");
    if (i->mask & IN_UNMOUNT)       printf("IN_UNMOUNT\t");
    printf("\n");
    if (i->len > 0)
        printf("        name: %s\n", i->name);
}

static int
dirTree(const char *pathname, const struct stat *sbuf, int type, struct FTW *ftwb)
{
    if (type == FTW_D) {
        wd = inotify_add_watch(inotifyfd, pathname, IN_ALL_EVENTS);
        if (wd == -1) {
            fprintf(stderr, "Failed to add %s to watch list.\nErrno: %d\n", pathname, errno);
            exit(EXIT_FAILURE);
        }
        wdlist[wd-1] = strdup(pathname);
        printf("Watching %s using wd: %d\n", pathname, wd);
    }
    return 0;
}

int
main(int argc, char **argv)
{
    if (argc < 2 || strcmp(argv[1], "--help") == 0)
        usage(argv[0], NULL);

    int flags, c;
    char buf[BUF_LEN];
    ssize_t numRead;
    char *ptr;
    struct inotify_event *event;
    opterr = 0;

    while ((c = getopt(argc, argv, "dmp")) != -1)
        switch(c)
        {
            case 'd':
                flags |= FTW_DEPTH;
                break;
            case 'm':
                flags |= FTW_MOUNT;
                break;
            case 'p':
                flags |= FTW_PHYS;
                break;
            case '?':
                if (isprint(optopt))
                    fprintf(stderr, "Unknown option `-%c'.\n", optopt);
                else
                    fprintf(stderr, "Unknown option character `\\x%x'.\n", optopt);
                return 1;
            default:
                usage(argv[0], NULL);
        }
    if (argc > optind + 1)
        usage(argv[0], NULL);

    inotifyfd = inotify_init();
    if (inotifyfd == -1) {
        fprintf(stderr, "Failed to initiate inotify instance.\nErrno: %d\n", errno);
        exit(EXIT_FAILURE);
    }

    if (nftw((argc > optind) ? argv[optind] : ".", dirTree, 10, flags) == -1) {
        perror("nftw");
        exit(EXIT_FAILURE);
    }

    for (;;) {
        pkgcnt = 0;
        numRead = read(inotifyfd, buf, BUF_LEN);
        if (numRead == 0) {
            fprintf(stderr, "read from inotify fd returned 0.\n");
            exit(EXIT_FAILURE);
        }
        if (numRead == -1) {
            fprintf(stderr, "Failed to read from inotify fd.\nErrno: %d\n", errno);
            exit(EXIT_FAILURE);
        }
        // printf("Read %ld bytes from inotify fd.\n", (long)numRead);
        for (ptr = buf; ptr < buf + numRead; ) {
            pkgcnt++;
            event = (struct inotify_event *)ptr;
            // displayInotifyEvent(event);
            checkCreate(event);
            checkMove(event);
            checkDelete(event);
            checkInotifyStatus(event);
            ptr += sizeof(struct inotify_event) + event->len;
        }
    }
    exit(EXIT_SUCCESS);
}
