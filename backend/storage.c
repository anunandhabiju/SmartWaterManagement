/**
 * @file storage.c
 * @brief File Persistence implementation using standard C file I/O
 */

#include "storage.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include <direct.h>
#include <io.h>
#define MKDIR(d) _mkdir(d)
#define ACCESS(f, m) _access(f, m)
#else
#include <sys/stat.h>
#include <unistd.h>
#define MKDIR(d) mkdir(d, 0755)
#define ACCESS(f, m) access(f, m)
#endif

static const char *get_data_dir(void) {
    /* Check if backend/data exists, otherwise use data/ */
    if (ACCESS("backend/data", 0) == 0 || ACCESS("backend", 0) == 0) {
        MKDIR("backend");
        MKDIR("backend/data");
        return "backend/data";
    }
    MKDIR("data");
    return "data";
}

static void make_path(char *out, size_t sz, const char *filename) {
    snprintf(out, sz, "%s/%s", get_data_dir(), filename);
}

/* --- TANKS PERSISTENCE --- */
static int save_tanks(void) {
    char path[256];
    make_path(path, sizeof(path), "tanks.dat");

    FILE *fp = fopen(path, "w");
    if (!fp) return -1;

    Tank tanks[MAX_TANKS];
    int count = tank_get_all(tanks, MAX_TANKS);
    fprintf(fp, "%d\n", count);

    for (int i = 0; i < count; i++) {
        fprintf(fp, "%d|%s|%.2f|%.2f|%s|%s\n",
                tanks[i].id,
                tanks[i].name,
                tanks[i].capacity,
                tanks[i].current_level,
                tanks[i].location,
                tanks[i].status);
    }
    fclose(fp);
    return 0;
}

static int load_tanks(void) {
    char path[256];
    make_path(path, sizeof(path), "tanks.dat");

    FILE *fp = fopen(path, "r");
    if (!fp) return -1;

    int count = 0;
    if (fscanf(fp, "%d\n", &count) != 1 || count <= 0) {
        fclose(fp);
        return -1;
    }

    tank_clear_all();
    char line[512];
    for (int i = 0; i < count && fgets(line, sizeof(line), fp); i++) {
        line[strcspn(line, "\r\n")] = '\0';
        char name[64] = {0}, loc[64] = {0}, status[16] = {0};
        int id = 0;
        double cap = 0.0, level = 0.0;

        char *token = strtok(line, "|");
        if (token) id = atoi(token);
        (void)id;
        token = strtok(NULL, "|");
        if (token) strncpy(name, token, sizeof(name) - 1);
        token = strtok(NULL, "|");
        if (token) cap = atof(token);
        token = strtok(NULL, "|");
        if (token) level = atof(token);
        token = strtok(NULL, "|");
        if (token) strncpy(loc, token, sizeof(loc) - 1);
        token = strtok(NULL, "|");
        if (token) strncpy(status, token, sizeof(status) - 1);

        tank_add(name, cap, level, loc);
    }
    fclose(fp);
    return 0;
}

/* --- AREAS (BST) PERSISTENCE --- */
static int save_areas(const AreaBST *areas) {
    char path[256];
    make_path(path, sizeof(path), "areas.dat");

    FILE *fp = fopen(path, "w");
    if (!fp) return -1;

    AreaNode arr[50];
    int count = bst_inorder_to_array(areas, arr, 50);
    fprintf(fp, "%d\n", count);

    for (int i = 0; i < count; i++) {
        fprintf(fp, "%d|%s|%.2f|%.2f|%d|%s\n",
                arr[i].id,
                arr[i].name,
                arr[i].demand,
                arr[i].supplied,
                arr[i].priority,
                arr[i].category);
    }
    fclose(fp);
    return 0;
}

static int load_areas(AreaBST *areas) {
    char path[256];
    make_path(path, sizeof(path), "areas.dat");

    FILE *fp = fopen(path, "r");
    if (!fp) return -1;

    int count = 0;
    if (fscanf(fp, "%d\n", &count) != 1 || count <= 0) {
        fclose(fp);
        return -1;
    }

    bst_free_all(areas);
    bst_init(areas);

    char line[512];
    for (int i = 0; i < count && fgets(line, sizeof(line), fp); i++) {
        line[strcspn(line, "\r\n")] = '\0';
        int id = 0, priority = 3;
        char name[64] = {0}, cat[32] = {0};
        double demand = 0.0, supplied = 0.0;

        char *token = strtok(line, "|");
        if (token) id = atoi(token);
        token = strtok(NULL, "|");
        if (token) strncpy(name, token, sizeof(name) - 1);
        token = strtok(NULL, "|");
        if (token) demand = atof(token);
        token = strtok(NULL, "|");
        if (token) supplied = atof(token);
        token = strtok(NULL, "|");
        if (token) priority = atoi(token);
        token = strtok(NULL, "|");
        if (token) strncpy(cat, token, sizeof(cat) - 1);

        bst_insert(areas, id, name, demand, priority, cat);
        if (supplied > 0.0) {
            bst_add_supply(areas, id, supplied);
        }
    }
    fclose(fp);
    return 0;
}

/* --- COMPLAINTS QUEUE PERSISTENCE --- */
static int save_complaints(const ComplaintQueue *queue) {
    char path[256];
    make_path(path, sizeof(path), "complaints.dat");

    FILE *fp = fopen(path, "w");
    if (!fp) return -1;

    Complaint arr[MAX_QUEUE_CAPACITY];
    int count = queue_get_all(queue, arr, MAX_QUEUE_CAPACITY);
    fprintf(fp, "%d\n", count);

    for (int i = 0; i < count; i++) {
        fprintf(fp, "%d|%d|%s|%s|%s|%s|%s|%s\n",
                arr[i].id,
                arr[i].area_id,
                arr[i].area_name,
                arr[i].issue_type,
                arr[i].description,
                arr[i].priority,
                arr[i].timestamp,
                arr[i].status);
    }
    fclose(fp);
    return 0;
}

static int load_complaints(ComplaintQueue *queue) {
    char path[256];
    make_path(path, sizeof(path), "complaints.dat");

    FILE *fp = fopen(path, "r");
    if (!fp) return -1;

    int count = 0;
    if (fscanf(fp, "%d\n", &count) != 1) {
        fclose(fp);
        return -1;
    }

    queue_clear(queue);
    char line[1024];
    for (int i = 0; i < count && fgets(line, sizeof(line), fp); i++) {
        line[strcspn(line, "\r\n")] = '\0';
        Complaint c;
        memset(&c, 0, sizeof(c));

        char *token = strtok(line, "|");
        if (token) c.id = atoi(token);
        token = strtok(NULL, "|");
        if (token) c.area_id = atoi(token);
        token = strtok(NULL, "|");
        if (token) strncpy(c.area_name, token, sizeof(c.area_name) - 1);
        token = strtok(NULL, "|");
        if (token) strncpy(c.issue_type, token, sizeof(c.issue_type) - 1);
        token = strtok(NULL, "|");
        if (token) strncpy(c.description, token, sizeof(c.description) - 1);
        token = strtok(NULL, "|");
        if (token) strncpy(c.priority, token, sizeof(c.priority) - 1);
        token = strtok(NULL, "|");
        if (token) strncpy(c.timestamp, token, sizeof(c.timestamp) - 1);
        token = strtok(NULL, "|");
        if (token) strncpy(c.status, token, sizeof(c.status) - 1);

        queue_enqueue(queue, &c);
    }
    fclose(fp);
    return 0;
}

/* --- MAINTENANCE (LINKED LIST) PERSISTENCE --- */
static int save_maintenance(const MaintenanceList *list) {
    char path[256];
    make_path(path, sizeof(path), "maintenance.dat");

    FILE *fp = fopen(path, "w");
    if (!fp) return -1;

    MaintenanceNode arr[100];
    int count = list_to_array(list, arr, 100);
    fprintf(fp, "%d\n", count);

    for (int i = 0; i < count; i++) {
        fprintf(fp, "%d|%d|%s|%s|%s|%s|%s|%s\n",
                arr[i].id,
                arr[i].complaint_id,
                arr[i].area_name,
                arr[i].issue,
                arr[i].status,
                arr[i].technician,
                arr[i].created_at,
                arr[i].resolved_at);
    }
    fclose(fp);
    return 0;
}

static int load_maintenance(MaintenanceList *list) {
    char path[256];
    make_path(path, sizeof(path), "maintenance.dat");

    FILE *fp = fopen(path, "r");
    if (!fp) return -1;

    int count = 0;
    if (fscanf(fp, "%d\n", &count) != 1) {
        fclose(fp);
        return -1;
    }

    list_free_all(list);
    list_init(list);

    char line[1024];
    for (int i = 0; i < count && fgets(line, sizeof(line), fp); i++) {
        line[strcspn(line, "\r\n")] = '\0';
        int id = 0, comp_id = 0;
        char area[64] = {0}, issue[256] = {0}, status[32] = {0}, tech[64] = {0}, cat[32] = {0}, resat[32] = {0};

        char *token = strtok(line, "|");
        if (token) id = atoi(token);
        token = strtok(NULL, "|");
        if (token) comp_id = atoi(token);
        token = strtok(NULL, "|");
        if (token) strncpy(area, token, sizeof(area) - 1);
        token = strtok(NULL, "|");
        if (token) strncpy(issue, token, sizeof(issue) - 1);
        token = strtok(NULL, "|");
        if (token) strncpy(status, token, sizeof(status) - 1);
        token = strtok(NULL, "|");
        if (token) strncpy(tech, token, sizeof(tech) - 1);
        token = strtok(NULL, "|");
        if (token) strncpy(cat, token, sizeof(cat) - 1);
        token = strtok(NULL, "|");
        if (token) strncpy(resat, token, sizeof(resat) - 1);

        int assigned_id = list_insert(list, comp_id, area, issue, status, tech);
        MaintenanceNode *node = list_search(list, assigned_id);
        if (node) {
            node->id = id;
            if (id >= list->next_id) list->next_id = id + 1;
            strncpy(node->created_at, cat, sizeof(node->created_at) - 1);
            strncpy(node->resolved_at, resat, sizeof(node->resolved_at) - 1);
        }
    }
    fclose(fp);
    return 0;
}

/* --- ALL SAVE & LOAD --- */
int storage_save_all(const AreaBST *areas, const ComplaintQueue *queue,
                     const MaintenanceList *list, const PipelineGraph *graph,
                     const DistributionManager *dm) {
    (void)graph;
    (void)dm;
    int res = 0;
    if (save_tanks() != 0) res = -1;
    if (save_areas(areas) != 0) res = -1;
    if (save_complaints(queue) != 0) res = -1;
    if (save_maintenance(list) != 0) res = -1;
    return res;
}

int storage_load_all(AreaBST *areas, ComplaintQueue *queue,
                     MaintenanceList *list, PipelineGraph *graph,
                     DistributionManager *dm) {
    (void)graph;
    (void)dm;
    int t_res = load_tanks();
    int a_res = load_areas(areas);
    int c_res = load_complaints(queue);
    int m_res = load_maintenance(list);

    if (t_res != 0 || a_res != 0 || c_res != 0 || m_res != 0) {
        return -1; /* Some files were missing */
    }
    return 0;
}

void storage_reset_demo_data(AreaBST *areas, ComplaintQueue *queue,
                             MaintenanceList *list, PipelineGraph *graph,
                             DistributionManager *dm) {
    tank_load_defaults();
    bst_load_defaults(areas);
    queue_load_defaults(queue);
    list_load_defaults(list);
    graph_load_defaults(graph);
    dist_init(dm);

    storage_save_all(areas, queue, list, graph, dm);
}

void storage_init(AreaBST *areas, ComplaintQueue *queue, MaintenanceList *list,
                  PipelineGraph *graph, DistributionManager *dm) {
    if (storage_load_all(areas, queue, list, graph, dm) != 0) {
        /* File missing or corrupted -> populate default demo data */
        storage_reset_demo_data(areas, queue, list, graph, dm);
    } else {
        graph_load_defaults(graph);
        dist_init(dm);
    }
}
