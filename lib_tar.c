#include <string.h>
#include <stdbool.h>
#include <stdio.h>
#include <unistd.h>
#include "lib_tar.h"

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

    //off_t lseek_size = lseek(tar_fd, 0, SEEK_END); // Size of entire archive
    //int nb_tot_blocks = lseek_size / 512;

    long to_go_cumulate = 0; // The position in the file !

    tar_header_t * data = malloc(sizeof(tar_header_t));
    if(data == NULL) return -1;

    if(read(tar_fd, data, sizeof(tar_header_t)) == -1){
        printf("an error occurred while reading");
        return -1;
    }

    while(strcmp(data->magic, "ustar") != 0 || data->version[0] != '\0'){

        long to_go = ((TAR_INT(data->size) / 512) * 512);
        if(to_go % 512 > 0){
            to_go += 512;
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

        to_go_cumulate += to_go;

        lseek(tar_fd, to_go_cumulate, SEEK_CUR);

        nb_of_headers++;

        if(read(tar_fd, data, sizeof(tar_header_t)) == -1){
            printf("an error occurred while reading");
            return -1;
        }

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

    //off_t lseek_size = lseek(tar_fd, 0, SEEK_END); // Size of entire archive
    //int nb_tot_blocks = lseek_size / 512;

    long to_go_cumulate = 0; // The position in the file !

    tar_header_t * data = malloc(sizeof(tar_header_t));
    if(data == NULL) return -1;


    while(lseek(tar_fd, to_go_cumulate, SEEK_SET) != 1) {

        ssize_t reading = read(tar_fd, data, sizeof(tar_header_t));
        if(reading < sizeof(tar_header_t)) break;

        long to_go = ((TAR_INT(data->size) / 512) * 512);
        if(to_go % 512 > 0){
            to_go += 512;
        }

        if(strcmp(data->name, path) == 0) return 1;

        to_go_cumulate += to_go;

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

    //off_t lseek_size = lseek(tar_fd, 0, SEEK_END); // Size of entire archive
    //int nb_tot_blocks = lseek_size / 512;

    long to_go_cumulate = 0; // The position in the file !

    tar_header_t * data = malloc(sizeof(tar_header_t));
    if(data == NULL) return -1;


    while(lseek(tar_fd, to_go_cumulate, SEEK_SET) != 1) {

        ssize_t reading = read(tar_fd, data, sizeof(tar_header_t));
        if(reading < sizeof(tar_header_t)) break;

        long to_go = ((TAR_INT(data->size) / 512) * 512);
        if(to_go % 512 > 0){
            to_go += 512;
        }

        if(strcmp(data->name, path) == 0 && data->typeflag == DIRTYPE) return 1;

        to_go_cumulate += to_go;

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

    //off_t lseek_size = lseek(tar_fd, 0, SEEK_END); // Size of entire archive
    //int nb_tot_blocks = lseek_size / 512;

    long to_go_cumulate = 0; // The position in the file !

    tar_header_t * data = malloc(sizeof(tar_header_t));
    if(data == NULL) return -1;


    while(lseek(tar_fd, to_go_cumulate, SEEK_SET) != 1) {

        ssize_t reading = read(tar_fd, data, sizeof(tar_header_t));
        if(reading < sizeof(tar_header_t)) break;

        long to_go = ((TAR_INT(data->size) / 512) * 512);
        if(to_go % 512 > 0){
            to_go += 512;
        }

        if(strcmp(data->name, path) == 0 && data->typeflag == REGTYPE) return 1;

        to_go_cumulate += to_go;

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
int list(int tar_fd, char *path, char **entries, size_t *no_entries) {
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