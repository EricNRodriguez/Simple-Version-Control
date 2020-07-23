#ifndef ASSIGNMENT_2_SVC_COMMIT_H
#define ASSIGNMENT_2_SVC_COMMIT_H

#include "../snapshot/snapshot.h"
#include "../file_data/file_data.h"
#include <stdio.h>
#include <string.h>

enum CommitChangeType {Add = 0, Remove = 1, Change = 2};

typedef struct HashChange {
    int old_hash; // snapshot of previous version
    int new_hash; // snapshot of revision
} HashChange;

typedef struct CommitRecord {
    char *file_name;                   // file name, null terminated
    enum CommitChangeType change_type; // type of change committed
    HashChange hash_change;            // only used for changed files
} CommitRecord;

typedef struct Commit {
    char *id;                        // null terminated commit id (hex)
    size_t branch_id;                // id of branch
    char *message;                   // null terminated message
    CommitRecord *commit_record;     // record of all file changes committed
    size_t n_record;                 // number of commit records
    size_t record_len;               // allocated length of commit_record
    struct Commit **parent_commits;  // address of parent commits
    size_t n_parent_commits;         // number of parent commits
    Snapshot snapshot;               // snapshot of current state of tracked files
} Commit;

/** @brief Initialises commit instance.
 *
 *  Initialises commit instance. Parameters located in params.h. id and
 *  parent_commit fields initialised to NULL. message is copied. Allocates
 *  commit_record_len sized commit record. Allocates snapshot of size to
 *  INIT_SNAPSHOT_SIZE. All number fields set to 0.
 *
 *  @param message : null terminated commit message.
 *  @param commit_record_len : initialised size of commit_record.
 *  @param branch_id : id of branch commit is associated with.
 *  @return Commit address.
 */
Commit *init_commit(char *message, size_t commit_record_len, size_t branch_id);

/** @brief Generates commit hex id.
 *
 *  If commit or message are NULL, NULL is returned. Id is calculated and
 *  returned as 6 or greater character null terminated hex string, allocated
 *  dynamically.
 *
 *  @param commit : commit address.
 *  @return Null terminated commit id (hex) string.
 */
char *generate_commit_id(Commit *commit);

/** @brief Compares commit record by state.
 *
 *  Compares commit records when sorting by type.
 *
 *  @param a : address of commit record.
 *  @param b : address of commit record.
 *  @return 1 if a type > b type, 0 otherwise.
 */
int compare_commit_record_change(const void *a, const void *b);

/** @brief Compares commit record by name.
 *
 *  Comparision of commit records ny name (alphabetical) for sorting by name.
 *
 *  @param a : address of commit record.
 *  @param b : address of commit record.
 *  @return 1 a and b are out of order (alphabetical), 0 otherwise.
 */
int compare_commit_record_name(const void *a, const void *b);

/** @brief Commits deleted file.
 *
 *  If commit or file_path are NULL, nothing is done. Commit record field
 *  is re-allocated if necessary. New record with name file_path and change
 *  type Remove is added to next available index. file_path is copied, and must
 *  be released. n_records is incremented.
 *
 *  @param commit : address of commit instance
 *  @param file_path : Null terminated file path
 *  @return 0 if successful, -1 otherwise.
 */
int commit_deleted_file(Commit *commit, char *file_path);

/** @brief Commits staged file.
 *
 *  If commit or file path are NULL, nothing is done and -1 is returned. Commit
 *  record field is resized if necessary. New record with file_path is added to
 *  next available index, with change set to Add. Snapshot of file is taken.
 *
 *  @param commit : address of commit instance
 *  @param file_path : Null terminated file path
 *  @return hash of file if successful, -1 otherwise.
 */
int commit_staged_file(Commit *commit, char *file_path);

/** @brief Commits tracked file.
 *
 *  If commit or file path are NULL, nothing is done and -1 is returned. Commit
 *  record field is resized if necessary. Hash is recalculated, and commit record
 *  with Change type is created if different to last known hash. Snapshot is taken
 *  regardless.
 *
 *  @param commit : address of commit instance
 *  @param file_path : Null terminated file path
 *  @return hash of file if successful, -1 otherwise.
 */
int commit_tracked_file(Commit *commit, char *file_path, int old_hash);

/** @brief Resizes commit record if full.
 *
 *  If commit record is full, size is increased by factor ARRAY_GROWTH_RATE,
 *  specified in params.h.
 *
 *  @param commit : address of commit instance
 */
void resize_commit_record(Commit *commit);

/** @brief Releases memory associated with commit.
 *
 *  Frees all memory associated with commit, and commit itself.
 *
 *  @param commit : address of commit instance
 */
void free_commit(Commit *c);

#endif //ASSIGNMENT_2_SVC_COMMIT_H
