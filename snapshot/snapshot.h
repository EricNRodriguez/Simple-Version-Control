#ifndef ASSIGNMENT_2_SVC_SNAPSHOT_H
#define ASSIGNMENT_2_SVC_SNAPSHOT_H

#include "../params.h"
#include "../memory/memory.h"
#include <stdio.h>
#include <unistd.h>

typedef struct FileSnapshot {
    char *name;  // original file name, including extension
    int hash;    // hash of the file
} FileSnapshot;

typedef struct Snapshot {
    FileSnapshot *file_snapshots;  // all file snapshots int the snapshot
    size_t n_files;                // number of files
    size_t file_snapshots_len;     // length of file snapshot
} Snapshot;

/** @brief Initialises new snapshot.
 *
 *  Initialises snapshot. Files is allocated with size set to INIT_SNAPSHOT_SIZE,
 *  specified in params.h. This MUST be released.
 *
 *  @return snapshot instance.
 */
Snapshot init_snapshot();

/** @brief Takes a new file snapshot.
 *
 *  If snapshot, name, file contents, or hash are NULL, nothing is done and -1
 *  is returned. Snapshot files array is reallocated if full. New file snapshot
 *  based on the provided parameters is created. A new file is only created if
 *  no file with the name <hash>.svc exists in the svc directory.
 *
 *  @param ss : snapshot instance.
 *  @param name : file path.
 *  @param hash : hash of file.
 *  @param file_contents : byte array of file contents.
 *  @param file_contents_len : length of file contents.
 *  @return 0 if successful, -1 otherwise.
 */
int new_file_snapshot(Snapshot *ss, char *name, int hash, char *file_contents,
                      size_t file_contents_len);

#endif //ASSIGNMENT_2_SVC_SNAPSHOT_H
