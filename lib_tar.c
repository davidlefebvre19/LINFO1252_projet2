#include <string.h>
#include <stdbool.h>
#include <stdio.h>
#include <unistd.h>
#include "lib_tar.h"

#define block_size 512

/**
 * A function which returns true if we are at the end of the tar archive and false otherwise
 */

bool first_empty_block(int tar_fd){
    char the_block[block_size];
    ssize_t reading = pread(tar_fd, the_block, block_size, lseek(tar_fd, 0, SEEK_CUR));
    if(reading < 0) return false;

    for (int i = 0; i < block_size; ++i) {
        if(the_block[i] != 0){
            return false;
        }
    }

    lseek(tar_fd, block_size, SEEK_CUR);
    return true;
}

/*
bool first_empty_block(int tar_fd){

    off_t to_go = lseek(tar_fd, 0, SEEK_CUR);

    char the_block[block_size];

    ssize_t reading = read(tar_fd, the_block, block_size);
    if(reading < 0) return false;

    for (int i = 0; i < block_size; ++i) {
        if(the_block[i] != 0){
            lseek(tar_fd, to_go, SEEK_SET);
            return false;
        }
    }

    lseek(tar_fd, to_go, SEEK_SET);
    return true;

}
 */

bool is_it_the_end(int tar_fd){

    off_t to_go = lseek(tar_fd, 0, SEEK_CUR);

    char first_block[block_size];
    char second_block[block_size];

    ssize_t reading = read(tar_fd, first_block, block_size);
    if(reading < 0) return false;

    reading = read(tar_fd, second_block, block_size);
    if(reading < 0) return false;

    for (int i = 0; i < block_size; ++i) {
        if(first_block[i] != 0 || second_block[i] != 0){
            lseek(tar_fd, to_go, SEEK_SET);
            return false;
        }
    }
    lseek(tar_fd, to_go, SEEK_SET);
    return true;
}

/**
 * Checks whether the archive is valid.
 *
 * Each non-null header of a valid archive has:
 *  - a magic value of "ustar" and a null,
 *  - a version value of "00" and no null,
 *  - a correct checksum
 *
 * @param tar_fd A file descriptor pointing to the start of a file supposed to contain a tar archive.
 *
 * @return a zero or positive value if the archive is valid, representing the number of non-null headers in the archive,
 *         -1 if the archive contains a header with an invalid magic value,
 *         -2 if the archive contains a header with an invalid version value,
 *         -3 if the archive contains a header with an invalid checksum value
 */

int check_archive(int tar_fd) {

    int nb_of_headers = 0;
    long to_go_cumulate = 0;

    bool first_block_empty = false;

    while(true) {

        if(first_empty_block(tar_fd)){
            if(first_block_empty == true){
                break;
            } else {
                first_block_empty = true;
                //to_go_cumulate += 512;
                continue;
            }
        }

        tar_header_t * data = malloc(sizeof(tar_header_t));
        if(data == NULL) return 0;

        if(read(tar_fd, data, sizeof(tar_header_t)) == -1){
            printf("an error occurred while reading");
            return 0;
        }

        if(strncmp(data->magic, TMAGIC, TMAGLEN-1) != 0) return -1;
        if(strncmp(data->version, TVERSION, TVERSLEN-1) != 0) return -2;

        long int chksum = TAR_INT(data->chksum);
        memset(data->chksum, 32, 8);

        uint8_t  *ptr = (uint8_t *) data;
        uint tot = 0;

        for (int i = 0; i < sizeof(tar_header_t); ++i) {
            tot += *ptr;
            ptr++;
        }

        if(chksum != tot) return -3;

        long to_go = ((TAR_INT(data->size) / 512) * 512);
        if(to_go % 512 > 0){
            to_go += 512;
        }

        to_go_cumulate += to_go;
        lseek(tar_fd, to_go_cumulate, SEEK_CUR);
        nb_of_headers++;
        free(data);
        if(is_it_the_end(tar_fd)) break;

    }
    return nb_of_headers;
}



/**
 * Checks whether an entry exists in the archive.
 *
 * @param tar_fd A file descriptor pointing to the start of a valid tar archive file.
 * @param path A path to an entry in the archive.
 *
 * @return zero if no entry at the given path exists in the archive,
 *         any other value otherwise.
 */
int exists(int tar_fd, char *path) {

    long to_go_cumulate = 0;

    while(true) {

        tar_header_t * data = malloc(sizeof(tar_header_t));
        if(data == NULL) return 0;

        if(read(tar_fd, data, sizeof(tar_header_t)) == -1){
            printf("an error occurred while reading");
            return 0;
        }

        if(strcmp(data->name, path) == 0) return 1;

        long to_go = ((TAR_INT(data->size) / 512) * 512);
        if(to_go % 512 > 0){
            to_go += 512;
        }

        to_go_cumulate += to_go;
        lseek(tar_fd, to_go_cumulate, SEEK_CUR);
        free(data);
        if(is_it_the_end(tar_fd)) break;
    }
    return 0;
}

/**
 * Checks whether an entry exists in the archive and is a directory.
 *
 * @param tar_fd A file descriptor pointing to the start of a valid tar archive file.
 * @param path A path to an entry in the archive.
 *
 * @return zero if no entry at the given path exists in the archive or the entry is not a directory,
 *         any other value otherwise.
 */
int is_dir(int tar_fd, char *path) {

    long to_go_cumulate = 0;

    while(true) {

        tar_header_t * data = malloc(sizeof(tar_header_t));
        if(data == NULL) return 0;

        if(read(tar_fd, data, sizeof(tar_header_t)) == -1){
            printf("an error occurred while reading");
            return 0;
        }

        if(strcmp(data->name, path) == 0 && data->typeflag == DIRTYPE) return 1;

        long to_go = ((TAR_INT(data->size) / 512) * 512);
        if(to_go % 512 > 0){
            to_go += 512;
        }

        to_go_cumulate += to_go;
        lseek(tar_fd, to_go_cumulate, SEEK_CUR);
        if(is_it_the_end(tar_fd) == true) break;
    }
    return 0;
}

/**
 * Checks whether an entry exists in the archive and is a file.
 *
 * @param tar_fd A file descriptor pointing to the start of a valid tar archive file.
 * @param path A path to an entry in the archive.
 *
 * @return zero if no entry at the given path exists in the archive or the entry is not a file,
 *         any other value otherwise.
 */
int is_file(int tar_fd, char *path) {

    long to_go_cumulate = 0;

    while(true) {

        tar_header_t * data = malloc(sizeof(tar_header_t));
        if(data == NULL) return 0;

        if(read(tar_fd, data, sizeof(tar_header_t)) == -1){
            printf("an error occurred while reading");
            return 0;
        }

        if(strcmp(data->name, path) == 0 && data->typeflag == REGTYPE) return 1;

        long to_go = ((TAR_INT(data->size) / 512) * 512);
        if(to_go % 512 > 0){
            to_go += 512;
        }

        to_go_cumulate += to_go;
        lseek(tar_fd, to_go_cumulate, SEEK_CUR);
        if(is_it_the_end(tar_fd) == true) break;
    }
    return 0;
}

/**
 * Checks whether an entry exists in the archive and is a symlink.
 *
 * @param tar_fd A file descriptor pointing to the start of a valid tar archive file.
 * @param path A path to an entry in the archive.
 * @return zero if no entry at the given path exists in the archive or the entry is not symlink,
 *         any other value otherwise.
 */
int is_symlink(int tar_fd, char *path) {

    long to_go_cumulate = 0;

    while(true) {

        tar_header_t * data = malloc(sizeof(tar_header_t));
        if(data == NULL) return 0;

        if(read(tar_fd, data, sizeof(tar_header_t)) == -1){
            printf("an error occurred while reading");
            return 0;
        }

        if(strcmp(data->name, path) == 0 && data->typeflag == SYMTYPE) return 1;

        long to_go = ((TAR_INT(data->size) / 512) * 512);
        if(to_go % 512 > 0){
            to_go += 512;
        }

        to_go_cumulate += to_go;
        lseek(tar_fd, to_go_cumulate, SEEK_CUR);
        if(is_it_the_end(tar_fd) == true) break;
    }
    return 0;
}


/**
 * Lists the entries at a given path in the archive.
 * list() does not recurse into the directories listed at the given path.
 *
 * Example:
 *  dir/          list(..., "dir/", ...) lists "dir/a", "dir/b", "dir/c/" and "dir/e/"
 *   ├── a
 *   ├── b
 *   ├── c/
 *   │   └── d
 *   └── e/
 *
 * @param tar_fd A file descriptor pointing to the start of a valid tar archive file.
 * @param path A path to an entry in the archive. If the entry is a symlink, it must be resolved to its linked-to entry.
 * @param entries An array of char arrays, each one is long enough to contain a tar entry path.
 * @param no_entries An in-out argument.
 *                   The caller set it to the number of entries in `entries`.
 *                   The callee set it to the number of entries listed.
 *
 * @return zero if no directory at the given path exists in the archive,
 *         any other value otherwise.
 */
int is_a_subdir(char *path,int dir){
    int found = 0;
    int len = strlen(path);
    for(int i=0;i<len-dir;i++){
        if(path[i]=='/') found = 1;
    }
    return (found);
}

int list(int tar_fd, char *path, char **entries, size_t *no_entries) {

    // Used to set offset
    long to_go_cumulate = 0;
    // used to set the in-out arg and compute the nb of directories
    int entries_cumulator = 0;
    int no_directories = 0;

    // Loop until EOF
    while (1) {
        tar_header_t * data = malloc(sizeof(tar_header_t));
        if(data == NULL) return -1;

        if(read(tar_fd, data, sizeof(tar_header_t)) == -1){
            printf("an error occurred while reading");
            return -1;
        }

        // check EOF
        if (is_it_the_end(tar_fd)) break;
        if (*(uint8_t *) data == 0) break;

        // here for debugging purpose
        printf("linkname : %s \n", data->linkname);
        printf("type : %c \n", data->typeflag);

        // check if current file has symlink
        if (data->typeflag == SYMTYPE) {
            lseek(tar_fd, 0, SEEK_SET);
            return list(tar_fd, data->linkname, entries, no_entries); // Update entries and no_entries on the path pointed by the linkname
        }

        // Check if the current entry is a dir, update no_directories if so
        // Also, update entries array and no_entries variable IF enough space is available in entries array
        int dir =0;
        if (data->typeflag == DIRTYPE) {
            no_directories++;
            dir =1;
        }

        // If current entry is in a listed directory, skip it (this function does not recurse in folders
        if (!(is_a_subdir(data->name, dir))) {
            strcpy(entries[entries_cumulator], data->name);
            printf("filename %s\n", data->name);
            entries_cumulator++;
        }

        // Compute nb of bytes until next header or end of file
        long to_go = ((TAR_INT(data->size) / 512) * 512);
        if ((TAR_INT(data->size)%512) != 0) {
            to_go+=512;
        }

        // Add jump size to to_go_cumulate and move file offset
        to_go_cumulate += to_go;
        lseek(tar_fd, to_go, SEEK_CUR);
        free(data);
    }

    // We set no_entries to the number of entries listed
    * no_entries = (size_t) entries_cumulator;
    if (no_directories > 0) return 1;
    return 0;
}

/**
 * Reads a file at a given path in the archive.
 *
 * @param tar_fd A file descriptor pointing to the start of a valid tar archive file.
 * @param path A path to an entry in the archive to read from.  If the entry is a symlink, it must be resolved to its linked-to entry.
 * @param offset An offset in the file from which to start reading from, zero indicates the start of the file.
 * @param dest A destination buffer to read the given file into.
 * @param len An in-out argument.
 *            The caller set it to the size of dest.
 *            The callee set it to the number of bytes written to dest.
 *
 * @return -1 if no entry at the given path exists in the archive or the entry is not a file,
 *         -2 if the offset is outside the file total length,
 *         zero if the file was read in its entirety into the destination buffer,
 *         a positive value if the file was partially read, representing the remaining bytes left to be read to reach
 *         the end of the file.
 *
 */
ssize_t read_file(int tar_fd, char *path, size_t offset, uint8_t *dest, size_t *len) {
    return 0;
}