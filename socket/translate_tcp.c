#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void
translate(char *hexstring, size_t len, size_t set_size)
{
    int i;
    // declare hex as automatic variable would result in error
    char *hex = (char *)calloc(10, sizeof(char));
    char *ptr = hexstring;
    ptr += len;
    for (i = 0; i < len / set_size; i++) {
        if (i) printf(".");
        ptr -= set_size;
        strncpy(hex, ptr, set_size);
        printf("%ld", strtol(hex, NULL, 16));
    }
    free(hex);
}

char
*split(char *ipaddr_port, char **ipaddr, char **port)
{
    char *ptr;
    ptr = strchr(ipaddr_port, ':');
    *ptr++ = 0;
    *ipaddr = ipaddr_port;

    *port = ptr;
    ptr = strchr(ptr, ' ');
    *ptr++ = 0;
    return ptr;
}

int
main(int argc, char **argv)
{
    FILE *fd;
    int nbytes;
    char buf[2048];
    char *ptr, *ipaddr, *port, *ripaddr, *rport;
    if ((fd = fopen("/proc/net/tcp", "r")) == NULL) {
        fprintf(stderr, "Failed to open the file.\n");
        return 1;
    }
    fgets(buf, 2048, fd);
    while ((fgets(buf, 2048, fd)) != NULL) {
        if ((ptr = strchr(buf, ':')) == NULL) {
            fprintf(stderr, "Failed to get any ipaddress.\n");
            fclose(fd);
            return 1;
        }
        ptr += 2;
        ptr = split(ptr, &ipaddr, &port);
        // ptr = split(ptr, &ripaddr, &rport);
        printf("ipaddr: %s, port: %s, ", ipaddr, port);
        translate(ipaddr, 8, 2);
        printf(":");
        translate(port, 4, 4);

        printf(" -> ");

        split(ptr, &ripaddr, &rport);
        translate(ripaddr, 8, 2);
        printf(":");
        translate(rport, 4, 4);
        printf("\n");
    }
    return 0;
}
