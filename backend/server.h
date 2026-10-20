/**
 * @file server.h
 * @brief REST API HTTP Server using Mongoose and cJSON
 *
 * Implements the embedded HTTP server, JSON serialization/deserialization,
 * REST API route dispatching, and static file serving for the frontend.
 */

#ifndef SERVER_H
#define SERVER_H

#include "tank.h"
#include "bst.h"
#include "queue.h"
#include "linkedlist.h"
#include "graph.h"
#include "distribution.h"
#include "storage.h"

/* Server configuration */
#define DEFAULT_PORT "8080"
#define WEB_ROOT "frontend"

/* Global Activity Log item */
typedef struct {
    char timestamp[32];
    char category[32];     /* "Tank", "Distribution", "Complaint", "Maintenance", "Network" */
    char description[256];
    char status[16];        /* "Success", "Warning", "Error" */
} ActivityItem;

#define MAX_ACTIVITIES 40

/* Initializes and runs the HTTP server event loop */
void server_start(const char *port);

/* Stops the HTTP server */
void server_stop(void);

/* Adds an activity entry to the activity log */
void server_log_activity(const char *category, const char *description, const char *status);

#endif /* SERVER_H */
