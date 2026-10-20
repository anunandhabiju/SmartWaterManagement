/**
 * @file distribution.h
 * @brief Water Distribution and Allocation Engine
 *
 * Implements manual distribution, automatic priority-based allocation
 * (essential services first), shortage detection, and leakage simulation.
 */

#ifndef DISTRIBUTION_H
#define DISTRIBUTION_H

#include "tank.h"
#include "bst.h"
#include "queue.h"
#include "linkedlist.h"
#include "graph.h"

typedef struct {
    int id;
    int source_tank_id;
    char source_tank_name[64];
    int dest_area_id;
    char dest_area_name[64];
    double amount_litres;
    char mode[16];          /* "Manual" or "Automatic" */
    char timestamp[32];
    char status[16];        /* "Success", "Partial", "Failed" */
} DistributionRecord;

#define MAX_DISTRIBUTION_HISTORY 50

typedef struct {
    DistributionRecord records[MAX_DISTRIBUTION_HISTORY];
    int count;
    double total_distributed_litres;
    double total_leakage_loss_litres;
} DistributionManager;

/* Initializes distribution manager */
void dist_init(DistributionManager *dm);

/* Performs manual water distribution from tank to area.
 * Returns: 0 on success, -1 invalid params, -2 insufficient tank water, -3 demand already satisfied. */
int dist_manual(DistributionManager *dm, int tank_id, int area_id, double amount,
                AreaBST *areas);

/* Performs automatic water distribution using priority queue logic (Hospital -> Commercial -> Residential).
 * Returns: number of areas served, or -1 on error. */
int dist_automatic(DistributionManager *dm, AreaBST *areas, double *out_allocated_total);

/* Simulates pipeline leakage: deducts loss from accounting and auto-registers a complaint */
int dist_simulate_leakage(DistributionManager *dm, int pipeline_id, double loss_litres,
                          ComplaintQueue *queue, PipelineGraph *graph);

/* Gets all distribution history records */
int dist_get_history(const DistributionManager *dm, DistributionRecord *out_records, int max_out);

/* Returns total water distributed */
double dist_get_total_distributed(const DistributionManager *dm);

/* Returns total simulated leakage loss */
double dist_get_total_leakage(const DistributionManager *dm);

#endif /* DISTRIBUTION_H */
