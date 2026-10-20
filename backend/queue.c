/**
 * @file queue.c
 * @brief Complaint Processing implementation using Circular Queue
 */

#include "queue.h"
#include <stdio.h>
#include <string.h>
#include <time.h>

void queue_init(ComplaintQueue *q) {
    if (!q) return;
    q->front = 0;
    q->rear = -1;
    q->count = 0;
    q->capacity = MAX_QUEUE_CAPACITY;
    memset(q->items, 0, sizeof(q->items));
}

int queue_is_full(const ComplaintQueue *q) {
    if (!q) return 1;
    return (q->count >= q->capacity);
}

int queue_is_empty(const ComplaintQueue *q) {
    if (!q) return 1;
    return (q->count == 0);
}

int queue_enqueue(ComplaintQueue *q, const Complaint *item) {
    if (!q || !item) return -1;
    if (queue_is_full(q)) {
        return -1; /* Queue overflow */
    }

    q->rear = (q->rear + 1) % q->capacity;
    q->items[q->rear] = *item;
    q->count++;
    return 0;
}

int queue_dequeue(ComplaintQueue *q, Complaint *out_item) {
    if (!q) return -1;
    if (queue_is_empty(q)) {
        return -1; /* Queue underflow */
    }

    if (out_item) {
        *out_item = q->items[q->front];
    }

    /* Clear the dequeued slot */
    memset(&q->items[q->front], 0, sizeof(Complaint));
    q->front = (q->front + 1) % q->capacity;
    q->count--;
    return 0;
}

int queue_peek(const ComplaintQueue *q, Complaint *out_item) {
    if (!q || !out_item) return -1;
    if (queue_is_empty(q)) {
        return -1;
    }
    *out_item = q->items[q->front];
    return 0;
}

int queue_get_count(const ComplaintQueue *q) {
    if (!q) return 0;
    return q->count;
}

int queue_get_all(const ComplaintQueue *q, Complaint *out_items, int max_out) {
    if (!q || !out_items || max_out <= 0) return 0;

    int n = (q->count < max_out) ? q->count : max_out;
    for (int i = 0; i < n; i++) {
        int idx = (q->front + i) % q->capacity;
        out_items[i] = q->items[idx];
    }
    return n;
}

int queue_search_by_id(const ComplaintQueue *q, int id, Complaint *out_item) {
    if (!q) return -1;
    for (int i = 0; i < q->count; i++) {
        int idx = (q->front + i) % q->capacity;
        if (q->items[idx].id == id) {
            if (out_item) {
                *out_item = q->items[idx];
            }
            return 0;
        }
    }
    return -1;
}

void queue_clear(ComplaintQueue *q) {
    queue_init(q);
}

void queue_load_defaults(ComplaintQueue *q) {
    queue_init(q);

    Complaint c1 = {
        .id = 101,
        .area_id = 3,
        .area_name = "North Sector Residential",
        .issue_type = "Low Pressure",
        .description = "Persistent low pressure reported during morning peak hours (6 AM - 9 AM).",
        .priority = "Medium",
        .timestamp = "2026-10-20 07:30",
        .status = "Pending"
    };
    queue_enqueue(q, &c1);

    Complaint c2 = {
        .id = 102,
        .area_id = 5,
        .area_name = "Industrial Park East",
        .issue_type = "Leakage",
        .description = "Water pooling observed near junction J-2 main valve pipeline.",
        .priority = "High",
        .timestamp = "2026-10-20 09:15",
        .status = "Pending"
    };
    queue_enqueue(q, &c2);

    Complaint c3 = {
        .id = 103,
        .area_id = 1,
        .area_name = "City General Hospital",
        .issue_type = "No Water Supply",
        .description = "Intermittent supply in auxiliary wing storage tank.",
        .priority = "High",
        .timestamp = "2026-10-20 10:05",
        .status = "Pending"
    };
    queue_enqueue(q, &c3);
}
