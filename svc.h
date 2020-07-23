#ifndef svc_h
#define svc_h

#include "memory/memory.h"
#include "snapshot/snapshot.h"
#include "commit/commit.h"
#include "branch/branch.h"
#include "params.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

typedef struct resolution {
    char *file_name;       // file path of file with conflicts
    char *resolved_file;   // file path of resolved file contents
} resolution;

/** @brief Initialises svc helper struct.
 *
 *  Creates internal data structure required for following methods.
 *
 *  @return pointer to the struct instance.
 */
void *svc_init(void);

/** @brief Releases all svc allocated memory.
 *
 *  Only call at end of program.
 *
 *  @param helper : address of svc data structure returned from init.
 */
void cleanup(void *helper);

/** @brief Hashes file content.
 *
 *  Hashes file at file path. If file path is NULL, -1 is returned. If no file
 *  exists at the current path, or access is not permitted, -2 is returned. On
 *  successful completion, the hash is returned.
 *
 *  @param helper : address of svc data structure returned from init.
 *  @param file_path : file path of file to be hashed.
 *  @return Hash if successful, error code if unsuccessful.
 */
int hash_file(void *helper, char *file_path);

/** @brief Commits.
 *
 *  Commits current state of all tracked and staged files, with commit message.
 *  If no changes are found, and there are no staged files, NULL is returned.
 *
 *  @param helper : address of svc data structure returned from init.
 *  @param message : commit message
 *  @return Commit id (Hex).
 */
char *svc_commit(void *helper, char *message);

/** @brief Retrieves commit address.
 *
 *  Returns internal address of commit, for use in other svc functions. If
 *  the commit doesnt exist, or commit_id is NULL, NULL is returned. Otherwise,
 *  a valid address is returned. This address will be released during cleanup,
 *  do NOT free.
 *
 *  @param helper : address of svc data structure returned from init.
 *  @param commit_id : commit id (hex).
 *  @return Commit address.
 */
void *get_commit(void *helper, char *commit_id);

/** @brief Retrieves commit ids of previous commits.
 *
 *  Returns address of array, containing all commit id's of previous commits.
 *  If n_prev is NULL, nothing is done and NULL is returned. If commit is NULL
 *  or it has no parent commits (first commit), n_prev is set to 0, and NULL is
 *  returned. Otherwise, n_prev is set to the allocated length of the returned
 *  array of commit id's. This array is NOT released during cleanup. Individual
 *  commit id's ARE released, however.
 *
 *  @param helper : address of svc data structure returned from init.
 *  @param commit : commit id (Hex).
 *  @param n_prev : address to assign length of returned array.
 *  @return Commit id's of parent commits.
 */
char **get_prev_commits(void *helper, void *commit, int *n_prev);

/** @brief Prints commit data to stdout.
 *
 *  Prints commit data in the format presented below. If helper or commit_id are
 *  NULL, nothing is printed and NULL is returned. If the commit id is invalid,
 *  Invalid commit id is printed to stdout.
 *
 *  Format of output:
 *
 *  <commit id (hex)> [<branch name>] : <commit message>
 *      + <staged file name>
 *      - <deleted file name>
 *      / <changed file name> [<previous hash> -> <new hash>]
 *
 *      Tracked files (<number of tracked files>):
 *      [<hash of trackec file>] <file name>
 *
 *
 *  @param helper : address of svc data structure returned from init.
 *  @param message : commit message
 *  @return Commit id (Hex).
 */
void print_commit(void *helper, char *commit_id);

/** @brief Creates new branch.
 *
 *  Creates a new branch, with the name branch_name. If branch name is invalid,
 *  -1 is returned. Valid branch names are specified according to the expression
 *  [0-9a-zA-Z/_-]+$. If branch name exists, -2 is returned and nothing is done.
 *  If there are uncommited changes, -3 is returned and nothing is done.
 *
 *  @param helper : address of svc data structure returned from init.
 *  @param branch_name : null terminated branch name.
 *  @return 0 if successful, error code if unsuccessful.
 */
int svc_branch(void *helper, char *branch_name);

/** @brief Checks out existing branch.
 *
 *  If branch doesnt exist, -1 is returned. If uncommitted changes exist, -2 is
 *  returned. Otherwise, branch is checked out and 0 is returned.
 *
 *  @param helper : address of svc data structure returned from init.
 *  @param branch_name : null terminated branch name.
 *  @return 0 if successful, error code if unsuccessful.
 */
int svc_checkout(void *helper, char *branch_name);

/** @brief Prints and returns array of all branch names.
 *
 *  If n_branches or helper is NULL, nothing is done and NULL is returned.
 *  Branches names are printed  to stdout in the order that they were created.
 *  Returned array is NOT released during cleanup. Branch names contained in
 *  returned arrays ARE released during cleanup.
 *
 *  @param helper : address of svc data structure returned from init.
 *  @param n_branches : address for length of returned array to be set.
 *  @return Address of array if successful, NULL if unsuccessful.
 */
char **list_branches(void *helper, int *n_branches);

/** @brief Stages file.
 *
 *  If file_name is NULL, nothing is done and -1 is returned. If file is already
 *  staged or tracked, nothing is done and -2 is returned. If file_name doesnt
 *  exist or permissions are invalid, nothing is done and -3 is returned. Hash
 *  is returned if successful.
 *
 *  @param helper : address of svc data structure returned from init.
 *  @param file_name : NULL terminated file name.
 *  @return Hash of file is successful, error code if unsuccessful.
 */
int svc_add(void *helper, char *file_name);

/** @brief Removes file from SVC.
 *
 *  If helper or file_name is NULL, nothing is done and -1 is returned. If
 *  file_name is not known to SVC, nothing is done and -2 is returned. If succ-
 *  essful, file is removed from SVC, and last known hash is returned.
 *
 *  FILE IS NOT DELETED FROM THE FILE SYSTEM, ONLY REMOVED FROM SVC.
 *
 *  @param helper : address of svc data structure returned from init.
 *  @param file_name : NULL terminated file name.
 *  @return Hash of file is successful, error code if unsuccessful.
 */
int svc_rm(void *helper, char *file_name);

/** @brief Resets to commit.
 *
 *  If helper or commit_id is NULL, nothing is done and -1 is returned. If commit
 *  is invalid (doesnt exist), nothing is done and -2 is returned. Otherwise,
 *  svc resets all files to the state captured by the commit.
 *
 *  @param helper : address of svc data structure returned from init.
 *  @param commit_id : NULL terminated commit id.
 *  @return 0 if successful, error code if unsuccessful.
 */
int svc_reset(void *helper, char *commit_id);

/** @brief Merges branch with current branch.
 *
 *  If helper or branch name are NULL, nothing is done and NULL is returned. If
 *  branch doesnt exist, Branch not found is printed to stdout, and NULL is
 *  returned. If branch is current branch, Cannot merge branch with itself is
 *  printed to stdout and NULL is returned. If uncommitted changes exist,
 *  Changes must be committed is printed to stdout and NULL is returned.
 *
 *  Merge will stage all files tracked by the merged branch, unknown to the
 *  current branch. File conflicts are resolved though the resolutions array.
 *  If file path in resolution is NULL, file is deleted from SVC.
 *
 *  Commit with message Merged branch [<branch name>] is made, and Merge
 *  successful is printed to stdout. The commit id is returned.
 *
 *  @param helper : address of svc data structure returned from init.
 *  @param branch_name : NULL terminated branch name.
 *  @param resolutions : array of resolutions for file conflicts.
 *  @param n_resolutions : length of resolutions array.
 *  @return Commit id if successful, NULL if unsuccessful.
 */
char *svc_merge(void *helper, char *branch_name, resolution *resolutions, int n_resolutions);

#endif
