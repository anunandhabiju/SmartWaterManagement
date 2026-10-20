/**
 * @file distribution.c
 * @brief Water Distribution and Allocation Engine Implementation
 */

#include "distribution.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

static void get_timestamp(char *buf, size_t sz) {
    time_t now = time(NULL);
    struct tm *tm_info = localtime(&now);
    if (tm_info) {
        strftime(buf, sz, "%Y-%m-%d %H:%M:%S", tm_info);
    } else {
        snprintf(buf, sz, "2026-10-20 12:00:00");
    }
}

void dist_init(DistributionManager *dm) {
    if (!dm) return;
    dm->count = 0;
    dm->total_distributed_litres = 0.0;
    dm->total_leakage_loss_litres = 0.0;
    memset(dm->records, 0, sizeof(dm->records));
}

static void record_distribution(DistributionManager *dm, int tank_id, const char *tank_name,
                                int area_id, const char *area_name, double amount,
                                const char *mode, const char *status) {
    if (!dm) return;

    if (dm->count >= MAX_DISTRIBUTION_HISTORY) {
        /* Shift array to maintain most recent history */
        for (int i = 0; i < MAX_DISTRIBUTION_HISTORY - 1; i++) {
            dm->records[i] = dm->records[i + 1];
        }
        dm->count = MAX_DISTRIBUTION_HISTORY - 1;
    }

    DistributionRecord *rec = &dm->records[dm->count++];
    rec->id = dm->count;
    rec->source_tank_id = tank_id;
    strncpy(rec->source_tank_name, tank_name ? tank_name : "Tank", sizeof(rec->source_tank_name) - 1);
    rec->dest_area_id = area_id;
    strncpy(rec->dest_area_name, area_name ? area_name : "Area", sizeof(rec->dest_area_name) - 1);
    rec->amount_litres = amount;
    strncpy(rec->mode, mode ? mode : "Manual", sizeof(rec->mode) - 1);
    strncpy(rec->status, status ? status : "Success", sizeof(rec->status) - 1);
    get_timestamp(rec->timestamp, sizeof(rec->timestamp));

    if (strcmp(status, "Success") == 0 || strcmp(status, "Partial") == 0) {
        dm->total_distributed_litres += amount;
    }
}

int dist_manual(DistributionManager *dm, int tank_id, int area_id, double amount,
                AreaBST *areas) {
    if (!dm || !areas || amount <= 0.0) {
        return -1;
    }

    Tank *tank = tank_get_by_id(tank_id);
    if (!tank) return -1;

    AreaNode *area = bst_search(areas, area_id);
    if (!area) return -1;

    if (tank->current_level < amount) {
        return -2; /* Insufficient water in source tank */
    }

    double needed = area->demand - area->supplied;
    if (needed <= 0.001) {
        return -3; /* Area demand already satisfied */
    }

    /* Distribute up to needed demand */
    double actual = (amount > needed) ? needed : amount;

    tank_update_water(tank_id, -actual);
    bst_add_supply(areas, area_id, actual);

    record_distribution(dm, tank->id, tank->name, area->id, area->name, actual, "Manual", "Success");
    return 0;
}

int dist_automatic(DistributionManager *dm, AreaBST *areas, double *out_allocated_total) {
    if (!dm || !areas) return -1;

    AreaNode area_list[50];
    int area_count = bst_inorder_to_array(areas, area_list, 50);
    if (area_count == 0) return 0;

    /* Sort by Priority (Priority 1 first, then 2, then 3) using stable selection sort */
    for (int i = 0; i < area_count - 1; i++) {
        for (int j = i + 1; j < area_count; j++) {
            if (area_list[j].priority < area_list[i].priority) {
                AreaNode temp = area_list[i];
                area_list[i] = area_list[j];
                area_list[j] = temp;
            }
        }
    }

    Tank tanks[MAX_TANKS];
    int tank_count = tank_get_all(tanks, MAX_TANKS);

    double total_allocated = 0.0;
    int areas_served = 0;

    for (int a = 0; a < area_count; a++) {
        AreaNode *area = bst_search(areas, area_list[a].id);
        if (!area) continue;

        double needed = area->demand - area->supplied;
        if (needed <= 0.001) continue;

        /* Distribute from available tanks */
        for (int t = 0; t < tank_count && needed > 0.001; t++) {
            Tank *tank = tank_get_by_id(tanks[t].id);
            if (!tank || tank->current_level <= 0.001) continue;

            double can_give = (tank->current_level < needed) ? tank->current_level : needed;
            tank_update_water(tank->id, -can_give);
            bst_add_supply(areas, area->id, can_give);

            needed -= can_give;
            total_allocated += can_give;

            record_distribution(dm, tank->id, tank->name, area->id, area->name, can_give, "Automatic", "Success");
            areas_served++;
        }
    }

    if (out_allocated_total) {
        *out_allocated_total = total_allocated;
    }
    return areas_served;
}

int dist_simulate_leakage(DistributionManager *dm, int pipeline_id, double loss_litres,
                          ComplaintQueue *queue, PipelineGraph *graph) {
    (void)graph;
    if (!dm || !queue || loss_litres <= 0.0) return -1;

    dm->total_leakage_loss_litres += loss_litres;

    /* Deduct from first tank with water to simulate system loss */
    Tank tanks[MAX_TANKS];
    int tc = tank_get_all(tanks, MAX_TANKS);
    for (int i = 0; i < tc; i++) {
        Tank *t = tank_get_by_id(tanks[i].id);
        if (t && t->current_level > 0.0) {
            double deduct = (t->current_level < loss_litres) ? t->current_level : loss_litres;
            tank_update_water(t->id, -deduct);
            break;
        }
    }

    /* Automatically register a high-priority complaint in the Circular Queue */
    static int auto_complaint_id = 901;
    Complaint c;
    memset(&c, 0, sizeof(c));
    c.id = auto_complaint_id++;
    c.area_id = 4; /* Junction Trunk area */
    strncpy(c.area_name, "Pipeline Network Grid", sizeof(c.area_name) - 1);
    strncpy(c.issue_type, "Leakage", sizeof(c.issue_type) - 1);

    snprintf(c.description, sizeof(c.description),
             "[SIMULATED SENSOR ALARM] Detected significant pressure drop & %.0f L leakage on Pipeline #%d.",
             loss_litres, pipeline_id);
    strncpy(c.priority, "High", sizeof(c.priority) - 1);
    strncpy(c.status, "Pending", sizeof(c.status) - 1);
    get_timestamp(c.timestamp, sizeof(c.timestamp));

    queue_enqueue(queue, &c);
    return 0;
}

int dist_get_history(const DistributionManager *dm, DistributionRecord *out_records, int max_out) {
    if (!dm || !out_records || max_out <= 0) return 0;
    int n = (dm->count < max_out) ? dm->count : max_out;
    for (int i = 0; i < n; i++) {
        /* Return most recent first */
        out_records[i] = dm->records[dm->count - 1 - i];
    }
    return n;
}

double dist_get_total_distributed(const DistributionManager *dm) {
    return dm ? dm->total_distributed_litres : 0.0;
}

double dist_get_total_leakage(const DistributionManager *dm) {
    return dm ? dm->total_leakage_loss_litres : 0.0;
}
