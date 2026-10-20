/**
 * @file linkedlist.c
 * @brief Maintenance Management implementation using Singly Linked List
 */

#include "linkedlist.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

static void get_current_time_str(char *buf, size_t sz) {
    time_t now = time(NULL);
    struct tm *tm_info = localtime(&now);
    if (tm_info) {
        strftime(buf, sz, "%Y-%m-%d %H:%M", tm_info);
    } else {
        snprintf(buf, sz, "2026-10-20 12:00");
    }
}

void list_init(MaintenanceList *list) {
    if (!list) return;
    list->head = NULL;
    list->count = 0;
    list->next_id = 501;
}

int list_insert(MaintenanceList *list, int complaint_id, const char *area_name,
                const char *issue, const char *status, const char *technician) {
    if (!list) return -1;

    MaintenanceNode *node = (MaintenanceNode*)malloc(sizeof(MaintenanceNode));
    if (!node) {
        return -1; /* Out of memory */
    }

    node->id = list->next_id++;
    node->complaint_id = complaint_id;

    strncpy(node->area_name, area_name ? area_name : "General Area", sizeof(node->area_name) - 1);
    node->area_name[sizeof(node->area_name) - 1] = '\0';

    strncpy(node->issue, issue ? issue : "Standard maintenance check", sizeof(node->issue) - 1);
    node->issue[sizeof(node->issue) - 1] = '\0';

    strncpy(node->status, status ? status : "Pending", sizeof(node->status) - 1);
    node->status[sizeof(node->status) - 1] = '\0';

    strncpy(node->technician, technician ? technician : "Unassigned", sizeof(node->technician) - 1);
    node->technician[sizeof(node->technician) - 1] = '\0';

    get_current_time_str(node->created_at, sizeof(node->created_at));
    node->resolved_at[0] = '\0';

    /* Insert at head for O(1) insertion */
    node->next = list->head;
    list->head = node;
    list->count++;

    return node->id;
}

MaintenanceNode* list_search(const MaintenanceList *list, int id) {
    if (!list) return NULL;
    MaintenanceNode *curr = list->head;
    while (curr) {
        if (curr->id == id) {
            return curr;
        }
        curr = curr->next;
    }
    return NULL;
}

int list_update_status(MaintenanceList *list, int id, const char *new_status, const char *technician) {
    MaintenanceNode *node = list_search(list, id);
    if (!node) return -1;

    if (new_status && strlen(new_status) > 0) {
        strncpy(node->status, new_status, sizeof(node->status) - 1);
        node->status[sizeof(node->status) - 1] = '\0';

        if (strcmp(new_status, "Resolved") == 0) {
            get_current_time_str(node->resolved_at, sizeof(node->resolved_at));
        }
    }

    if (technician && strlen(technician) > 0) {
        strncpy(node->technician, technician, sizeof(node->technician) - 1);
        node->technician[sizeof(node->technician) - 1] = '\0';
    }

    return 0;
}

int list_delete(MaintenanceList *list, int id) {
    if (!list || !list->head) return -1;

    MaintenanceNode *curr = list->head;
    MaintenanceNode *prev = NULL;

    while (curr && curr->id != id) {
        prev = curr;
        curr = curr->next;
    }

    if (!curr) return -1; /* Not found */

    if (!prev) {
        /* Deleting head node */
        list->head = curr->next;
    } else {
        prev->next = curr->next;
    }

    free(curr);
    list->count--;
    return 0;
}

int list_delete_resolved(MaintenanceList *list) {
    if (!list || !list->head) return 0;

    int deleted = 0;
    MaintenanceNode *curr = list->head;
    MaintenanceNode *prev = NULL;

    while (curr) {
        if (strcmp(curr->status, "Resolved") == 0) {
            MaintenanceNode *to_free = curr;
            if (!prev) {
                list->head = curr->next;
                curr = list->head;
            } else {
                prev->next = curr->next;
                curr = curr->next;
            }
            free(to_free);
            list->count--;
            deleted++;
        } else {
            prev = curr;
            curr = curr->next;
        }
    }

    return deleted;
}

int list_get_count(const MaintenanceList *list) {
    if (!list) return 0;
    return list->count;
}

int list_to_array(const MaintenanceList *list, MaintenanceNode *out_array, int max_out) {
    if (!list || !out_array || max_out <= 0) return 0;

    int count = 0;
    MaintenanceNode *curr = list->head;
    while (curr && count < max_out) {
        out_array[count] = *curr;
        out_array[count].next = NULL; /* Detach pointer copy */
        count++;
        curr = curr->next;
    }
    return count;
}

void list_free_all(MaintenanceList *list) {
    if (!list) return;

    MaintenanceNode *curr = list->head;
    while (curr) {
        MaintenanceNode *next = curr->next;
        free(curr);
        curr = next;
    }

    list->head = NULL;
    list->count = 0;
}

void list_load_defaults(MaintenanceList *list) {
    list_free_all(list);
    list_init(list);

    list_insert(list, 102, "Industrial Park East", "Main pipeline joint valve seal leak repair", "In Progress", "Eng. Alex Rivera");
    list_insert(list, 101, "North Sector Residential", "Pressure booster pump recalibration in Substation 3", "Pending", "Eng. Maya Chen");
    list_insert(list, 0, "Central Basin Tank", "Routine ultrasonic sediment & flow meter inspection", "Resolved", "Eng. David Miller");
}
