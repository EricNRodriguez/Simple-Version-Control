#ifndef ASSIGNMENT_2_SVC_BRANCH_H
#define ASSIGNMENT_2_SVC_BRANCH_H

#include "../params.h"
#include "../commit/commit.h"
#include "../file_data/file_data.h"
#include <stdio.h>
#include <string.h>
#include <regex.h>

typedef struct Branch {
    char *name;        // branch name
    Commit *commit;    // last commit
    FileData *files;   // files known to vc
    size_t n_files;    // number of files known to vc
    size_t files_len;  // files allocated length
} Branch;

/** @brief Creates master branch.
 *
 *  Creates master branch instance. Name and Files are initialised based
 *  on parameters in params.h. Commit is initialised to NULL.
 *
 *  @return Master branch.
 */
Branch init_master_branch();

/** @brief Updates branch files.
 *
 *  Removes all deleted files. Remaining files are set to Tracked state. If
 *  branch is NULL, nothing is done.
 *
 *  @param Address of branch.
 */
void clean_branch_files(Branch *b);

/** @brief Checks if name is valid.
 *
 *  Determines if the name is a valid branch name. Validity is determined
 *  though the regular expression BRANCH_NAME_REGEX in params.h.
 *
 *  If name is NULL, 0 is returned. If regex does not compile, error is printed
 *  to stderr, and 0 is returned. If name is valid, 1 is returned.
 *
 *  @param Null terminated name.
 *  @return 1 if valid name, 0 otherwise.
 */
int is_valid_branch_name(char *name);

/** @brief Releases unique branch memory.
 *
 *  Frees all allocated memory that is UNIQUE to the branch: Name and Files.
 *  Commit is NOT released!
 *
 *  @param Branch value.
 */
void free_branch(Branch b);

#endif //ASSIGNMENT_2_SVC_BRANCH_H
