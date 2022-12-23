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


    //int ret0 = check_archive(fd);
    // printf("check archive val : %d", ret0);

    //int num_entries = 5;
    size_t no_entries;
    int a = 8;
    no_entries = (size_t) a;
    size_t *no_entriesptr = &no_entries;

    size_t st=8;
    char **array = malloc(sizeof(char *)*st);
    for(int i=0;i<st;i++){
        array[i] = (char *)calloc(20,sizeof(char *));
    }
    int ret = list(fd, ".", (char **) array, no_entriesptr);
    printf("list returned %d, the in-out argmt val is : %zu\n", ret, *no_entriesptr);

    for (int i = 0; i < 5; ++i) {
        printf("entries : %s\n", array[i]);
    }
    char *s = malloc(sizeof (char )*6);
    strcpy(s, "kurwa\0");
    return EXIT_SUCCESS;
}