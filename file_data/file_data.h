#ifndef ASSIGNMENT_2_SVC_FILE_MANAGEMENT_H
#define ASSIGNMENT_2_SVC_FILE_MANAGEMENT_H

#include "../params.h"
#include "../memory/memory.h"
#include <sys/stat.h>
#define __USE_XOPEN_EXTENDED 1
#include <ftw.h>
#include <string.h>
#include <stdio.h>

enum FileState {Tracked = 0, Staged = 2, Deleted = 3};

typedef struct FileData {
    char *file_path;      // null terminated file path
    enum FileState state; // current state of the file
    size_t previous_hash; // previous known hash, only set when state is
                          // Tracked or Deleted
} FileData;

/** @brief Copies file content and returns hash.
 *
 *  If file is not available, -1 is returned and errno is set. If file_copy and
 *  file_size are NOT NULL, file is copied, with address stored in file_copy, and
 *  file_size set accordingly. If file is copied, address stored at file_copy
 *  needs to be released. Hash is always calculated if file access is successful.
 *
 *  @param file_path : path of file to be hashed.
 *  @param file_copy : address for file_copy address to be set.
 *  @param file_size : address for file size to be set.
 *  @return hash of file.
 */
int hash_and_copy_file( char *file_path, char **file_copy, size_t *file_size);

/** @brief Deep copy file data array.
 *
 *  If file data array is NULL, NULL is returned. Else, file data array is copied and
 *  address returned. Every file path is also copied. Copied file data and names
 *  MUST be released.
 *
 *  @param fd : address of file data array.
 *  @param n_file_data : number of files stored.
 *  @param file_data_len : total allocated size of file data array.
 *  @return address of copied array.
 */
FileData *copy_file_data(FileData *fd, size_t n_file_data, size_t file_data_len);

/** @brief Updates content of old file to contents of new file.
 *
 *  If new file path is NULL, nothing is done. If either old file or new file
 *  are unable to be opened, perror is called and exit with status 2 occurs.
 *  Otherwise, file contents old_file_path are copied to new_file_path.
 *
 *  If new file path doesnt lead to an existing file, it is created.
 *
 *  @param old_file_path : path of file contents being copied.
 *  @param new_file_path : address for file for contents to be copied to.
 */
void update_file(char *old_file_path, char *new_file_path);

/** @brief Checks if file_path exists in file_data array.
 *
 *  If files is NULL or file_path is not an entry in files, 1 is returned.
 *  Otherwise, file contains file_path and 0 is returned.
 *
 *  @param files : array of file data.
 *  @param n_files : number of file_data instances in files.
 *  @param file_path : file path to be located.
 *  @return 1 if unknown, 0 if known.
 */
int is_unknown(FileData *files, size_t n_files, char *file_path);

/** @brief Removes svc directory and all files within it.
 *
 *  Directory path SVC_DIR_PATH taken from params.h. If error occurs, appropriate
 *  error messages are printed to stderr.
 */
void remove_svc_directory();

#endif //ASSIGNMENT_2_SVC_FILE_MANAGEMENT_H
