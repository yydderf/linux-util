#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pwd.h>
#include <errno.h>
#include <dirent.h>

typedef struct proc Proc;

struct proc
{
    Proc *pproc;
    Proc *cproc;
    char *pname;
    pid_t pid;
};

static void
listDir(char *path)
{
    DIR *dirptr;
    struct dirent *dp;

    while (path[strlen(path) - 1] == '/' && strlen(path) > 1) path[strlen(path) - 1] = 0;

    dirptr = opendir(path);
    if (dirptr == NULL) {
        fprintf(stderr, "Failed to open the directory: %s\nErrno: %d\n", path, errno);
        exit(EXIT_FAILURE);
    }
    while ((dp = readdir(dirptr))) {
        errno = 0;
        if (strcmp(dp->d_name, ".") == 0 || strcmp(dp->d_name, "..") == 0) continue;
        printf("%s/%s\n", path, dp->d_name);
    }
    if (errno != 0) {
        fprintf(stderr, "Failed to read file in directory: %s\nErrno: %d\n", errno);
        exit(EXIT_FAILURE);
    }
}

char*
getParent(char *path)
{
    FILE *fp;
    char *ptr;
    char buf[256];
    strcat(path, "/status");
    if ((fp = fopen(path, "r")) != NULL) {
        while (fgets(buf, sizeof(buf), fp)) {
            if (ptr = strstr(buf, "PPid:")) {
                ptr = strchr(buf, '\t');
                *ptr++ = 0;
                if (fclose(fp) != 0) {
                    fprintf(stderr, "Failed to close file: %s/status\n", path);
                    exit(EXIT_FAILURE);
                }
                return ptr;
            }
        }
    } else if (errno == ENOENT) {
        fprintf(stderr, "%s does not exist.\n", path);
        return "-1";
    }
    fprintf(stderr, "Unknown Error\nErrno: %d\n", errno);
    exit(EXIT_FAILURE);
}

char*
getProcName(char *path)
{
    FILE *fp;
    char *ptr;
    char *pname_ptr;
    char buf[256];
    // strcat(path, "/status");
    if ((fp = fopen(path, "r")) != NULL) {
        while (fgets(buf, sizeof(buf), fp)) {
            if (ptr = strstr(buf, "Name:")) {
                ptr = strchr(ptr, '\t');
                *ptr++ = 0;
                ptr[strlen(ptr) - 1] = 0;
                if (fclose(fp) != 0) {
                    fprintf(stderr, "Failed to close file: %s\n", path);
                    exit(EXIT_FAILURE);
                }
                pname_ptr = strdup(ptr);
                return pname_ptr;
            }
        }
    } else if (errno == ENOENT) {
        fprintf(stderr, "%s does not exist.\n", path);
        return NULL;
    }
    fprintf(stderr, "Unknown Error\nErrno: %d\n", errno);
    exit(EXIT_FAILURE);
}

int
main(int argc, char **argv)
{
    if (argc >= 2 && strcmp(argv[1], "--help") == 0) {
        printf("Usage: %s [pid...]\n", argv[0]);
        exit(EXIT_SUCCESS);
    }
    if (argc < 2) {
        fprintf(stderr, "Try '%s --help' for more information.\n", argv[0]);
        exit(EXIT_SUCCESS);
    }
    for (argv++; *argv; argv++) {
        int flag = 0;
        for (char *tmpptr = *argv; *tmpptr; tmpptr++) {
            if (*tmpptr > '9' || *tmpptr < '0') {
                flag = 1;
            }
        }
        if (flag) {
            if (strcmp(*argv, "self")) {
                fprintf(stderr, "%s is not a valid number.\n", *argv);
                continue;
            }
        }
        Proc *node_proc = (Proc *)malloc(sizeof(Proc));
        Proc *curr_proc = (Proc *)malloc(sizeof(Proc));
        curr_proc = node_proc;
        node_proc->cproc = NULL;
        char path[20];
        pid_t pid;
        sprintf(path, "/proc/%s", *argv);
        while ((pid = atol(getParent(path))) != 0 && pid != -1) {
            Proc *parent_proc = (Proc *)malloc(sizeof(Proc));
            curr_proc->pproc = parent_proc;
            curr_proc->pname = getProcName(path);
            parent_proc->pproc = NULL;
            parent_proc->cproc = curr_proc;
            parent_proc->pid = pid;
            curr_proc = parent_proc;
            sprintf(path, "/proc/%ld", pid);
        }
        if (pid != 0) continue;
        curr_proc->pname = getProcName(path);
        while (curr_proc->pid) {
            printf("%ld(%s)───", curr_proc->pid, curr_proc->pname);
            free(curr_proc->pname);
            curr_proc = curr_proc->cproc;
            free(curr_proc->pproc);
        }
        printf("%s(%s)\n", *argv, node_proc->pname);
        free(node_proc->pname);
        free(node_proc);
    }
    exit(EXIT_SUCCESS);
}
