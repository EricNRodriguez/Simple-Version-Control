#include "svc.h"

typedef struct VersionControl {
    Branch *branches;        // all branches, including deleted
    size_t n_branches;       // number of branches
    size_t len_branches;     // capacity of branches
    size_t current_branch;   // index of current branch in branches
    Commit **commits;        // all commits known to vc
    size_t n_commits;        // number of current commits (total)
    size_t len_commits;      // total allocated size of commits array
} VersionControl;

/** @brief Checks if there exist uncommitted changes.
 *
 *  If uncommitted changes exist, 1 is returned. Otherwise, 0 is
 *  returned. Uncommitted changes involve staged files that are not
 *  tracked, and tracked files that have diverged from their last known
 *  state, recorded in the snapshot of the previous commit.
 *
 *  @param vc : Version control instance address.
 *  @return 0 if no uncommitted changes exist, 1 otherwise.
 */
static int check_uncommitted_changes(VersionControl *vc);

/** @brief Restores tracked files to state recorded in snapshot.
 *
 *  If parameters are NULL, nothing is done. Every file recorded by the
 *  provided snapshot is restored to the state recorded by the snapshot.
 *
 *  @param vc : Version control instance address.
 *  @param ss : Snapshot instance address.
 */
static void restore_snapshot(VersionControl *vc, Snapshot *ss);

/** @brief Finds index of branch by name.
 *
 *  Finds the index of the branch_name, in vc array. If vc or branch name
 *  are NULL, nothing is done and -1 is returned. If the branch with branch_name
 *  is found, its index is returned. Otherwise, -1 is returned.
 *
 *  @param vc : Version control instance address.
 *  @param branch_name : Null terminated string.
 */
static int get_branch_index(VersionControl *vc, char *branch_name);

void *svc_init(void) {
    VersionControl *vc = (VersionControl *) safe_malloc(sizeof(VersionControl));

    // init svc directory
    if (mkdir(SVC_DIR_PATH, S_IRWXU) == -1) {
        perror("unable to make svc directory");
        return NULL;
    }

    // init branches array
    vc->branches = (Branch *) safe_malloc(INIT_BRANCHES_SIZE * sizeof(Branch));
    vc->len_branches = INIT_BRANCHES_SIZE;
    vc->n_branches = 0;

    // init commits array
    vc->commits = (Commit **) safe_malloc(INIT_COMMIT_SIZE * sizeof(Commit *));
    vc->len_commits = INIT_COMMIT_SIZE;
    vc->n_commits = 0;

    // init master branch
    vc->branches[MASTER_BRANCH_INDEX] = init_master_branch();
    vc->current_branch = MASTER_BRANCH_INDEX;
    vc->n_branches++;
    return vc;
}

void cleanup(void *helper) {
    if (!helper) {
        return;
    }

    VersionControl *vc = (VersionControl *) helper;

    // free commit and associated snapshot memory
    for (size_t i = 0; i < vc->n_commits; ++i) {
        free_commit(vc->commits[i]);
    }
    free(vc->commits);

    // clean branches
    for (size_t i = 0; i < vc->n_branches; ++i) {
        free_branch(vc->branches[i]);
    }
    free(vc->branches);

    // delete snapshot files and .svc dir
    remove_svc_directory();

    free(vc);
    return;
}

int hash_file(void *helper, char *file_path) {
    if (!file_path) {
        return -1;
    }

    // calc hash, dont copy file
    errno = 0;
    int hash = hash_and_copy_file(file_path, NULL, NULL);
    if (hash == -1) {
        switch errno {
            case ENOENT:
                return -2;
                break;
            case EFAULT:
                return -1;
                break;
            default:
                fprintf(stderr,"fopen error: %s\n", strerror(errno));
                return -3;
        }
    }

    return hash;
}

char *svc_commit(void *helper, char *message) {
    if (!helper || !message) {
        return NULL;
    }

    VersionControl *vc = (VersionControl *) helper;

    // resize if necessary
    if (vc->n_commits == vc->len_commits) {
        vc->commits = safe_realloc(vc->commits, vc->len_commits *
                                   ARRAY_GROWTH_RATE * sizeof(Commit *));
        vc->len_commits *= ARRAY_GROWTH_RATE;
    }

    // initialise new commit
    Commit *new_commit =
            init_commit(message, vc->branches[vc->current_branch].n_files,
                        vc->current_branch);
    vc->commits[vc->n_commits] = new_commit;
    vc->n_commits++;

    // check current known files, commit changes
    FileData *fd = NULL;
    for (size_t i = 0; i < vc->branches[vc->current_branch].n_files; ++i) {
        fd = &vc->branches[vc->current_branch].files[i];
        if (fd->state == Deleted) {
            commit_deleted_file(new_commit, fd->file_path);
        } else if (fd->state == Staged) {
            if (access(fd->file_path, F_OK) == -1) {
                fd->state = Deleted;
            } else {
                // commit change and upgrade status
                fd->state = Tracked;
                fd->previous_hash = commit_staged_file(new_commit, fd->file_path);
            }
        } else {
            if (access(fd->file_path, F_OK) == -1) {
                commit_deleted_file(new_commit, fd->file_path);
                fd->state = Deleted;
            } else {
                // update hash
                fd->previous_hash = commit_tracked_file(new_commit, fd->file_path,
                                                        fd->previous_hash);
            }
        }
    }

    // no changes, undo commit
    if (new_commit->n_record == 0) {
        free_commit(new_commit);
        vc->n_commits--;
        return NULL;
    }

    char *commit_id = generate_commit_id(new_commit);
    new_commit->id = commit_id;

    // edge from new commit to prev commit if exists
    if (vc->branches[vc->current_branch].commit) {
        new_commit->parent_commits = safe_malloc(sizeof(Commit *));
        new_commit->parent_commits[0] = vc->branches[vc->current_branch].commit;
        new_commit->n_parent_commits = 1;
    }

    // advance branch to latest commit
    vc->branches[vc->current_branch].commit = new_commit;
    // remove deleted files, track remaining
    clean_branch_files(&vc->branches[vc->current_branch]);
    return commit_id;
}

void *get_commit(void *helper, char *commit_id) {
    if (!helper || !commit_id) {
        return NULL;
    }

    VersionControl *vc = (VersionControl *) helper;

    // linear search through commit array in order of creation
    for (size_t i = vc->n_commits - 1; i >= 0 && i < vc->n_commits; --i) {
        if (!strcmp(vc->commits[i]->id, commit_id)) {
            return vc->commits[i];
        }
    }

    return NULL;
}

char **get_prev_commits(void *helper, void *commit, int *n_prev) {
    if (!helper || !n_prev) {
        return NULL;
    }

    if (!commit) {
        *n_prev = 0;
        return NULL;
    }

    Commit *c = (Commit *) commit;

    *n_prev = c->n_parent_commits;
    if (!c->n_parent_commits) {
        return NULL;
    }

    // store adjacent commit ids
    char **adjacent_commit_ids = safe_malloc(c->n_parent_commits *
                                              sizeof(char *));
    for (size_t i = 0; i < c->n_parent_commits; ++i) {
        adjacent_commit_ids[i] = c->parent_commits[i]->id;
    }

    return adjacent_commit_ids;
}

void print_commit(void *helper, char *commit_id) {
    if (!helper) {
        return;
    }

    if (!commit_id) {
        printf("Invalid commit id\n");
        return;
    }

    VersionControl *vc = (VersionControl *) helper;

    // find commit by id
    Commit *selected_commit = get_commit(vc, commit_id);

    // NULL, no commit found
    if (!selected_commit) {
        printf("Invalid commit id\n");
        return;
    }

    // sort commit by state
    qsort(selected_commit->commit_record, selected_commit->n_record,
            sizeof(CommitRecord), compare_commit_record_change);

    // print commit info
    printf("%s [%s]: %s\n", commit_id,
            vc->branches[selected_commit->branch_id].name,
            selected_commit->message);

    // print changed/staged/deleted files
    for (size_t i = 0; i < selected_commit->n_record; ++i) {
        switch (selected_commit->commit_record[i].change_type) {
            case Add:
                printf("    + %s\n",
                       selected_commit->commit_record[i].file_name);
                break;
            case Remove:
                printf("    - %s\n",
                       selected_commit->commit_record[i].file_name);
                break;
            case Change:
                printf("    / %s [%10d -> %10d]\n",
                       selected_commit->commit_record[i].file_name,
                       selected_commit->commit_record[i].hash_change.old_hash,
                       selected_commit->commit_record[i].hash_change.new_hash);
                break;
        }
    }
    printf("\n");

    // print tracked files and data
    Snapshot *ss = &selected_commit->snapshot;
    printf("    Tracked files (%zu):\n", ss->n_files);
    for (size_t i = 0; i < ss->n_files; ++i) {
        printf("    [%10d] %s\n", ss->file_snapshots[i].hash,
                ss->file_snapshots[i].name);
    }

    return;
}

static int get_branch_index(VersionControl *vc, char *branch_name) {
    if (!vc || !branch_name) {
        return -1;
    }

    // linear search in order of creation
    for (size_t i = vc->n_branches - 1; i >= 0 && i < vc->n_branches; --i) {
        if (!strcmp(vc->branches[i].name, branch_name)) {
            return i;
        }
    }

    // no match
    return -1;
}

static int check_uncommitted_changes(VersionControl *vc) {
    if (!vc) {
        return 0;
    }

    Commit *last_commit = vc->branches[vc->current_branch].commit;
    if (!last_commit) {
        return 0;
    }

    // check for changes
    for (size_t i = 0; i < vc->branches[vc->current_branch].n_files; ++i) {
        if (vc->branches[vc->current_branch].files[i].state == Staged) {
            return 1;
        } else if (vc->branches[vc->current_branch].files[i].state == Tracked) {
            // compare current file hash with last known hash
            if (hash_file(vc, vc->branches[vc->current_branch].files[i].file_path)
                != vc->branches[vc->current_branch].files[i].previous_hash) {
                return 1;
            }
        }
    }

    return 0;
}

int svc_branch(void *helper, char *branch_name) {
    if (!helper || !branch_name || !is_valid_branch_name(branch_name)) {
        return -1;
    }

    VersionControl *vc = (VersionControl *) helper;

    if (get_branch_index(vc, branch_name) != -1) {
        return -2;
    }

    if (check_uncommitted_changes(vc)) {
        return -3;
    }

    // check if resize
    if (vc->len_branches == vc->n_branches) {
        vc->branches = safe_realloc(vc->branches, vc->n_branches *
                                    ARRAY_GROWTH_RATE * sizeof(Branch));
        vc->len_branches++;
    }

    // copy previous branch
    vc->branches[vc->n_branches] = vc->branches[vc->current_branch];
    vc->branches[vc->n_branches].name = copy_string(branch_name);
    vc->branches[vc->n_branches].files =
            copy_file_data(vc->branches[vc->current_branch].files,
                           vc->branches[vc->current_branch].n_files,
                           vc->branches[vc->current_branch].files_len);
    vc->n_branches++;

    return 0;
}

static void restore_snapshot(VersionControl *vc, Snapshot *ss) {
    if (!vc || !ss) {
        return;
    }

    // update tracked file contents to snapshot content
    for (size_t i = 0; i < ss->n_files; ++i) {
        char snapshot_f_name[HASH_HEX_SIZE];
        sprintf(snapshot_f_name, SVC_FILE_PATH_FMT, ss->file_snapshots[i].hash);
        update_file(ss->file_snapshots[i].name, snapshot_f_name);
    }

    return;
}

int svc_checkout(void *helper, char *branch_name) {
    if (!helper || !branch_name) {
        return -1;
    }

    VersionControl *vc = (VersionControl *) helper;

    size_t branch_index = get_branch_index(vc, branch_name);
    if (branch_index == -1) {
        return -1;
    }

    if (check_uncommitted_changes(vc)) {
        return -2;
    }
    // switch to current branch
    vc->current_branch = branch_index;

    // last snapshot taken on the new branch is coppied
    if (vc->branches[vc->current_branch].commit) {
        // restore files to last commit on new branch
        restore_snapshot(vc, &vc->branches[vc->current_branch].commit->snapshot);
    }

    return 0;
}

char **list_branches(void *helper, int *n_branches) {
    if (!helper || !n_branches) {
        return NULL;
    }

    VersionControl *vc = (VersionControl *) helper;

    // record branch name and print it to stdout
    char **branch_names = safe_malloc(vc->n_branches * sizeof(char *));
    for (size_t i = 0; i < vc->n_branches; ++i) {
        branch_names[i] = vc->branches[i].name;
        printf("%s\n", branch_names[i]);
    }
    *n_branches = vc->n_branches;

    return branch_names;
}

int svc_add(void *helper, char *file_name) {
    if (!file_name || !helper) {
        return -1;
    }

    if (access(file_name, F_OK ) == -1) {
        return -3;
    }

    VersionControl *vc = (VersionControl *) helper;
    Branch *cur_branch = &vc->branches[vc->current_branch];

    // branch name is unknown
    if (!is_unknown(cur_branch->files, cur_branch->n_files, file_name)) {
        return -2;
    }

    // cpy file name
    char *file_name_cpy = copy_string(file_name);

    // calc hash of file
    int hash = hash_file(NULL, file_name);

    // add space if required
    if (cur_branch->n_files == cur_branch->files_len) {
        cur_branch->files = (FileData *) safe_realloc(cur_branch->files,
                                                      cur_branch->files_len *
                                                      ARRAY_GROWTH_RATE *
                                                      sizeof(FileData));
        cur_branch->files_len *= ARRAY_GROWTH_RATE;
    }

    // stage file
    cur_branch->files[cur_branch->n_files].state = Staged;
    cur_branch->files[cur_branch->n_files].file_path = file_name_cpy;
    cur_branch->files[cur_branch->n_files].previous_hash = hash;
    cur_branch->n_files++;

    return hash;
}

int svc_rm(void *helper, char *file_name) {
    if (!helper || !file_name) {
        return -1;
    }

    VersionControl *vc = (VersionControl *) helper;
    Branch *curr_branch = &vc->branches[vc->current_branch];

    // remove from vc
    for (size_t i = 0; i < curr_branch->n_files; ++i) {
        if (!strcmp(curr_branch->files[i].file_path, file_name) &&
            curr_branch->files[i].state == Staged) {
            // removed staged file
            free(curr_branch->files[i].file_path);
            int prev_hash = curr_branch->files[i].previous_hash;
            memmove(curr_branch->files + i, curr_branch->files + (i + 1),
                    (curr_branch->n_files - i - 1) * sizeof(FileData));
            curr_branch->n_files--;
            return prev_hash;
        } else if (!strcmp(curr_branch->files[i].file_path, file_name) &&
                    curr_branch->files[i].state != Deleted) {
            // remove file if state isnt deleted
            curr_branch->files[i].state = Deleted;
            return curr_branch->files[i].previous_hash;
        }
    }

    // no file exists in vc
    return -2;
}

int svc_reset(void *helper, char *commit_id) {
    if (!helper || !commit_id) {
        return -1;
    }

    VersionControl *vc = (VersionControl *) helper;

    Commit *c = (Commit *) get_commit(vc, commit_id);
    if (!c) {
        return -2;
    }

    // reset branch to commit
    vc->branches[vc->current_branch].commit = c;
    c->branch_id = vc->current_branch;

    // free prev files
    for (size_t i = 0; i < vc->branches[vc->current_branch].n_files; ++i) {
        free(vc->branches[vc->current_branch].files[i].file_path);
    }
    free(vc->branches[vc->current_branch].files);

    // generate new file data from snapshot
    vc->branches[vc->current_branch].n_files = c->snapshot.n_files;
    vc->branches[vc->current_branch].files_len = c->snapshot.n_files;
    vc->branches[vc->current_branch].files =
            safe_malloc(vc->branches[vc->current_branch].files_len *
                         sizeof(FileData));

    // track restored files
    for (size_t i = 0; i < vc->branches[vc->current_branch].n_files; ++i) {
        // copy file name
        vc->branches[vc->current_branch].files[i].file_path =
                copy_string(c->snapshot.file_snapshots[i].name);
        // set all files to tracked
        vc->branches[vc->current_branch].files[i].state = Tracked;
        vc->branches[vc->current_branch].files[i].previous_hash =
                c->snapshot.file_snapshots[i].hash;
    }

    // restore snapshot
    restore_snapshot(vc, &c->snapshot);
    return 0;
}

char *svc_merge(void *helper, char *branch_name, struct resolution *resolutions,
        int n_resolutions) {
    if (!helper) {
        return NULL;
    }

    if (!branch_name) {
        printf("Invalid branch name\n");
        return NULL;
    }

    VersionControl *vc = (VersionControl *) helper;

    // index in vc branch array
    int branch_index = get_branch_index(vc, branch_name);

    if (branch_index < 0) {
        printf("Branch not found\n");
        return NULL;
    }

    if (branch_index == vc->current_branch) {
        printf("Cannot merge a branch with itself\n");
        return NULL;
    }

    if (check_uncommitted_changes(vc)) {
        printf("Changes must be committed\n");
        return NULL;
    }

    // last snapshot of the branch being merged
    Snapshot *merge_snapshot = &vc->branches[branch_index].commit->snapshot;
    for (size_t i = 0; i < merge_snapshot->n_files; ++i) {
        // if unknown to vc, stage file
        if (is_unknown(vc->branches[vc->current_branch].files,
                       vc->branches[vc->current_branch].n_files,
                       merge_snapshot->file_snapshots[i].name)) {
            if (access(merge_snapshot->file_snapshots[i].name, F_OK) == -1) {
                char snapshot_file_name[HASH_HEX_SIZE];
                // format file path to svc directory
                sprintf(snapshot_file_name, SVC_FILE_PATH_FMT,
                        merge_snapshot->file_snapshots[i].hash);
                // update file to old contents from snapshot
                update_file(merge_snapshot->file_snapshots[i].name,
                            snapshot_file_name);
            }
            svc_add(vc, merge_snapshot->file_snapshots[i].name);
        }
    }

    // resolve conflicts
    for (size_t i = 0; i < n_resolutions; ++i) {
        // update file to resolutions
        if (access(resolutions[i].resolved_file, F_OK) == -1) {
            remove(resolutions[i].file_name);
        } else {
            update_file(resolutions[i].file_name, resolutions[i].resolved_file);
        }
    }

    // print merge msg to stdout
    char *commit_msg = safe_malloc(MAX_BRANCH_NAME_LEN * sizeof(char));
    sprintf(commit_msg, "Merged branch %s", branch_name);
    char *commit_id = svc_commit(vc, commit_msg);

    // add as parent commit if non-null
    if (commit_id) {
        // child commit
        Commit *c = vc->commits[vc->n_commits - 1];
        // allocate memory for second parent
        c->parent_commits =
                safe_realloc(vc->commits[vc->n_commits - 1]->parent_commits,
                             (vc->commits[vc->n_commits - 1]->n_parent_commits + 1) *
                             sizeof(Commit *));
        // add branch commit as parent of child
        c->parent_commits[c->n_parent_commits] = vc->branches[branch_index].commit;
        vc->commits[vc->n_commits - 1]->n_parent_commits++;
    }

    free(commit_msg);

    printf("Merge successful\n");

    return commit_id;
}
