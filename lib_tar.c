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
    tar_header_t data;

    if(read(tar_fd, &data, sizeof(tar_header_t)) < sizeof(tar_header_t)) return true;

    if(data.typeflag == 0) return true;
    lseek(tar_fd, 0, SEEK_SET);
    return false;
}

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

    while(true) {

        tar_header_t * data = malloc(sizeof(tar_header_t));
        if(data == NULL) return 0;

        if(read(tar_fd, data, sizeof(tar_header_t)) == -1){
            printf("an error occurred while reading");
            return -1;
        }

        // Check end-of-file
        if (*(uint8_t *) data == 0) break;

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

        // Compute nb of bytes until next header or end of file
        long to_go = ((TAR_INT(data->size) / 512) * 512);
        if ((TAR_INT(data->size)%512) != 0) {
            to_go+=512;
        }

        nb_of_headers++;

        // Add jump size to to_go_cumulate and move file offset
        to_go_cumulate += to_go;
        lseek(tar_fd, to_go, SEEK_CUR);
        free(data);
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

/** ADDED BY STUDENT
 * Check if the file or folder in the given path is in a subdirectory
 *
 * Example:
 *  dir/          is_a_subdir("dir/d", 0) will return 1, is_a_subdir("a", 0) will return 0
 *   ├── a        is_a_subdir("c/", 1) will return 0
 *   ├── b
 *   ├── c/
 *   │   └── d
 *   └── e/
 *
 * @param path A path to an entry in the archive. If the entry is a symlink, it must be resolved to its linked-to entry.
 * @param dir A value equal to 1 if the path given points to a directory
 *
 * @return zero if file/directory is inside a directory, 1 otherwise
 */
int is_a_subdir(char *path,int dir){
    int found = 0;
    int len = strlen(path);
    for(int i=0;i<len-dir;i++){
        if(path[i]=='/') found = 1;
    }
    return (found);
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
/*
int check_symlink_content(int tar_fd, long offset, int * dirptr, char * filename) {
    tar_header_t * data = malloc(sizeof(tar_header_t));
    if(data == NULL) return -1;

    lseek(tar_fd, offset, SEEK_SET);
    if(read(tar_fd, data, sizeof(tar_header_t)) == -1){
        //printf("an error occurred while reading");
        return 0;
    }

    char * path = data->linkname;
    // first remove "." and "/"
    if (path[strlen(path)-1] == '/') *dirptr=1;
    int slashcnt = 0;
    for (int i = 0; i < strlen(path)-1-(*dirptr); ++i) {
        if (path[i] == '/') slashcnt++;
    }
    while (slashcnt != 0) {
        char curr = *path;
        if (curr == '/'){
            slashcnt-=1;
        }
        path++;
    }
    strcpy(filename, path);
    return 1;
}
*/

int list(int tar_fd, char *path, char **entries, size_t *no_entries) {

    // Used to set offset
    long to_go_cumulate = 0;
    // used to set the in-out arg and compute the nb of directories
    int entries_cumulator = 0;
    int no_directories = 0;


    // Loop until EOF
    while (1) {
        tar_header_t * data = malloc(sizeof(tar_header_t));
        if(data == NULL) return 0;

        if(read(tar_fd, data, sizeof(tar_header_t)) == -1){
            //printf("an error occurred while reading");
            return 0;
        }

        // check EOF
        if (*(uint8_t *) data == 0) break;

        // here for debugging purpose
        //printf("linkname : %s \n", data->linkname);
        //printf("type : %c \n", data->typeflag);
        // check if current file has symlink
        if (data->typeflag == SYMTYPE) {
            int * dir = malloc(sizeof (int ));
            *dir = 0;
            if (data->typeflag == DIRTYPE) {
                no_directories++;
                *dir =1;
            }
            if (!(is_a_subdir(data->name, *dir))) {
                int dir = 0;
                int *dirptr = &dir;
                // 6 cases for filename : 1. "../file(/)", "file(/)", "directory/file(/)",  (/) have been added because the filename can eventually be a directory
                // This pointer will also be used to store the filename/directory name on the entry list
                char *filename = data->linkname;
                //check_symlink_content(tar_fd, to_go_cumulate, dirptr, filename);
                char * path = data->linkname;
                // first remove "." and "/"
                if (path[strlen(path)-1] == '/') *dirptr=1;
                int slashcnt = 0;
                for (int i = 0; i < strlen(path)-1-(*dirptr); ++i) {
                    if (path[i] == '/') slashcnt++;
                }
                while (slashcnt != 0) {
                    char curr = *path;
                    if (curr == '/'){
                        slashcnt-=1;
                    }
                    path++;
                }
                strcpy(filename, path);
                if (dir) {
                    no_directories++;
                }
                if (entries_cumulator < *(int *) no_entries) strcpy(entries[entries_cumulator], filename);
                entries_cumulator++;
            }
        }
        else {
        // Check if the current entry is a dir, update no_directories if so
        // Also, update entries array and no_entries variable IF enough space is available in entries array
        int dir =0;
        if (data->typeflag == DIRTYPE) {
            no_directories++;
            dir =1;
        }

        // If current entry is in a listed directory, skip it (this function does not recurse in folders
        if (!(is_a_subdir(data->name, dir))) {
            char * filename = data->name;
            int intnoentries = *(int *) no_entries;
            if (entries_cumulator<intnoentries) {
                strcpy(entries[entries_cumulator], filename);
            }
            //printf("filename %s\n", data->name);
            entries_cumulator++;
        }
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

    if(path == NULL || strlen(path) == 0) return -1;
    if(dest == NULL) return -1;

    tar_header_t * data = malloc(sizeof(tar_header_t));
    if(data == NULL) return -1;

    ssize_t reading = 0;
    long to_go_cumulate = 0;

    while(true) {

        if(read(tar_fd, data, sizeof(tar_header_t)) == -1){
            printf("an error occurred while reading");
            return 0;
        }


        if (strcmp(data->name, path) == 0){
            if(data->typeflag != REGTYPE && data->typeflag != AREGTYPE) return -1;
        }


        long to_go = ((TAR_INT(data->size) / 512) * 512);
        //if(to_go % 512 > 0){
        //    to_go += 512;
        //}

        if(offset > to_go) return -2;
        lseek(tar_fd, offset, SEEK_CUR);
        reading = read(tar_fd, dest, *len);
        *len = reading;

        if(reading < to_go){
            return to_go - reading;
        } else if(reading == to_go){
            return 0;
        } else {
            to_go_cumulate += to_go;
            lseek(tar_fd, to_go_cumulate, SEEK_CUR);
            if(is_it_the_end(tar_fd) == true) break;
        }

    }

    return -1;
}