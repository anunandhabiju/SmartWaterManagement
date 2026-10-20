/**
 * @file linkedlist.h
 * @brief Maintenance Management using Singly Linked List (Data Structure 3)
 *
 * Implements a dynamic singly linked list in C with malloc/free for work orders,
 * supporting insertion, search by maintenance ID, status updates, resolved node
 * deletion, and traversal.
 */

#ifndef LINKEDLIST_H
#define LINKEDLIST_H

typedef struct MaintenanceNode {
    int id;
    int complaint_id;       /* Associated complaint ID (or 0 if preventive maintenance) */
    char area_name[64];
    char issue[256];
    char status[32];        /* "Pending", "In Progress", "Resolved" */
    char technician[64];
    char created_at[32];
    char resolved_at[32];
    struct MaintenanceNode *next;
} MaintenanceNode;

typedef struct {
    MaintenanceNode *head;
    int count;
    int next_id;
} MaintenanceList;

/* Initializes an empty maintenance linked list */
void list_init(MaintenanceList *list);

/* Inserts a new maintenance record at the head (or tail). Returns assigned ID. */
int list_insert(MaintenanceList *list, int complaint_id, const char *area_name,
                const char *issue, const char *status, const char *technician);

/* Searches for a maintenance record by ID. Returns pointer or NULL. */
MaintenanceNode* list_search(const MaintenanceList *list, int id);

/* Updates status and optionally records resolution timestamp. Returns 0 on success, -1 if not found. */
int list_update_status(MaintenanceList *list, int id, const char *new_status, const char *technician);

/* Deletes a specific record by ID. Frees memory. Returns 0 on success, -1 if not found. */
int list_delete(MaintenanceList *list, int id);

/* Deletes ALL resolved records from the list. Returns number of deleted nodes. */
int list_delete_resolved(MaintenanceList *list);

/* Returns current number of nodes */
int list_get_count(const MaintenanceList *list);

/* Populates array with shallow copies of records for display/serialization. Returns count copied. */
int list_to_array(const MaintenanceList *list, MaintenanceNode *out_array, int max_out);

/* Frees all dynamically allocated nodes in list */
void list_free_all(MaintenanceList *list);

/* Loads sample default maintenance records */
void list_load_defaults(MaintenanceList *list);

#endif /* LINKEDLIST_H */
