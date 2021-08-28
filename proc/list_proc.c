#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pwd.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>

static void getProcStatus(char *path, long spec_uid);

uid_t
name2id(char *name)
{
    struct passwd *pwd;
    if ((pwd = getpwnam(name)) == NULL) {
        return -1;
    }
    return pwd->pw_uid;
}



static void
listDir(char *path, char *username)
{
    DIR *dirptr;
    struct dirent *dp;
    char filepath[100];
    if (path[strlen(path) - 1] == '/') path[strlen(path) - 1] = '\0';
    dirptr = opendir(path);
    if (dirptr == NULL) {
        fprintf(stderr, "Failed to open directory %s.\nErrno: %d\n", path, errno);
        exit(EXIT_FAILURE);
    }
    for(;;) {
        errno = 0;
        dp = readdir(dirptr);
        if (dp == NULL) break;
        if (strcmp(dp->d_name, ".") == 0|| strcmp(dp->d_name, "..") == 0) continue;
        // printf("%s/%s\n", path, dp->d_name);
        sprintf(filepath, "%s/%s", path, dp->d_name);
        getProcStatus(filepath, name2id(username));
    }
    if (errno != 0) {
        fprintf(stderr, "Failed to get file info from %s.\n%d\n", path, errno);
        exit(EXIT_FAILURE);
    }
    if (closedir(dirptr) == -1) {
        fprintf(stderr, "Failed to close %s.\n%d\n", path, errno);
        exit(EXIT_FAILURE);
    }
}

static void
getProcStatus(char *path, long spec_uid)
{
    FILE *fptr;
    char *ptr, *name, *pidp;
    uid_t uid;
    pid_t pid;
    int nopid = 1;
    char buf[512], st_path[512];
    sprintf(st_path, "%s/status", path);
    if ((fptr = fopen(st_path, "r")) != NULL) {
        // assuming name always comes before pid
        while (fgets(buf, sizeof(buf), fptr)) {
            if (ptr = strstr(buf, "Name:")) {
                ptr = strchr(ptr, '\t');
                *ptr++ = 0;
                ptr[strlen(ptr) - 1] = 0;
                name = strdup(ptr);
            }
            if ((ptr = strstr(buf, "Pid:")) && nopid) {
                ptr = strchr(ptr, '\t');
                *ptr++ = 0;
                pid = atol(ptr);
                nopid = 0;
            }
            if (ptr = strstr(buf, "Uid:")) {
                ptr = strchr(ptr, '\t');
                *ptr++ = 0;
                uid = atol(ptr);
                break;
            }
        }
        if (spec_uid == uid) {
            printf("%-25s%-30s%-6ld%-6ld\n", st_path, name, pid, uid);
        }
        if (fclose(fptr)) {
            fprintf(stderr, "Failed to close file: %s\nErrno: %d\n", st_path, errno);
            exit(EXIT_FAILURE);
        }
        free(name);
    } else if (errno != ENOENT && errno != ENOTDIR) {
        fprintf(stderr, "Failed to open file: %s\nErrno: %d\n", st_path, errno);
        exit(EXIT_FAILURE);
    }
}

int
main(int argc, char **argv)
{
    if (argc == 2 && strcmp(argv[1], "--help") == 0) {
        printf("\
Usage: %s [username]\n\
", argv[0]);
        exit(EXIT_SUCCESS);
    }
    printf("%-25s%-30s%-6s%-6s\n", "Directory", "Name", "Pid", "Uid");
    if (argc == 1) {
        // need to confirm `USER` variable exists in env
        listDir("/proc", getenv("USER"));
    }
    else
        for (++argv; *argv; argv++)
            listDir("/proc", *argv);
    exit(EXIT_SUCCESS);
}
