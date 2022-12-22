#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "lib_tar.c"
#include "stddef.h"
//#include "lib_tar.h"

#define ENTRY_PATH_LEN 256

/**
 * You are free to use this file to write tests for your implementation
 */

void debug_dump(const uint8_t *bytes, size_t len) {
    for (int i = 0; i < len;) {
        printf("%04x:  ", (int) i);

        for (int j = 0; j < 16 && i + j < len; j++) {
            printf("%02x ", bytes[i + j]);
        }
        printf("\t");
        for (int j = 0; j < 16 && i < len; j++, i++) {
            printf("%c ", bytes[i]);
        }
        printf("\n");
    }
}

int main(int argc, char **argv) {
    if (argc < 2) {
        printf("Usage: %s tar_file\n", argv[0]);
        return -1;
    }

    int fd = open(argv[1] , O_RDONLY);
    if (fd == -1) {
        perror("open(tar_file)");
        return -1;
    }
    char **entries;
    int num_entries = 5;
    size_t no_entries;
    int a = 5;
    no_entries = (size_t) a;
    size_t *no_entriesptr = &no_entries;
    entries = malloc(num_entries * sizeof(char*));
    for (int i = 0; i < num_entries; i++) {
        entries[i] = malloc(ENTRY_PATH_LEN * sizeof(char));
    }
    int ret = list(fd, "./", entries, no_entriesptr);
    printf("check_archive returned %d\n", ret);
    free(no_entriesptr);
    return 0;
}