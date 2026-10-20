/**
 * @file server.c
 * @brief REST API HTTP Server implementation using Mongoose and cJSON
 */

#include "server.h"
#include "mongoose.h"
#include "cJSON.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/* Global state singletons */
static AreaBST g_areas;
static ComplaintQueue g_complaint_queue;
static MaintenanceList g_maintenance_list;
static PipelineGraph g_pipeline_graph;
static DistributionManager g_dist_manager;

/* Activity Log */
static ActivityItem g_activities[MAX_ACTIVITIES];
static int g_activity_count = 0;

static int g_server_running = 1;

static void get_current_time(char *buf, size_t sz) {
    time_t now = time(NULL);
    struct tm *tm_info = localtime(&now);
    if (tm_info) {
        strftime(buf, sz, "%Y-%m-%d %H:%M:%S", tm_info);
    } else {
        snprintf(buf, sz, "2026-10-20 12:00:00");
    }
}

void server_log_activity(const char *category, const char *description, const char *status) {
    if (g_activity_count >= MAX_ACTIVITIES) {
        for (int i = 0; i < MAX_ACTIVITIES - 1; i++) {
            g_activities[i] = g_activities[i + 1];
        }
        g_activity_count = MAX_ACTIVITIES - 1;
    }

    ActivityItem *act = &g_activities[g_activity_count++];
    get_current_time(act->timestamp, sizeof(act->timestamp));
    strncpy(act->category, category ? category : "System", sizeof(act->category) - 1);
    act->category[sizeof(act->category) - 1] = '\0';
    strncpy(act->description, description ? description : "", sizeof(act->description) - 1);
    act->description[sizeof(act->description) - 1] = '\0';
    strncpy(act->status, status ? status : "Success", sizeof(act->status) - 1);
    act->status[sizeof(act->status) - 1] = '\0';
}

/* Helper to send JSON HTTP responses */
static void send_json_response(struct mg_connection *c, int status_code, cJSON *json_obj) {
    char *json_str = cJSON_PrintUnformatted(json_obj);
    if (!json_str) {
        mg_http_reply(c, 500, "Content-Type: application/json\r\nAccess-Control-Allow-Origin: *\r\n", "{\"success\":false,\"error\":\"JSON generation failed\"}");
        return;
    }

    mg_http_reply(c, status_code,
                  "Content-Type: application/json\r\n"
                  "Access-Control-Allow-Origin: *\r\n"
                  "Access-Control-Allow-Methods: GET, POST, PATCH, DELETE, OPTIONS\r\n"
                  "Access-Control-Allow-Headers: Content-Type\r\n",
                  "%s", json_str);

    cJSON_free(json_str);
}

static void send_error(struct mg_connection *c, int status_code, const char *msg) {
    cJSON *resp = cJSON_CreateObject();
    cJSON_AddBoolToObject(resp, "success", 0);
    cJSON_AddStringToObject(resp, "error", msg ? msg : "An error occurred");
    send_json_response(c, status_code, resp);
    cJSON_Delete(resp);
}

static void send_success(struct mg_connection *c, cJSON *data_obj, const char *msg) {
    cJSON *resp = cJSON_CreateObject();
    cJSON_AddBoolToObject(resp, "success", 1);
    if (msg) {
        cJSON_AddStringToObject(resp, "message", msg);
    }
    if (data_obj) {
        cJSON_AddItemToObject(resp, "data", data_obj);
    }
    send_json_response(c, 200, resp);
    cJSON_Delete(resp);
}

/* --- API HANDLERS --- */

/* GET /api/dashboard */
static void handle_get_dashboard(struct mg_connection *c) {
    double total_capacity = 0.0, total_available = 0.0;
    tank_get_totals(&total_capacity, &total_available);

    double total_demand = 0.0, total_supplied = 0.0, total_unmet = 0.0;
    bst_get_totals(&g_areas, &total_demand, &total_supplied, &total_unmet);

    double distributed = dist_get_total_distributed(&g_dist_manager);
    double leakage = dist_get_total_leakage(&g_dist_manager);

    int pending_complaints = queue_get_count(&g_complaint_queue);

    cJSON *data = cJSON_CreateObject();
    cJSON_AddNumberToObject(data, "total_capacity_litres", total_capacity);
    cJSON_AddNumberToObject(data, "total_available_litres", total_available);
    cJSON_AddNumberToObject(data, "total_distributed_litres", distributed);
    cJSON_AddNumberToObject(data, "total_demand_litres", total_demand);
    cJSON_AddNumberToObject(data, "total_supplied_litres", total_supplied);
    cJSON_AddNumberToObject(data, "total_unmet_demand_litres", total_unmet);
    cJSON_AddNumberToObject(data, "simulated_leakage_loss_litres", leakage);
    double fulfillment = (total_demand > 0.0) ? (total_supplied / total_demand) * 100.0 : 100.0;
    cJSON_AddNumberToObject(data, "fulfillment_percentage", fulfillment);
    cJSON_AddNumberToObject(data, "pending_complaints_count", pending_complaints);
    cJSON_AddNumberToObject(data, "maintenance_records_count", list_get_count(&g_maintenance_list));

    /* Tank levels array */
    Tank tanks[MAX_TANKS];
    int tank_cnt = tank_get_all(tanks, MAX_TANKS);
    cJSON *tanks_arr = cJSON_CreateArray();
    for (int i = 0; i < tank_cnt; i++) {
        cJSON *t = cJSON_CreateObject();
        cJSON_AddNumberToObject(t, "id", tanks[i].id);
        cJSON_AddStringToObject(t, "name", tanks[i].name);
        cJSON_AddNumberToObject(t, "capacity", tanks[i].capacity);
        cJSON_AddNumberToObject(t, "current_level", tanks[i].current_level);
        double pct = (tanks[i].capacity > 0.0) ? (tanks[i].current_level / tanks[i].capacity) * 100.0 : 0.0;
        cJSON_AddNumberToObject(t, "percentage", pct);
        cJSON_AddStringToObject(t, "status", tanks[i].status);
        cJSON_AddItemToArray(tanks_arr, t);
    }
    cJSON_AddItemToObject(data, "tank_levels", tanks_arr);

    /* Area demands array */
    AreaNode areas[50];
    int area_cnt = bst_inorder_to_array(&g_areas, areas, 50);
    cJSON *areas_arr = cJSON_CreateArray();
    for (int i = 0; i < area_cnt; i++) {
        cJSON *a = cJSON_CreateObject();
        cJSON_AddNumberToObject(a, "id", areas[i].id);
        cJSON_AddStringToObject(a, "name", areas[i].name);
        cJSON_AddNumberToObject(a, "demand", areas[i].demand);
        cJSON_AddNumberToObject(a, "supplied", areas[i].supplied);
        double unmet_a = (areas[i].demand > areas[i].supplied) ? (areas[i].demand - areas[i].supplied) : 0.0;
        cJSON_AddNumberToObject(a, "unmet", unmet_a);
        double pct_a = (areas[i].demand > 0.0) ? (areas[i].supplied / areas[i].demand) * 100.0 : 100.0;
        cJSON_AddNumberToObject(a, "percentage", pct_a);
        cJSON_AddNumberToObject(a, "priority", areas[i].priority);
        cJSON_AddStringToObject(a, "category", areas[i].category);
        cJSON_AddItemToArray(areas_arr, a);
    }
    cJSON_AddItemToObject(data, "area_demands", areas_arr);

    /* Recent activities */
    cJSON *act_arr = cJSON_CreateArray();
    for (int i = g_activity_count - 1; i >= 0; i--) {
        cJSON *item = cJSON_CreateObject();
        cJSON_AddStringToObject(item, "timestamp", g_activities[i].timestamp);
        cJSON_AddStringToObject(item, "category", g_activities[i].category);
        cJSON_AddStringToObject(item, "description", g_activities[i].description);
        cJSON_AddStringToObject(item, "status", g_activities[i].status);
        cJSON_AddItemToArray(act_arr, item);
    }
    cJSON_AddItemToObject(data, "recent_activities", act_arr);

    /* Dynamic Notifications */
    cJSON *notifs = cJSON_CreateArray();
    for (int i = 0; i < tank_cnt; i++) {
        if (strcmp(tanks[i].status, "Low") == 0) {
            cJSON *n = cJSON_CreateObject();
            cJSON_AddStringToObject(n, "level", "warning");
            char msg[128];
            snprintf(msg, sizeof(msg), "%s water level is critically low (< 25%%).", tanks[i].name);
            cJSON_AddStringToObject(n, "message", msg);
            cJSON_AddItemToArray(notifs, n);
        } else if (strcmp(tanks[i].status, "Empty") == 0) {
            cJSON *n = cJSON_CreateObject();
            cJSON_AddStringToObject(n, "level", "danger");
            char msg[128];
            snprintf(msg, sizeof(msg), "%s is completely EMPTY!", tanks[i].name);
            cJSON_AddStringToObject(n, "message", msg);
            cJSON_AddItemToArray(notifs, n);
        }
    }
    if (pending_complaints >= 5) {
        cJSON *n = cJSON_CreateObject();
        cJSON_AddStringToObject(n, "level", "warning");
        cJSON_AddStringToObject(n, "message", "Complaint Queue backlog: 5 or more pending customer complaints.");
        cJSON_AddItemToArray(notifs, n);
    }
    cJSON_AddItemToObject(data, "notifications", notifs);

    send_success(c, data, "Dashboard metrics retrieved");
}

/* GET /api/tanks */
static void handle_get_tanks(struct mg_connection *c) {
    Tank tanks[MAX_TANKS];
    int count = tank_get_all(tanks, MAX_TANKS);

    cJSON *arr = cJSON_CreateArray();
    for (int i = 0; i < count; i++) {
        cJSON *t = cJSON_CreateObject();
        cJSON_AddNumberToObject(t, "id", tanks[i].id);
        cJSON_AddStringToObject(t, "name", tanks[i].name);
        cJSON_AddNumberToObject(t, "capacity", tanks[i].capacity);
        cJSON_AddNumberToObject(t, "current_level", tanks[i].current_level);
        double pct = (tanks[i].capacity > 0.0) ? (tanks[i].current_level / tanks[i].capacity) * 100.0 : 0.0;
        cJSON_AddNumberToObject(t, "percentage", pct);
        cJSON_AddStringToObject(t, "location", tanks[i].location);
        cJSON_AddStringToObject(t, "status", tanks[i].status);
        cJSON_AddItemToArray(arr, t);
    }

    cJSON *data = cJSON_CreateObject();
    cJSON_AddItemToObject(data, "tanks", arr);
    send_success(c, data, "Tanks retrieved");
}

/* POST /api/tanks */
static void handle_post_tanks(struct mg_connection *c, struct mg_http_message *hm) {
    cJSON *req = cJSON_ParseWithLength(hm->body.buf, hm->body.len);
    if (!req) {
        send_error(c, 400, "Invalid JSON payload");
        return;
    }

    cJSON *name = cJSON_GetObjectItem(req, "name");
    cJSON *cap = cJSON_GetObjectItem(req, "capacity");
    cJSON *level = cJSON_GetObjectItem(req, "initial_level");
    cJSON *loc = cJSON_GetObjectItem(req, "location");

    if (!name || !cap || !level || cap->valuedouble <= 0.0 || level->valuedouble < 0.0) {
        cJSON_Delete(req);
        send_error(c, 400, "Invalid parameters: capacity must be > 0 and level >= 0");
        return;
    }

    int id = tank_add(name->valuestring, cap->valuedouble, level->valuedouble, loc ? loc->valuestring : "Sector");
    cJSON_Delete(req);

    if (id < 0) {
        send_error(c, 400, "Failed to add tank: Maximum capacity reached or invalid values");
        return;
    }

    storage_save_all(&g_areas, &g_complaint_queue, &g_maintenance_list, &g_pipeline_graph, &g_dist_manager);
    char desc[128];
    snprintf(desc, sizeof(desc), "Added new tank #%d with capacity %.0f L", id, cap->valuedouble);
    server_log_activity("Tank", desc, "Success");

    cJSON *data = cJSON_CreateObject();
    cJSON_AddNumberToObject(data, "tank_id", id);
    send_success(c, data, "Tank created successfully");
}

/* POST /api/tanks/add-water */
static void handle_post_tanks_add_water(struct mg_connection *c, struct mg_http_message *hm) {
    cJSON *req = cJSON_ParseWithLength(hm->body.buf, hm->body.len);
    if (!req) {
        send_error(c, 400, "Invalid JSON payload");
        return;
    }

    cJSON *id = cJSON_GetObjectItem(req, "tank_id");
    cJSON *amount = cJSON_GetObjectItem(req, "amount");

    if (!id || !amount || amount->valuedouble <= 0.0) {
        cJSON_Delete(req);
        send_error(c, 400, "Invalid parameters: amount must be positive");
        return;
    }

    int res = tank_update_water(id->valueint, amount->valuedouble);
    cJSON_Delete(req);

    if (res == -1) {
        send_error(c, 404, "Tank ID not found");
        return;
    } else if (res == -2) {
        send_error(c, 400, "Cannot add water: Exceeds maximum tank capacity");
        return;
    } else if (res == -3) {
        send_error(c, 400, "Cannot deduct water: Level would become negative");
        return;
    }

    storage_save_all(&g_areas, &g_complaint_queue, &g_maintenance_list, &g_pipeline_graph, &g_dist_manager);
    Tank *t = tank_get_by_id(id->valueint);
    char desc[128];
    snprintf(desc, sizeof(desc), "Refilled %.0f L into %s", amount->valuedouble, t ? t->name : "Tank");
    server_log_activity("Tank", desc, "Success");

    send_success(c, NULL, "Water level updated successfully");
}

/* DELETE /api/tanks */
static void handle_delete_tanks(struct mg_connection *c, struct mg_http_message *hm) {
    char id_str[32] = {0};
    mg_http_get_var(&hm->query, "id", id_str, sizeof(id_str));
    int tank_id = atoi(id_str);

    if (tank_id <= 0) {
        send_error(c, 400, "Invalid or missing tank ID");
        return;
    }

    if (tank_delete(tank_id) != 0) {
        send_error(c, 404, "Tank not found");
        return;
    }

    storage_save_all(&g_areas, &g_complaint_queue, &g_maintenance_list, &g_pipeline_graph, &g_dist_manager);
    char desc[128];
    snprintf(desc, sizeof(desc), "Decommissioned tank ID %d", tank_id);
    server_log_activity("Tank", desc, "Warning");

    send_success(c, NULL, "Tank deleted successfully");
}

/* GET /api/areas */
static void handle_get_areas(struct mg_connection *c) {
    AreaNode arr[50];
    int count = bst_inorder_to_array(&g_areas, arr, 50);

    cJSON *areas_arr = cJSON_CreateArray();
    for (int i = 0; i < count; i++) {
        cJSON *a = cJSON_CreateObject();
        cJSON_AddNumberToObject(a, "id", arr[i].id);
        cJSON_AddStringToObject(a, "name", arr[i].name);
        cJSON_AddNumberToObject(a, "demand", arr[i].demand);
        cJSON_AddNumberToObject(a, "supplied", arr[i].supplied);
        double unmet = (arr[i].demand > arr[i].supplied) ? (arr[i].demand - arr[i].supplied) : 0.0;
        cJSON_AddNumberToObject(a, "unmet", unmet);
        double pct = (arr[i].demand > 0.0) ? (arr[i].supplied / arr[i].demand) * 100.0 : 100.0;
        cJSON_AddNumberToObject(a, "percentage", pct);
        cJSON_AddNumberToObject(a, "priority", arr[i].priority);
        cJSON_AddStringToObject(a, "category", arr[i].category);
        cJSON_AddItemToArray(areas_arr, a);
    }

    cJSON *data = cJSON_CreateObject();
    cJSON_AddItemToObject(data, "areas", areas_arr);
    cJSON_AddNumberToObject(data, "total_count", count);
    send_success(c, data, "Areas retrieved in sorted ID order (BST Inorder)");
}

/* POST /api/areas */
static void handle_post_areas(struct mg_connection *c, struct mg_http_message *hm) {
    cJSON *req = cJSON_ParseWithLength(hm->body.buf, hm->body.len);
    if (!req) {
        send_error(c, 400, "Invalid JSON payload");
        return;
    }

    cJSON *id = cJSON_GetObjectItem(req, "id");
    cJSON *name = cJSON_GetObjectItem(req, "name");
    cJSON *demand = cJSON_GetObjectItem(req, "demand");
    cJSON *priority = cJSON_GetObjectItem(req, "priority");
    cJSON *category = cJSON_GetObjectItem(req, "category");

    if (!id || !name || !demand || demand->valuedouble < 0.0) {
        cJSON_Delete(req);
        send_error(c, 400, "Missing or invalid fields");
        return;
    }

    int prio = priority ? priority->valueint : 3;
    const char *cat = category ? category->valuestring : "Residential";

    int res = bst_insert(&g_areas, id->valueint, name->valuestring, demand->valuedouble, prio, cat);
    cJSON_Delete(req);

    if (res == -1) {
        send_error(c, 409, "Duplicate Area ID: An area with this ID already exists in the BST");
        return;
    } else if (res < 0) {
        send_error(c, 500, "Memory allocation error");
        return;
    }

    storage_save_all(&g_areas, &g_complaint_queue, &g_maintenance_list, &g_pipeline_graph, &g_dist_manager);
    char desc[128];
    snprintf(desc, sizeof(desc), "Registered new area #%d (%s) with %.0f L demand", id->valueint, name->valuestring, demand->valuedouble);
    server_log_activity("Area", desc, "Success");

    send_success(c, NULL, "Area inserted successfully into BST");
}

/* GET /api/areas/search */
static void handle_search_area(struct mg_connection *c, struct mg_http_message *hm) {
    char id_str[32] = {0};
    mg_http_get_var(&hm->query, "id", id_str, sizeof(id_str));
    int area_id = atoi(id_str);

    if (area_id <= 0) {
        send_error(c, 400, "Invalid or missing search area ID");
        return;
    }

    AreaNode *a = bst_search(&g_areas, area_id);
    if (!a) {
        send_error(c, 404, "Area not found in BST");
        return;
    }

    cJSON *data = cJSON_CreateObject();
    cJSON_AddNumberToObject(data, "id", a->id);
    cJSON_AddStringToObject(data, "name", a->name);
    cJSON_AddNumberToObject(data, "demand", a->demand);
    cJSON_AddNumberToObject(data, "supplied", a->supplied);
    double unmet = (a->demand > a->supplied) ? (a->demand - a->supplied) : 0.0;
    cJSON_AddNumberToObject(data, "unmet", unmet);
    cJSON_AddNumberToObject(data, "priority", a->priority);
    cJSON_AddStringToObject(data, "category", a->category);

    send_success(c, data, "Area found via BST O(log n) search");
}

/* PATCH /api/areas */
static void handle_patch_areas(struct mg_connection *c, struct mg_http_message *hm) {
    cJSON *req = cJSON_ParseWithLength(hm->body.buf, hm->body.len);
    if (!req) {
        send_error(c, 400, "Invalid JSON payload");
        return;
    }

    cJSON *id = cJSON_GetObjectItem(req, "id");
    cJSON *demand = cJSON_GetObjectItem(req, "demand");
    cJSON *priority = cJSON_GetObjectItem(req, "priority");

    if (!id) {
        cJSON_Delete(req);
        send_error(c, 400, "Missing area id");
        return;
    }

    double d = demand ? demand->valuedouble : -1.0;
    int p = priority ? priority->valueint : -1;

    int res = bst_update(&g_areas, id->valueint, d, p);
    cJSON_Delete(req);

    if (res != 0) {
        send_error(c, 404, "Area not found");
        return;
    }

    storage_save_all(&g_areas, &g_complaint_queue, &g_maintenance_list, &g_pipeline_graph, &g_dist_manager);
    send_success(c, NULL, "Area updated successfully");
}

/* GET /api/distribution */
static void handle_get_distribution(struct mg_connection *c) {
    DistributionRecord records[MAX_DISTRIBUTION_HISTORY];
    int count = dist_get_history(&g_dist_manager, records, MAX_DISTRIBUTION_HISTORY);

    cJSON *rec_arr = cJSON_CreateArray();
    for (int i = 0; i < count; i++) {
        cJSON *r = cJSON_CreateObject();
        cJSON_AddNumberToObject(r, "id", records[i].id);
        cJSON_AddNumberToObject(r, "source_tank_id", records[i].source_tank_id);
        cJSON_AddStringToObject(r, "source_tank_name", records[i].source_tank_name);
        cJSON_AddNumberToObject(r, "dest_area_id", records[i].dest_area_id);
        cJSON_AddStringToObject(r, "dest_area_name", records[i].dest_area_name);
        cJSON_AddNumberToObject(r, "amount_litres", records[i].amount_litres);
        cJSON_AddStringToObject(r, "mode", records[i].mode);
        cJSON_AddStringToObject(r, "status", records[i].status);
        cJSON_AddStringToObject(r, "timestamp", records[i].timestamp);
        cJSON_AddItemToArray(rec_arr, r);
    }

    cJSON *data = cJSON_CreateObject();
    cJSON_AddItemToObject(data, "history", rec_arr);
    cJSON_AddNumberToObject(data, "total_distributed_litres", dist_get_total_distributed(&g_dist_manager));
    cJSON_AddNumberToObject(data, "total_leakage_loss_litres", dist_get_total_leakage(&g_dist_manager));

    send_success(c, data, "Distribution history retrieved");
}

/* POST /api/distribution/manual */
static void handle_post_dist_manual(struct mg_connection *c, struct mg_http_message *hm) {
    cJSON *req = cJSON_ParseWithLength(hm->body.buf, hm->body.len);
    if (!req) {
        send_error(c, 400, "Invalid JSON payload");
        return;
    }

    cJSON *tank_id = cJSON_GetObjectItem(req, "tank_id");
    cJSON *area_id = cJSON_GetObjectItem(req, "area_id");
    cJSON *amount = cJSON_GetObjectItem(req, "amount");

    if (!tank_id || !area_id || !amount || amount->valuedouble <= 0.0) {
        cJSON_Delete(req);
        send_error(c, 400, "Invalid parameters");
        return;
    }

    int res = dist_manual(&g_dist_manager, tank_id->valueint, area_id->valueint, amount->valuedouble, &g_areas);
    cJSON_Delete(req);

    if (res == -1) {
        send_error(c, 404, "Invalid Tank ID or Area ID");
        return;
    } else if (res == -2) {
        send_error(c, 400, "Insufficient water in source tank");
        return;
    } else if (res == -3) {
        send_error(c, 400, "Target area demand is already fully satisfied");
        return;
    }

    storage_save_all(&g_areas, &g_complaint_queue, &g_maintenance_list, &g_pipeline_graph, &g_dist_manager);
    char desc[128];
    snprintf(desc, sizeof(desc), "Manual distribution of %.0f L to Area #%d", amount->valuedouble, area_id->valueint);
    server_log_activity("Distribution", desc, "Success");

    send_success(c, NULL, "Water distributed successfully");
}

/* POST /api/distribution/automatic */
static void handle_post_dist_automatic(struct mg_connection *c) {
    double total_allocated = 0.0;
    int areas_served = dist_automatic(&g_dist_manager, &g_areas, &total_allocated);

    if (areas_served < 0) {
        send_error(c, 500, "Automatic allocation algorithm failed");
        return;
    }

    storage_save_all(&g_areas, &g_complaint_queue, &g_maintenance_list, &g_pipeline_graph, &g_dist_manager);
    char desc[128];
    snprintf(desc, sizeof(desc), "Priority automatic allocation completed: %.0f L supplied across %d areas", total_allocated, areas_served);
    server_log_activity("Distribution", desc, "Success");

    cJSON *data = cJSON_CreateObject();
    cJSON_AddNumberToObject(data, "total_allocated_litres", total_allocated);
    cJSON_AddNumberToObject(data, "areas_served", areas_served);
    send_success(c, data, "Automatic priority allocation completed successfully");
}

/* GET /api/complaints */
static void handle_get_complaints(struct mg_connection *c) {
    Complaint items[MAX_QUEUE_CAPACITY];
    int count = queue_get_all(&g_complaint_queue, items, MAX_QUEUE_CAPACITY);

    cJSON *arr = cJSON_CreateArray();
    for (int i = 0; i < count; i++) {
        cJSON *item = cJSON_CreateObject();
        cJSON_AddNumberToObject(item, "id", items[i].id);
        cJSON_AddNumberToObject(item, "area_id", items[i].area_id);
        cJSON_AddStringToObject(item, "area_name", items[i].area_name);
        cJSON_AddStringToObject(item, "issue_type", items[i].issue_type);
        cJSON_AddStringToObject(item, "description", items[i].description);
        cJSON_AddStringToObject(item, "priority", items[i].priority);
        cJSON_AddStringToObject(item, "timestamp", items[i].timestamp);
        cJSON_AddStringToObject(item, "status", items[i].status);
        cJSON_AddItemToArray(arr, item);
    }

    Complaint next_item;
    cJSON *next_obj = NULL;
    if (queue_peek(&g_complaint_queue, &next_item) == 0) {
        next_obj = cJSON_CreateObject();
        cJSON_AddNumberToObject(next_obj, "id", next_item.id);
        cJSON_AddStringToObject(next_obj, "area_name", next_item.area_name);
        cJSON_AddStringToObject(next_obj, "issue_type", next_item.issue_type);
        cJSON_AddStringToObject(next_obj, "priority", next_item.priority);
    }

    cJSON *data = cJSON_CreateObject();
    cJSON_AddItemToObject(data, "complaints", arr);
    cJSON_AddNumberToObject(data, "count", count);
    cJSON_AddNumberToObject(data, "capacity", MAX_QUEUE_CAPACITY);
    cJSON_AddBoolToObject(data, "is_full", queue_is_full(&g_complaint_queue));
    cJSON_AddBoolToObject(data, "is_empty", queue_is_empty(&g_complaint_queue));
    if (next_obj) {
        cJSON_AddItemToObject(data, "next_to_process", next_obj);
    }

    send_success(c, data, "Complaints queue retrieved");
}

/* POST /api/complaints */
static void handle_post_complaints(struct mg_connection *c, struct mg_http_message *hm) {
    cJSON *req = cJSON_ParseWithLength(hm->body.buf, hm->body.len);
    if (!req) {
        send_error(c, 400, "Invalid JSON payload");
        return;
    }

    if (queue_is_full(&g_complaint_queue)) {
        cJSON_Delete(req);
        send_error(c, 507, "Complaint Queue is full! Cannot register more complaints until processed.");
        return;
    }

    cJSON *area_id = cJSON_GetObjectItem(req, "area_id");
    cJSON *area_name = cJSON_GetObjectItem(req, "area_name");
    cJSON *issue_type = cJSON_GetObjectItem(req, "issue_type");
    cJSON *desc = cJSON_GetObjectItem(req, "description");
    cJSON *priority = cJSON_GetObjectItem(req, "priority");

    static int s_next_complaint_id = 110;
    Complaint comp;
    memset(&comp, 0, sizeof(comp));
    comp.id = s_next_complaint_id++;
    comp.area_id = area_id ? area_id->valueint : 0;
    strncpy(comp.area_name, area_name ? area_name->valuestring : "General Zone", sizeof(comp.area_name) - 1);
    strncpy(comp.issue_type, issue_type ? issue_type->valuestring : "Other", sizeof(comp.issue_type) - 1);
    strncpy(comp.description, desc ? desc->valuestring : "No description", sizeof(comp.description) - 1);
    strncpy(comp.priority, priority ? priority->valuestring : "Medium", sizeof(comp.priority) - 1);
    strncpy(comp.status, "Pending", sizeof(comp.status) - 1);
    get_current_time(comp.timestamp, sizeof(comp.timestamp));

    cJSON_Delete(req);

    if (queue_enqueue(&g_complaint_queue, &comp) != 0) {
        send_error(c, 500, "Failed to enqueue complaint");
        return;
    }

    storage_save_all(&g_areas, &g_complaint_queue, &g_maintenance_list, &g_pipeline_graph, &g_dist_manager);
    char act_desc[128];
    snprintf(act_desc, sizeof(act_desc), "Registered complaint #%d (%s) for %s", comp.id, comp.issue_type, comp.area_name);
    server_log_activity("Complaint", act_desc, "Warning");

    cJSON *data = cJSON_CreateObject();
    cJSON_AddNumberToObject(data, "complaint_id", comp.id);
    send_success(c, data, "Complaint enqueued into circular queue");
}

/* POST /api/complaints/process-next */
static void handle_post_process_complaint(struct mg_connection *c) {
    if (queue_is_empty(&g_complaint_queue)) {
        send_error(c, 400, "Complaint queue is empty: Nothing to process");
        return;
    }

    Complaint c_item;
    if (queue_dequeue(&g_complaint_queue, &c_item) != 0) {
        send_error(c, 500, "Failed to dequeue complaint");
        return;
    }

    /* Convert complaint into a Maintenance Record in the Singly Linked List */
    int m_id = list_insert(&g_maintenance_list, c_item.id, c_item.area_name, c_item.description, "In Progress", "Field Technician Assigned");

    storage_save_all(&g_areas, &g_complaint_queue, &g_maintenance_list, &g_pipeline_graph, &g_dist_manager);

    char act_desc[128];
    snprintf(act_desc, sizeof(act_desc), "Processed complaint #%d (FIFO) -> Created Maintenance Order #%d", c_item.id, m_id);
    server_log_activity("Complaint", act_desc, "Success");

    cJSON *data = cJSON_CreateObject();
    cJSON_AddNumberToObject(data, "processed_complaint_id", c_item.id);
    cJSON_AddNumberToObject(data, "created_maintenance_id", m_id);
    cJSON_AddStringToObject(data, "area_name", c_item.area_name);
    send_success(c, data, "Complaint dequeued and transferred to Maintenance Linked List");
}

/* GET /api/maintenance */
static void handle_get_maintenance(struct mg_connection *c, struct mg_http_message *hm) {
    char filter_status[32] = {0};
    mg_http_get_var(&hm->query, "status", filter_status, sizeof(filter_status));

    MaintenanceNode arr[100];
    int count = list_to_array(&g_maintenance_list, arr, 100);

    cJSON *m_arr = cJSON_CreateArray();
    for (int i = 0; i < count; i++) {
        if (filter_status[0] != '\0' && strcmp(arr[i].status, filter_status) != 0) {
            continue;
        }

        cJSON *node = cJSON_CreateObject();
        cJSON_AddNumberToObject(node, "id", arr[i].id);
        cJSON_AddNumberToObject(node, "complaint_id", arr[i].complaint_id);
        cJSON_AddStringToObject(node, "area_name", arr[i].area_name);
        cJSON_AddStringToObject(node, "issue", arr[i].issue);
        cJSON_AddStringToObject(node, "status", arr[i].status);
        cJSON_AddStringToObject(node, "technician", arr[i].technician);
        cJSON_AddStringToObject(node, "created_at", arr[i].created_at);
        cJSON_AddStringToObject(node, "resolved_at", arr[i].resolved_at);
        cJSON_AddItemToArray(m_arr, node);
    }

    cJSON *data = cJSON_CreateObject();
    cJSON_AddItemToObject(data, "records", m_arr);
    cJSON_AddNumberToObject(data, "total_count", list_get_count(&g_maintenance_list));
    send_success(c, data, "Maintenance records retrieved from Singly Linked List");
}

/* POST /api/maintenance */
static void handle_post_maintenance(struct mg_connection *c, struct mg_http_message *hm) {
    cJSON *req = cJSON_ParseWithLength(hm->body.buf, hm->body.len);
    if (!req) {
        send_error(c, 400, "Invalid JSON payload");
        return;
    }

    cJSON *comp_id = cJSON_GetObjectItem(req, "complaint_id");
    cJSON *area = cJSON_GetObjectItem(req, "area_name");
    cJSON *issue = cJSON_GetObjectItem(req, "issue");
    cJSON *status = cJSON_GetObjectItem(req, "status");
    cJSON *tech = cJSON_GetObjectItem(req, "technician");

    int cid = comp_id ? comp_id->valueint : 0;
    const char *a = area ? area->valuestring : "Infrastructure";
    const char *iss = issue ? issue->valuestring : "Standard inspection";
    const char *st = status ? status->valuestring : "Pending";
    const char *tc = tech ? tech->valuestring : "Unassigned";

    int id = list_insert(&g_maintenance_list, cid, a, iss, st, tc);
    cJSON_Delete(req);

    if (id < 0) {
        send_error(c, 500, "Memory allocation failure in Linked List");
        return;
    }

    storage_save_all(&g_areas, &g_complaint_queue, &g_maintenance_list, &g_pipeline_graph, &g_dist_manager);
    char desc[128];
    snprintf(desc, sizeof(desc), "Created maintenance record #%d (%s)", id, iss);
    server_log_activity("Maintenance", desc, "Success");

    cJSON *data = cJSON_CreateObject();
    cJSON_AddNumberToObject(data, "id", id);
    send_success(c, data, "Maintenance record inserted into Singly Linked List");
}

/* PATCH /api/maintenance/status */
static void handle_patch_maintenance_status(struct mg_connection *c, struct mg_http_message *hm) {
    cJSON *req = cJSON_ParseWithLength(hm->body.buf, hm->body.len);
    if (!req) {
        send_error(c, 400, "Invalid JSON payload");
        return;
    }

    cJSON *id = cJSON_GetObjectItem(req, "id");
    cJSON *status = cJSON_GetObjectItem(req, "status");
    cJSON *tech = cJSON_GetObjectItem(req, "technician");

    if (!id || !status) {
        cJSON_Delete(req);
        send_error(c, 400, "Missing id or status");
        return;
    }

    int res = list_update_status(&g_maintenance_list, id->valueint, status->valuestring, tech ? tech->valuestring : NULL);
    cJSON_Delete(req);

    if (res != 0) {
        send_error(c, 404, "Maintenance record ID not found");
        return;
    }

    storage_save_all(&g_areas, &g_complaint_queue, &g_maintenance_list, &g_pipeline_graph, &g_dist_manager);
    char desc[128];
    snprintf(desc, sizeof(desc), "Updated Maintenance #%d status to '%s'", id->valueint, status->valuestring);
    server_log_activity("Maintenance", desc, "Success");

    send_success(c, NULL, "Maintenance status updated");
}

/* DELETE /api/maintenance */
static void handle_delete_maintenance(struct mg_connection *c, struct mg_http_message *hm) {
    char delete_resolved[16] = {0};
    char id_str[32] = {0};
    mg_http_get_var(&hm->query, "delete_resolved", delete_resolved, sizeof(delete_resolved));
    mg_http_get_var(&hm->query, "id", id_str, sizeof(id_str));

    if (strcmp(delete_resolved, "true") == 0 || strcmp(delete_resolved, "1") == 0) {
        int cnt = list_delete_resolved(&g_maintenance_list);
        storage_save_all(&g_areas, &g_complaint_queue, &g_maintenance_list, &g_pipeline_graph, &g_dist_manager);

        char desc[128];
        snprintf(desc, sizeof(desc), "Purged %d resolved maintenance records from memory", cnt);
        server_log_activity("Maintenance", desc, "Success");

        cJSON *data = cJSON_CreateObject();
        cJSON_AddNumberToObject(data, "deleted_count", cnt);
        send_success(c, data, "Resolved maintenance records deleted from Linked List");
        return;
    }

    int mid = atoi(id_str);
    if (mid > 0) {
        if (list_delete(&g_maintenance_list, mid) != 0) {
            send_error(c, 404, "Maintenance record not found");
            return;
        }
        storage_save_all(&g_areas, &g_complaint_queue, &g_maintenance_list, &g_pipeline_graph, &g_dist_manager);
        send_success(c, NULL, "Maintenance record deleted");
        return;
    }

    send_error(c, 400, "Missing ID or delete_resolved parameter");
}

/* GET /api/network */
static void handle_get_network(struct mg_connection *c) {
    cJSON *nodes = cJSON_CreateArray();
    cJSON *edges = cJSON_CreateArray();

    for (int i = 0; i < g_pipeline_graph.vertex_count; i++) {
        Vertex *v = &g_pipeline_graph.vertices[i];
        cJSON *n = cJSON_CreateObject();
        cJSON_AddNumberToObject(n, "id", v->id);
        cJSON_AddStringToObject(n, "name", v->name);
        cJSON_AddStringToObject(n, "type", v->type);
        cJSON_AddNumberToObject(n, "x", v->pos_x);
        cJSON_AddNumberToObject(n, "y", v->pos_y);
        cJSON_AddItemToArray(nodes, n);

        /* Collect unique edges */
        EdgeNode *curr = v->head;
        while (curr) {
            if (v->id < curr->to_vertex) { /* Avoid double-counting undirected pairs */
                cJSON *e = cJSON_CreateObject();
                cJSON_AddNumberToObject(e, "edge_id", curr->edge_id);
                cJSON_AddNumberToObject(e, "source", v->id);
                cJSON_AddNumberToObject(e, "target", curr->to_vertex);
                cJSON_AddNumberToObject(e, "distance_m", curr->distance_m);
                cJSON_AddNumberToObject(e, "loss_rate", curr->loss_rate);
                cJSON_AddBoolToObject(e, "is_active", curr->is_active);
                cJSON_AddItemToArray(edges, e);
            }
            curr = curr->next;
        }
    }

    cJSON *data = cJSON_CreateObject();
    cJSON_AddItemToObject(data, "nodes", nodes);
    cJSON_AddItemToObject(data, "edges", edges);
    cJSON_AddStringToObject(data, "model_note", "Topological network model; connectivity analysis (BFS/DFS), not hydraulic head calculation.");
    send_success(c, data, "Pipeline network topology retrieved");
}

/* PATCH /api/network/pipelines/toggle */
static void handle_patch_pipeline_toggle(struct mg_connection *c, struct mg_http_message *hm) {
    cJSON *req = cJSON_ParseWithLength(hm->body.buf, hm->body.len);
    if (!req) {
        send_error(c, 400, "Invalid JSON payload");
        return;
    }

    cJSON *edge_id = cJSON_GetObjectItem(req, "edge_id");
    cJSON *is_active = cJSON_GetObjectItem(req, "is_active");

    if (!edge_id || !is_active) {
        cJSON_Delete(req);
        send_error(c, 400, "Missing edge_id or is_active");
        return;
    }

    int res = graph_set_edge_status(&g_pipeline_graph, edge_id->valueint, cJSON_IsTrue(is_active));
    cJSON_Delete(req);

    if (res != 0) {
        send_error(c, 404, "Pipeline edge ID not found");
        return;
    }

    char desc[128];
    snprintf(desc, sizeof(desc), "Pipeline #%d status changed to %s", edge_id->valueint, cJSON_IsTrue(is_active) ? "ACTIVE" : "DISABLED");
    server_log_activity("Network", desc, cJSON_IsTrue(is_active) ? "Success" : "Warning");

    send_success(c, NULL, "Pipeline status toggled");
}

/* POST /api/network/bfs */
static void handle_post_network_bfs(struct mg_connection *c, struct mg_http_message *hm) {
    cJSON *req = cJSON_ParseWithLength(hm->body.buf, hm->body.len);
    if (!req) {
        send_error(c, 400, "Invalid JSON payload");
        return;
    }

    cJSON *start = cJSON_GetObjectItem(req, "start_id");
    cJSON *target = cJSON_GetObjectItem(req, "target_id");

    if (!start || !target) {
        cJSON_Delete(req);
        send_error(c, 400, "Missing start_id or target_id");
        return;
    }

    BFSResult bfs = graph_bfs(&g_pipeline_graph, start->valueint, target->valueint);
    cJSON_Delete(req);

    cJSON *data = cJSON_CreateObject();
    cJSON_AddBoolToObject(data, "reachable", bfs.reachable);
    cJSON_AddNumberToObject(data, "total_distance_m", bfs.total_distance);
    cJSON_AddNumberToObject(data, "hop_count", bfs.path_length > 0 ? bfs.path_length - 1 : 0);

    cJSON *path_arr = cJSON_CreateArray();
    for (int i = 0; i < bfs.path_length; i++) {
        cJSON_AddItemToArray(path_arr, cJSON_CreateNumber(bfs.path[i]));
    }
    cJSON_AddItemToObject(data, "path", path_arr);

    send_success(c, data, "BFS shortest route executed in C");
}

/* POST /api/network/dfs */
static void handle_post_network_dfs(struct mg_connection *c, struct mg_http_message *hm) {
    cJSON *req = cJSON_ParseWithLength(hm->body.buf, hm->body.len);
    if (!req) {
        send_error(c, 400, "Invalid JSON payload");
        return;
    }

    cJSON *start = cJSON_GetObjectItem(req, "start_id");
    if (!start) {
        cJSON_Delete(req);
        send_error(c, 400, "Missing start_id");
        return;
    }

    DFSResult dfs = graph_dfs(&g_pipeline_graph, start->valueint);
    cJSON_Delete(req);

    cJSON *data = cJSON_CreateObject();
    cJSON_AddNumberToObject(data, "visited_count", dfs.visited_count);

    cJSON *arr = cJSON_CreateArray();
    for (int i = 0; i < dfs.visited_count; i++) {
        cJSON_AddItemToArray(arr, cJSON_CreateNumber(dfs.visited[i]));
    }
    cJSON_AddItemToObject(data, "visited_nodes", arr);

    send_success(c, data, "DFS reachable component traversal executed in C");
}

/* POST /api/network/failure-simulation */
static void handle_post_network_failure(struct mg_connection *c, struct mg_http_message *hm) {
    cJSON *req = cJSON_ParseWithLength(hm->body.buf, hm->body.len);
    if (!req) {
        send_error(c, 400, "Invalid JSON payload");
        return;
    }

    cJSON *edge = cJSON_GetObjectItem(req, "disabled_edge_id");
    cJSON *res = cJSON_GetObjectItem(req, "reservoir_id");

    if (!edge || !res) {
        cJSON_Delete(req);
        send_error(c, 400, "Missing disabled_edge_id or reservoir_id");
        return;
    }

    FailureSimulationResult sim = graph_simulate_pipeline_failure(&g_pipeline_graph, edge->valueint, res->valueint);
    cJSON_Delete(req);

    cJSON *data = cJSON_CreateObject();
    cJSON_AddNumberToObject(data, "reachable_count", sim.reachable_count);
    cJSON_AddNumberToObject(data, "unreachable_count", sim.unreachable_count);

    cJSON *reach_arr = cJSON_CreateArray();
    for (int i = 0; i < sim.reachable_count; i++) {
        cJSON_AddItemToArray(reach_arr, cJSON_CreateNumber(sim.reachable_ids[i]));
    }
    cJSON_AddItemToObject(data, "reachable_ids", reach_arr);

    cJSON *unreach_arr = cJSON_CreateArray();
    for (int i = 0; i < sim.unreachable_count; i++) {
        cJSON_AddItemToArray(unreach_arr, cJSON_CreateNumber(sim.unreachable_ids[i]));
    }
    cJSON_AddItemToObject(data, "unreachable_ids", unreach_arr);

    char act_desc[128];
    snprintf(act_desc, sizeof(act_desc), "Simulated failure on pipeline #%d: %d areas isolated", edge->valueint, sim.unreachable_count);
    server_log_activity("Network", act_desc, "Warning");

    send_success(c, data, "Pipeline failure simulation completed");
}

/* POST /api/simulation/leakage */
static void handle_post_leakage_sim(struct mg_connection *c, struct mg_http_message *hm) {
    cJSON *req = cJSON_ParseWithLength(hm->body.buf, hm->body.len);
    if (!req) {
        send_error(c, 400, "Invalid JSON payload");
        return;
    }

    cJSON *pipe_id = cJSON_GetObjectItem(req, "pipeline_id");
    cJSON *loss = cJSON_GetObjectItem(req, "loss_litres");

    if (!pipe_id || !loss || loss->valuedouble <= 0.0) {
        cJSON_Delete(req);
        send_error(c, 400, "Invalid pipeline_id or loss_litres");
        return;
    }

    int res = dist_simulate_leakage(&g_dist_manager, pipe_id->valueint, loss->valuedouble, &g_complaint_queue, &g_pipeline_graph);
    cJSON_Delete(req);

    if (res != 0) {
        send_error(c, 500, "Failed to apply leakage simulation");
        return;
    }

    storage_save_all(&g_areas, &g_complaint_queue, &g_maintenance_list, &g_pipeline_graph, &g_dist_manager);
    char act_desc[128];
    snprintf(act_desc, sizeof(act_desc), "Simulated %.0f L leakage on pipe #%d; Auto-created Queue Complaint", loss->valuedouble, pipe_id->valueint);
    server_log_activity("Simulation", act_desc, "Warning");

    cJSON *data = cJSON_CreateObject();
    cJSON_AddNumberToObject(data, "loss_litres", loss->valuedouble);
    cJSON_AddNumberToObject(data, "pipeline_id", pipe_id->valueint);
    send_success(c, data, "Simulated water loss deducted & high-priority complaint queued");
}

/* POST /api/system/reset */
static void handle_post_system_reset(struct mg_connection *c) {
    storage_reset_demo_data(&g_areas, &g_complaint_queue, &g_maintenance_list, &g_pipeline_graph, &g_dist_manager);
    g_activity_count = 0;
    server_log_activity("System", "Demonstration dataset restored to factory defaults", "Success");
    send_success(c, NULL, "System reset to demo defaults successfully");
}

/* --- MONGOOSE HTTP EVENT HANDLER --- */
static void fn(struct mg_connection *c, int ev, void *ev_data) {
    if (ev == MG_EV_HTTP_MSG) {
        struct mg_http_message *hm = (struct mg_http_message *) ev_data;

        /* Handle CORS Preflight OPTIONS */
        if (mg_strcmp(hm->method, mg_str("OPTIONS")) == 0) {
            mg_http_reply(c, 204,
                          "Access-Control-Allow-Origin: *\r\n"
                          "Access-Control-Allow-Methods: GET, POST, PATCH, DELETE, OPTIONS\r\n"
                          "Access-Control-Allow-Headers: Content-Type\r\n",
                          "");
            return;
        }

#define MATCH_URI(pattern) mg_match(hm->uri, mg_str(pattern), NULL)

        /* Route Dispatch */
        if (MATCH_URI("/api/dashboard")) {
            handle_get_dashboard(c);
        } else if (MATCH_URI("/api/tanks/add-water") && mg_strcmp(hm->method, mg_str("POST")) == 0) {
            handle_post_tanks_add_water(c, hm);
        } else if (MATCH_URI("/api/tanks")) {
            if (mg_strcmp(hm->method, mg_str("GET")) == 0) handle_get_tanks(c);
            else if (mg_strcmp(hm->method, mg_str("POST")) == 0) handle_post_tanks(c, hm);
            else if (mg_strcmp(hm->method, mg_str("DELETE")) == 0) handle_delete_tanks(c, hm);
            else send_error(c, 405, "Method Not Allowed");
        } else if (MATCH_URI("/api/areas/search")) {
            handle_search_area(c, hm);
        } else if (MATCH_URI("/api/areas")) {
            if (mg_strcmp(hm->method, mg_str("GET")) == 0) handle_get_areas(c);
            else if (mg_strcmp(hm->method, mg_str("POST")) == 0) handle_post_areas(c, hm);
            else if (mg_strcmp(hm->method, mg_str("PATCH")) == 0) handle_patch_areas(c, hm);
            else send_error(c, 405, "Method Not Allowed");
        } else if (MATCH_URI("/api/distribution/manual") && mg_strcmp(hm->method, mg_str("POST")) == 0) {
            handle_post_dist_manual(c, hm);
        } else if (MATCH_URI("/api/distribution/automatic") && mg_strcmp(hm->method, mg_str("POST")) == 0) {
            handle_post_dist_automatic(c);
        } else if (MATCH_URI("/api/distribution")) {
            handle_get_distribution(c);
        } else if (MATCH_URI("/api/complaints/process-next") && mg_strcmp(hm->method, mg_str("POST")) == 0) {
            handle_post_process_complaint(c);
        } else if (MATCH_URI("/api/complaints")) {
            if (mg_strcmp(hm->method, mg_str("GET")) == 0) handle_get_complaints(c);
            else if (mg_strcmp(hm->method, mg_str("POST")) == 0) handle_post_complaints(c, hm);
            else send_error(c, 405, "Method Not Allowed");
        } else if (MATCH_URI("/api/maintenance/status") && mg_strcmp(hm->method, mg_str("PATCH")) == 0) {
            handle_patch_maintenance_status(c, hm);
        } else if (MATCH_URI("/api/maintenance")) {
            if (mg_strcmp(hm->method, mg_str("GET")) == 0) handle_get_maintenance(c, hm);
            else if (mg_strcmp(hm->method, mg_str("POST")) == 0) handle_post_maintenance(c, hm);
            else if (mg_strcmp(hm->method, mg_str("DELETE")) == 0) handle_delete_maintenance(c, hm);
            else send_error(c, 405, "Method Not Allowed");
        } else if (MATCH_URI("/api/network/pipelines/toggle") && mg_strcmp(hm->method, mg_str("PATCH")) == 0) {
            handle_patch_pipeline_toggle(c, hm);
        } else if (MATCH_URI("/api/network/bfs") && mg_strcmp(hm->method, mg_str("POST")) == 0) {
            handle_post_network_bfs(c, hm);
        } else if (MATCH_URI("/api/network/dfs") && mg_strcmp(hm->method, mg_str("POST")) == 0) {
            handle_post_network_dfs(c, hm);
        } else if (MATCH_URI("/api/network/failure-simulation") && mg_strcmp(hm->method, mg_str("POST")) == 0) {
            handle_post_network_failure(c, hm);
        } else if (MATCH_URI("/api/network")) {
            handle_get_network(c);
        } else if (MATCH_URI("/api/simulation/leakage") && mg_strcmp(hm->method, mg_str("POST")) == 0) {
            handle_post_leakage_sim(c, hm);
        } else if (MATCH_URI("/api/system/reset") && mg_strcmp(hm->method, mg_str("POST")) == 0) {
            handle_post_system_reset(c);
        } else {
            /* Serve static files from frontend/ directory */
            struct mg_http_serve_opts opts = {
                .root_dir = WEB_ROOT,
                .ssi_pattern = NULL
            };
            mg_http_serve_dir(c, hm, &opts);
        }
    }
}

void server_start(const char *port) {
    /* Initialize storage and load records */
    storage_init(&g_areas, &g_complaint_queue, &g_maintenance_list, &g_pipeline_graph, &g_dist_manager);
    server_log_activity("System", "Smart Water Distribution System initialized", "Success");

    struct mg_mgr mgr;
    mg_mgr_init(&mgr);

    char url[64];
    snprintf(url, sizeof(url), "http://0.0.0.0:%s", port ? port : DEFAULT_PORT);

    printf("\n=================================================================\n");
    printf(" Smart Water Distribution and Management System - C Server\n");
    printf(" UN SDG 6: Clean Water and Sanitation Data Structures Mini-Project\n");
    printf(" Server listening at: http://localhost:%s\n", port ? port : DEFAULT_PORT);
    printf(" Press Ctrl+C to safely shut down server.\n");
    printf("=================================================================\n\n");

    struct mg_connection *c = mg_http_listen(&mgr, url, fn, &mgr);
    if (!c) {
        fprintf(stderr, "ERROR: Failed to bind server to port %s\n", port ? port : DEFAULT_PORT);
        return;
    }

    while (g_server_running) {
        mg_mgr_poll(&mgr, 100);
    }

    mg_mgr_free(&mgr);

    /* Free all dynamically allocated memory */
    bst_free_all(&g_areas);
    list_free_all(&g_maintenance_list);
    graph_free_all(&g_pipeline_graph);

    printf("\nServer shutdown complete. Dynamically allocated memory freed.\n");
}

void server_stop(void) {
    g_server_running = 0;
}
