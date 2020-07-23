#include "branch.h"

Branch init_master_branch() {
    Branch master_branch = {
            .name = (char *) safe_malloc(strlen(DEFAULT_BRANCH_NAME) +
                                         sizeof(char)),
            .files = (FileData *) safe_malloc(INIT_STAGING_SIZE *
                                               sizeof(FileData)),
            .n_files = 0,
            .files_len = INIT_STAGING_SIZE,
            .commit = NULL,
    };

    // set branch name
    strcpy(master_branch.name, DEFAULT_BRANCH_NAME); // master

    return master_branch;
}

void clean_branch_files(Branch *b) {
    if (!b) {
        return;
    }

    // replacement array without deleted files, tracks remaining files
    FileData *files_cleaned = safe_malloc(b->files_len * sizeof(FileData));
    size_t n_cleaned = 0;

    for (size_t i = 0; i < b->n_files; ++i) {
        if (b->files[i].state == Deleted) {
            // delete file, free name memory
            free(b->files[i].file_path);
        } else {
            files_cleaned[n_cleaned] = b->files[i];
            n_cleaned++;
        }
    }

    free(b->files);

    // update branch files to cleaned array
    b->files = files_cleaned;
    b->n_files = n_cleaned;
    return;
}

int is_valid_branch_name(char *name) {
    if (!name) {
        return 0;
    }

    // compile regex
    regex_t name_reg;
    if (regcomp(&name_reg, BRANCH_NAME_REGEX, REG_EXTENDED) != 0) {
        fprintf(stderr, "regex compile failed\n");
        return 0;
    }

    // check match
    int match = regexec(&name_reg, name, (size_t) 0, NULL, 0);

    // release regex
    regfree(&name_reg);

    return match == 0;
}

void free_branch(Branch b) {
    free(b.name);

    // free all file data (staged files)
    for (size_t j = 0; j < b.n_files; ++j) {
        free(b.files[j].file_path);
    }

    free(b.files);

    return;
}
