#include "snapshot.h"

Snapshot init_snapshot() {
    Snapshot s = {
            .file_snapshots = (FileSnapshot *) safe_malloc(INIT_SNAPSHOT_SIZE *
                    sizeof(FileSnapshot)),
            .n_files = 0,
            .file_snapshots_len = INIT_SNAPSHOT_SIZE,
    };
    return s;
}

int new_file_snapshot(Snapshot *ss, char *name, int hash, char *file_contents,
                      size_t file_contents_len) {
    if (!ss || !name || !file_contents || !hash) {
        return -1;
    }

    // resize if necessary
    if (ss->n_files == ss->file_snapshots_len) {
        ss->file_snapshots = safe_realloc(ss->file_snapshots,
                                         ss->file_snapshots_len *
                                          ARRAY_GROWTH_RATE *
                                          sizeof(FileSnapshot));
        ss->file_snapshots_len *= ARRAY_GROWTH_RATE;
    }

    // new file snapshot
    FileSnapshot *fss = &ss->file_snapshots[ss->n_files];
    ss->n_files++;
    fss->name = copy_string(name);
    fss->hash = hash;

    // convert id to hex
    char file_name[HASH_HEX_SIZE];
    sprintf(file_name, SVC_FILE_PATH_FMT, hash);

    // check file access permission exists
    if (access(file_name, F_OK) == -1) {
        FILE *f = fopen(file_name, "w");
        if (!f) {
            perror("unable to create file during commit");
            exit(2);
        }
        // save file contents
        fwrite(file_contents, sizeof(char), file_contents_len, f);
        fclose(f);
    }

    return 0;
}