/**
 * @file bst.c
 * @brief Area Management implementation using Binary Search Tree
 */

#include "bst.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void bst_init(AreaBST *tree) {
    if (!tree) return;
    tree->root = NULL;
    tree->count = 0;
}

static AreaNode* create_node(int id, const char *name, double demand, int priority, const char *category) {
    AreaNode *node = (AreaNode*)malloc(sizeof(AreaNode));
    if (!node) return NULL;

    node->id = id;
    strncpy(node->name, name ? name : "Unnamed Area", sizeof(node->name) - 1);
    node->name[sizeof(node->name) - 1] = '\0';

    node->demand = demand > 0.0 ? demand : 0.0;
    node->supplied = 0.0;
    node->priority = (priority >= 1 && priority <= 3) ? priority : 3;

    strncpy(node->category, category ? category : "Residential", sizeof(node->category) - 1);
    node->category[sizeof(node->category) - 1] = '\0';

    node->left = NULL;
    node->right = NULL;
    return node;
}

static int insert_node(AreaNode **node, int id, const char *name, double demand, int priority, const char *category) {
    if (*node == NULL) {
        *node = create_node(id, name, demand, priority, category);
        if (*node == NULL) return -2; /* Allocation failure */
        return 0;
    }

    if (id == (*node)->id) {
        return -1; /* Duplicate ID rejected */
    } else if (id < (*node)->id) {
        return insert_node(&((*node)->left), id, name, demand, priority, category);
    } else {
        return insert_node(&((*node)->right), id, name, demand, priority, category);
    }
}

int bst_insert(AreaBST *tree, int id, const char *name, double demand, int priority, const char *category) {
    if (!tree) return -2;
    int res = insert_node(&(tree->root), id, name, demand, priority, category);
    if (res == 0) {
        tree->count++;
    }
    return res;
}

static AreaNode* search_node(AreaNode *node, int id) {
    if (!node) return NULL;
    if (node->id == id) return node;
    if (id < node->id) {
        return search_node(node->left, id);
    } else {
        return search_node(node->right, id);
    }
}

AreaNode* bst_search(const AreaBST *tree, int id) {
    if (!tree) return NULL;
    return search_node(tree->root, id);
}

int bst_update(AreaBST *tree, int id, double new_demand, int new_priority) {
    AreaNode *node = bst_search(tree, id);
    if (!node) return -1;

    if (new_demand >= 0.0) {
        node->demand = new_demand;
    }
    if (new_priority >= 1 && new_priority <= 3) {
        node->priority = new_priority;
    }
    return 0;
}

double bst_add_supply(AreaBST *tree, int id, double amount) {
    AreaNode *node = bst_search(tree, id);
    if (!node || amount <= 0.0) return 0.0;

    double needed = node->demand - node->supplied;
    if (needed <= 0.0) return 0.0; /* Demand already met */

    double to_add = (amount < needed) ? amount : needed;
    node->supplied += to_add;
    return to_add;
}

static void reset_supply_helper(AreaNode *node) {
    if (!node) return;
    node->supplied = 0.0;
    reset_supply_helper(node->left);
    reset_supply_helper(node->right);
}

void bst_reset_all_supplies(AreaBST *tree) {
    if (!tree) return;
    reset_supply_helper(tree->root);
}

static void inorder_helper(const AreaNode *node, AreaNode *out_array, int max_out, int *index) {
    if (!node || *index >= max_out) return;

    inorder_helper(node->left, out_array, max_out, index);

    if (*index < max_out) {
        out_array[*index] = *node;
        out_array[*index].left = NULL;
        out_array[*index].right = NULL;
        (*index)++;
    }

    inorder_helper(node->right, out_array, max_out, index);
}

int bst_inorder_to_array(const AreaBST *tree, AreaNode *out_array, int max_out) {
    if (!tree || !out_array || max_out <= 0) return 0;
    int index = 0;
    inorder_helper(tree->root, out_array, max_out, &index);
    return index;
}

int bst_get_count(const AreaBST *tree) {
    if (!tree) return 0;
    return tree->count;
}

static void totals_helper(const AreaNode *node, double *demand, double *supplied, double *unmet) {
    if (!node) return;
    *demand += node->demand;
    *supplied += node->supplied;
    if (node->demand > node->supplied) {
        *unmet += (node->demand - node->supplied);
    }
    totals_helper(node->left, demand, supplied, unmet);
    totals_helper(node->right, demand, supplied, unmet);
}

void bst_get_totals(const AreaBST *tree, double *out_total_demand, double *out_total_supplied, double *out_unmet) {
    double d = 0.0, s = 0.0, u = 0.0;
    if (tree && tree->root) {
        totals_helper(tree->root, &d, &s, &u);
    }
    if (out_total_demand) *out_total_demand = d;
    if (out_total_supplied) *out_total_supplied = s;
    if (out_unmet) *out_unmet = u;
}

static void free_helper(AreaNode *node) {
    if (!node) return;
    free_helper(node->left);
    free_helper(node->right);
    free(node);
}

void bst_free_all(AreaBST *tree) {
    if (!tree) return;
    free_helper(tree->root);
    tree->root = NULL;
    tree->count = 0;
}

void bst_load_defaults(AreaBST *tree) {
    bst_free_all(tree);
    bst_init(tree);

    /* Insert in a balanced order for balanced tree */
    bst_insert(tree, 104, "South Valley Residential", 95000.0, 3, "Residential");
    bst_insert(tree, 102, "North Residential Block", 140000.0, 3, "Residential");
    bst_insert(tree, 101, "City General Hospital", 180000.0, 1, "Hospital");
    bst_insert(tree, 103, "East Commercial Complex", 110000.0, 2, "Commercial");
    bst_insert(tree, 105, "University Campus & Clinics", 130000.0, 1, "Hospital");
    bst_insert(tree, 106, "Industrial Manufacturing Zone", 160000.0, 2, "Commercial");
}
