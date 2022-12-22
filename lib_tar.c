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
/*
int isInSubDir(char *path,int dir){
    int len = strlen(path)-dir;
    for(int i = 0;i<len;i++){
        if(path[i]=='/') return 1;
    }
    return 0;
}
//dir1/test
//dir1/
int isInPath(char *buffdotname,char *t,int dir){

    char *path = buffdotname;
    char *target = t;
    while(*path=='.' || *path=='/') path++;

    // on enleve le / au debut du
    while(*target=='/' || *target=='.') target++;
    while(*target == *path && *target != '\0' && *path != '\0' ){
        target++;
        path++;
    }

    // on est au point ou a enleve toute la partie commun des 2 chemins
    if(strlen(target)>0) return -1; // cause: pas le meme path
    while(*path=='/') path++; // si ca arrive, j enleve un / en debut du string
    if(strlen(path)==0) return -1; //cause ceci est le meme nom que la cible
    if(isInSubDir(path,dir)==1) return -1;//cause : est dans un sous dossier
    return 1;
}
*/

int list(int tar_fd, char *path, char **entries, size_t *no_entries) {

    long to_go_cumulate = 0;
    int entries_cumulator = 0;

    while (1) {

        tar_header_t * data = malloc(sizeof (tar_header_t));
        if (data == NULL) return

    }

    /*
    int header_size = sizeof(tar_header_t);
    tar_header_t buff;
    ssize_t  r;
    int listed = 0;
    //printf("et c est reaprti %s \n",path);
    while(1){
        r=read(tar_fd,&buff,header_size);
        if(r==-1){
            printf("Une erreur est survenue \n");
            return 0;
        }
        if (*(uint8_t *) &buff == 0) break;
        //printf("%s \n",buff.linkname);
        // printf("- %c \n",buff.typeflag);
        if(buff.typeflag==SYMTYPE){
            if(strcmp(buff.linkname,path) != 0) {//j ai imbrique le if poru les perfs
                lseek(tar_fd, 0, SEEK_SET);
                return list(tar_fd, buff.linkname, entries, no_entries);
            }
        }
        if(isInPath(buff.name,path,(buff.typeflag==DIRTYPE))==1){
            if(listed>=*no_entries){
                printf("on ne peut pas ajouter car on a trop de fichier pour la taille du tableua \n");
            }else{
                strcpy(entries[listed],buff.name);
            }
            listed ++;
        }

        long int jump = (long int) (TAR_INT(buff.size)/512.0 + ((TAR_INT(buff.size)%512) != 0))*512;

        lseek(tar_fd,jump,SEEK_CUR);
    }
    *no_entries = (size_t) listed;
    if(listed>0){
        return 0;
    }else{
        return 1;
    }



long to_go_cumulate = 0;
size_t num_entries = 0;

while(true) {

    tar_header_t * data = malloc(sizeof(tar_header_t));
    if(data == NULL) return 0;

    if(read(tar_fd, data, sizeof(tar_header_t)) == -1){
        printf("an error occurred while reading");
        return 0;
    }

    if (is_dir(tar_fd, path)) return 1;

    if (strncmp(data->name, path, strlen(path)) == 0) {
        if (data->typeflag == DIRTYPE || data->typeflag == REGTYPE || data->typeflag == AREGTYPE) {
            if (num_entries < *no_entries) {
                strcpy(entries[num_entries], data->name);
                num_entries++;
            } else {
                break;
            }
        }
    }

    long to_go = ((TAR_INT(data->size) / 512) * 512);
    if(to_go % 512 > 0){
        to_go += 512;
    }

    to_go_cumulate += to_go;
    lseek(tar_fd, to_go_cumulate, SEEK_CUR);
    //free(data);
    if(is_it_the_end(tar_fd) == true) break;
}

*no_entries = num_entries;
return num_entries > 0;

     return 0;


while(true) {

    tar_header_t * data = malloc(sizeof(tar_header_t));
    if(data == NULL) return 0;

    if(read(tar_fd, data, sizeof(tar_header_t)) == -1){
        printf("an error occurred while reading");
        return 0;
    }

    if(data->typeflag == SYMTYPE){

        long to_go = ((TAR_INT(data->size) / 512) * 512);
        if(to_go % 512 > 0){
            to_go += 512;
        }

        to_go_cumulate += to_go;
        lseek(tar_fd, to_go_cumulate, SEEK_CUR);
        continue;
    }

    if(strncmp(data->name, path, strlen(path)) == 0){
        strncpy(entries[*no_entries], data->name, strlen(data->name));
        (*no_entries)++;
    }

    long to_go = ((TAR_INT(data->size) / 512) * 512);
    if(to_go % 512 > 0){
        to_go += 512;
    }

    to_go_cumulate += to_go;
    lseek(tar_fd, to_go_cumulate, SEEK_CUR);
    if(is_it_the_end(tar_fd) == true) break;
}
 */
/*
    while(true) {

        tar_header_t * data = malloc(sizeof(tar_header_t));
        if(data == NULL) return 0;

        if(read(tar_fd, data, sizeof(tar_header_t)) == -1){
            printf("an error occurred while reading");
            return 0;
        }

        if(data->typeflag == SYMTYPE) {
            if(strcmp(data->linkname, path) != 0){
                lseek(tar_fd, 0, SEEK_SET);
                return list(tar_fd, data->linkname, entries, no_entries);
            }
        }


        long to_go = ((TAR_INT(data->size) / 512) * 512);
        if(to_go % 512 > 0){
            to_go += 512;
        }

        to_go_cumulate += to_go;
        lseek(tar_fd, to_go_cumulate, SEEK_CUR);
        if(is_it_the_end(tar_fd) == true) break;
    }
*/


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