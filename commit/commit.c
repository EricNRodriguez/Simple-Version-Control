#include "commit.h"

Commit *init_commit(char *message, size_t commit_record_len, size_t branch_id) {
    if (!message) {
        return NULL;
    }

    Commit *commit = (Commit *) safe_malloc(sizeof(Commit));
    commit->id = NULL;
    commit->message = copy_string(message);
    commit->branch_id = branch_id;
    commit->commit_record = safe_malloc(commit_record_len * sizeof(CommitRecord));
    commit->record_len = commit_record_len;
    commit->n_record = 0;
    commit->parent_commits = NULL;
    commit->n_parent_commits = 0;
    commit->snapshot = init_snapshot();

    return commit;
}

char *generate_commit_id(Commit *commit) {
    if (!commit || !commit->message) {
        return NULL;
    }

    // sort commit record by filename
    qsort(commit->commit_record, commit->n_record, sizeof(CommitRecord),
            compare_commit_record_name);

    long long id = 0;
    for (size_t i = 0; i < strlen(commit->message); ++i) {
        id = (id + commit->message[i]) % 1000;
    }
    for (size_t i = 0; i < commit->n_record; ++i) {
        if (commit->commit_record[i].change_type == Add) {
            id = id + 376591;
        } else if (commit->commit_record[i].change_type == Remove) {
            id = id + 85973;
        } else {
            id = id + 9573681;
        }
        for (size_t j = 0; j < strlen(commit->commit_record[i].file_name); ++j) {
            id = ((id * (commit->commit_record[i].file_name[j] % 37)) %
                    15485863) + 1;
        }
    }

    // char array, max size
    char *id_hex = safe_malloc((16) * sizeof(char));
    snprintf(id_hex, 16, "%06llx", id);

    return id_hex;
}

int compare_commit_record_change(const void *a, const void *b) {
    return ((CommitRecord *)a)->change_type > ((CommitRecord *)b)->change_type;
}

int commit_deleted_file(Commit *commit, char *file_path) {
    if (!commit || !file_path) {
        return -1;
    }
    // resize
    resize_commit_record(commit);
    commit->commit_record[commit->n_record].file_name = copy_string(file_path);
    commit->commit_record[commit->n_record].change_type = Remove;
    commit->n_record++;
    return 0;
}

int commit_staged_file(Commit *commit, char *file_path) {
    if (!commit || !file_path) {
        return -1;
    }

    // hash and copy file contents
    char *file_cpy = NULL;
    size_t file_cpy_len = 0;
    int hash = hash_and_copy_file(file_path, &file_cpy, &file_cpy_len);

    // resize
    resize_commit_record(commit);

    // initialise commit record
    commit->commit_record[commit->n_record].file_name = copy_string(file_path);
    commit->commit_record[commit->n_record].change_type = Add;
    commit->commit_record[commit->n_record].hash_change.new_hash = hash;
    commit->n_record++;

    // create snapshot of files
    new_file_snapshot(&commit->snapshot, file_path, hash, file_cpy, file_cpy_len);

    // release file copy
    free(file_cpy);

    return hash;
}


int commit_tracked_file(Commit *commit, char *file_path, int old_hash) {
    if (!commit || !file_path) {
        return -1;
    }

    // hash file contents and copy into array
    char *file_cpy = NULL;
    size_t file_cpy_len = 0;
    int hash = hash_and_copy_file(file_path, &file_cpy, &file_cpy_len);

    if (hash != old_hash) {
        // resize
        resize_commit_record(commit);
        // record change
        commit->commit_record[commit->n_record].file_name = copy_string(file_path);
        commit->commit_record[commit->n_record].change_type = Change;
        commit->commit_record[commit->n_record].hash_change.old_hash = old_hash;
        commit->commit_record[commit->n_record].hash_change.new_hash = hash;
        commit->n_record++;
    }

    new_file_snapshot(&commit->snapshot, file_path, hash, file_cpy, file_cpy_len);
    free(file_cpy);
    return hash;
}

int compare_commit_record_name(const void *a, const void *b) {
    // compare with case insensitivity
    int cmp = strcasecmp(((CommitRecord *) a)->file_name ,
                         ((CommitRecord *) b)->file_name);

    // compare with case sensitivity if no difference exists in chars
    if (!cmp) {
        cmp = strcmp(((CommitRecord *) a)->file_name ,
                     ((CommitRecord *) b)->file_name);
    }

    return cmp > 0;
}

void resize_commit_record(Commit *commit) {
    // resize only if necessary
    if (commit->n_record == commit->record_len) {
        commit->commit_record = safe_realloc(commit->commit_record,
                                        commit->record_len *
                                              ARRAY_GROWTH_RATE *
                                              sizeof(char *));
        commit->record_len *= ARRAY_GROWTH_RATE;
    }

    return;
}

void free_commit(Commit *c) {
    if (!c) {
        return;
    }

    free(c->id);
    free(c->message);
    free(c->parent_commits);

    // free commit records
    for (size_t j = 0; j < c->n_record; ++j) {
        free(c->commit_record[j].file_name);
    }
    free(c->commit_record);

    // free snapshot
    for (size_t j = 0; j < c->snapshot.n_files; ++j) {
        free(c->snapshot.file_snapshots[j].name);
    }
    free(c->snapshot.file_snapshots);
    free(c);

    return;
}