/**
 * @file queue.h
 * @brief Complaint Processing using Circular Queue (Data Structure 2)
 *
 * Implements a FIFO Circular Queue in C with bounded capacity, modulo arithmetic
 * to prevent index drift, overflow/empty detection, and complaint inspection.
 */

#ifndef QUEUE_H
#define QUEUE_H

#define MAX_QUEUE_CAPACITY 50

typedef struct {
    int id;
    int area_id;
    char area_name[64];
    char issue_type[32];   /* "Leakage", "Low Pressure", "No Water Supply", "Other" */
    char description[256];
    char priority[16];     /* "High", "Medium", "Low" */
    char timestamp[32];    /* ISO / readable date-time string */
    char status[16];       /* "Pending", "Processing", "Resolved" */
} Complaint;

typedef struct {
    Complaint items[MAX_QUEUE_CAPACITY];
    int front;
    int rear;
    int count;
    int capacity;
} ComplaintQueue;

/* Initializes the circular queue */
void queue_init(ComplaintQueue *q);

/* Returns 1 if queue is full, 0 otherwise */
int queue_is_full(const ComplaintQueue *q);

/* Returns 1 if queue is empty, 0 otherwise */
int queue_is_empty(const ComplaintQueue *q);

/* Enqueues a new complaint. Returns 0 on success, -1 if queue is full. */
int queue_enqueue(ComplaintQueue *q, const Complaint *item);

/* Dequeues the next complaint (FIFO). Returns 0 on success, -1 if queue is empty. */
int queue_dequeue(ComplaintQueue *q, Complaint *out_item);

/* Peeks at the next complaint in line without removing it. Returns 0 on success, -1 if empty. */
int queue_peek(const ComplaintQueue *q, Complaint *out_item);

/* Returns current number of pending complaints */
int queue_get_count(const ComplaintQueue *q);

/* Populates array with all pending complaints in FIFO order */
int queue_get_all(const ComplaintQueue *q, Complaint *out_items, int max_out);

/* Searches for a complaint by ID. Returns 0 if found, -1 if not found. */
int queue_search_by_id(const ComplaintQueue *q, int id, Complaint *out_item);

/* Clears all items in queue */
void queue_clear(ComplaintQueue *q);

/* Helper to load default demonstration complaints into queue */
void queue_load_defaults(ComplaintQueue *q);

#endif /* QUEUE_H */
