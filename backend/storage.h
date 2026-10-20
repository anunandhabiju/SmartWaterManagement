/**
 * @file storage.h
 * @brief File Persistence for Smart Water Distribution System
 *
 * Implements disk saving and loading for Tanks, BST Areas, Circular Queue
 * Complaints, Linked List Maintenance, and Pipeline Graph records.
 */

#ifndef STORAGE_H
#define STORAGE_H

#include "tank.h"
#include "bst.h"
#include "queue.h"
#include "linkedlist.h"
#include "graph.h"
#include "distribution.h"

/* Initializes data directories and loads persisted state (or defaults if missing) */
void storage_init(AreaBST *areas, ComplaintQueue *queue, MaintenanceList *list,
                  PipelineGraph *graph, DistributionManager *dm);

/* Saves all system state to disk in backend/data/ */
int storage_save_all(const AreaBST *areas, const ComplaintQueue *queue,
                     const MaintenanceList *list, const PipelineGraph *graph,
                     const DistributionManager *dm);

/* Loads all system state from disk. Returns 0 on success, -1 if files were missing/loaded defaults. */
int storage_load_all(AreaBST *areas, ComplaintQueue *queue,
                     MaintenanceList *list, PipelineGraph *graph,
                     DistributionManager *dm);

/* Resets all components to default demonstration dataset and saves to disk */
void storage_reset_demo_data(AreaBST *areas, ComplaintQueue *queue,
                             MaintenanceList *list, PipelineGraph *graph,
                             DistributionManager *dm);

#endif /* STORAGE_H */
