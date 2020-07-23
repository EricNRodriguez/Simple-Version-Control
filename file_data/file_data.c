#include "file_data.h"

/** @brief Removes file from file system.
 *
 *  Wraps remove, for use in fts. If -1 is returned, error message is printed
 *  to stderr and -1 returned. Otherwise, 0 is returned.
 *
 *  @param file_path : path of file to be removed.
 *  @param sb
 *  @param typeflag
 *  @param ftwbuf
 *  @return 0 if successful, -1 if unsuccessful.
 */
static int remove_file(const char *file_path, const struct stat *sb, int typeflag,
        struct FTW *ftwbuf);

int hash_and_copy_file(char *file_path, char **file_copy, size_t *file_size) {
    FILE *f = fopen(file_path, "r");
    if (!f) {
        return -1;
    }

    if (file_copy && file_size) {
        // move file position to end
        fseek(f, 0, SEEK_END);
        // current offset (in bytes)
        *file_size = ftell(f);
        // reset file position to start of stream
        fseek(f, 0, SEEK_SET);
        // allocate space for file content copy
        *file_copy = (char *) safe_malloc(*file_size * sizeof(char));
    }

    // basic hash, copy file only file_cpy and file_size are non NULL
    int hash = 0;
    for (size_t i = 0; i < strlen(file_path); ++i) {
        hash = (hash + file_path[i]) % 1000;
    }
    int c = 0;
    int i = 0;
    while ((c = fgetc(f)) != EOF) {
        if (file_copy && file_size) {
            (*file_copy)[i] = c;
            i++;
        }
        hash = (hash + c) % 2000000000;
    }
    fclose(f);

    return hash;
}

FileData *copy_file_data(FileData *fd, size_t n_file_data, size_t file_data_len) {
    if (!fd) {
        return NULL;
    }

    // deep copy name, shallow copy remaining struct fields
    FileData *fd_cpy = safe_malloc(file_data_len * sizeof(FileData));
    for (size_t i = 0; i < n_file_data; ++i) {
        fd_cpy[i] = fd[i];
        fd_cpy[i].file_path = copy_string(fd_cpy[i].file_path);
    }

    return fd_cpy;
}

void update_file(char *old_file_path, char *new_file_path) {
    if (!new_file_path) {
        return;
    }

    FILE *f_old = fopen(old_file_path, "w+");
    FILE *f_new = fopen(new_file_path, "r");
    if (!f_old || !f_new) {
        perror("unable to open files in file update");
        exit(2);
    }

    // write new file data to old file
    int c;
    while(( c = fgetc(f_new) ) != EOF ) {
        fputc(c, f_old);
    }

    fclose(f_old);
    fclose(f_new);

    return;
}

int is_unknown(FileData *files, size_t n_files, char *file_path) {
    if (!files) {
        return 1;
    }

    // linear search for file name
    for (size_t i = 0; i < n_files; ++i) {
        if (!strcmp(files[i].file_path, file_path) && files[i].state != Deleted) {
            return 0;
        }
    }

    return 1;
}

int remove_file(const char *file_path, const struct stat *sb, int typeflag,
        struct FTW *ftwbuf) {
    // check if access is not permitted
    if (remove(file_path) == -1) {
        fprintf(stderr, "failed to remove file from svc dir during cleanup\n");
        return -1;
    }

    return 0;
}

void remove_svc_directory() {
    // traverse in reverse order, delete all files in directory, and directory
    if (nftw(SVC_DIR_PATH, remove_file, 100,
             FTW_DEPTH|FTW_MOUNT|FTW_PHYS) == -1) {
        fprintf(stderr, "error removing files in svd directory during cleanup\n");
    }

    return;
}
