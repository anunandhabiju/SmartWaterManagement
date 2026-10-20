/**
 * @file bst.h
 * @brief Area Management using Binary Search Tree (Data Structure 4)
 *
 * Implements a dynamic Binary Search Tree (BST) keyed by unique Area ID.
 * Supports average O(log n) search and insertion, duplicate ID prevention,
 * and O(n) inorder traversal to retrieve areas in strictly ascending ID order.
 */

#ifndef BST_H
#define BST_H

typedef struct AreaNode {
    int id;                /* Unique Area ID (BST Search Key) */
    char name[64];
    double demand;         /* Water demand in litres */
    double supplied;       /* Water supplied in litres */
    int priority;          /* 1 = Essential/Hospital, 2 = Commercial, 3 = Residential */
    char category[32];     /* "Hospital", "Commercial", "Residential" */
    struct AreaNode *left;
    struct AreaNode *right;
} AreaNode;

typedef struct {
    AreaNode *root;
    int count;
} AreaBST;

/* Initializes an empty Area BST */
void bst_init(AreaBST *tree);

/* Inserts a new area into the BST.
 * Returns: 0 on success, -1 on duplicate ID, -2 on memory allocation failure. */
int bst_insert(AreaBST *tree, int id, const char *name, double demand, int priority, const char *category);

/* Searches for an area by ID in O(log n) avg time. Returns pointer or NULL. */
AreaNode* bst_search(const AreaBST *tree, int id);

/* Updates demand and priority for an area. Returns 0 on success, -1 if not found. */
int bst_update(AreaBST *tree, int id, double new_demand, int new_priority);

/* Allocates water to an area. Returns amount actually added. */
double bst_add_supply(AreaBST *tree, int id, double amount);

/* Resets supplied water for all areas to 0 */
void bst_reset_all_supplies(AreaBST *tree);

/* Populates array with all areas in ascending order of Area ID using Inorder Traversal.
 * Returns count of copied areas. */
int bst_inorder_to_array(const AreaBST *tree, AreaNode *out_array, int max_out);

/* Returns total registered areas count */
int bst_get_count(const AreaBST *tree);

/* Computes aggregate demand, supply, and unmet demand across all areas */
void bst_get_totals(const AreaBST *tree, double *out_total_demand, double *out_total_supplied, double *out_unmet);

/* Deallocates all tree nodes recursively */
void bst_free_all(AreaBST *tree);

/* Populates default demonstration areas */
void bst_load_defaults(AreaBST *tree);

#endif /* BST_H */
