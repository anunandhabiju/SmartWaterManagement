/**
 * @file graph.c
 * @brief Pipeline Network implementation using Adjacency List Graph
 */

#include "graph.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void graph_init(PipelineGraph *g) {
    if (!g) return;
    g->vertex_count = 0;
    g->next_edge_id = 1;
    for (int i = 0; i < MAX_GRAPH_VERTICES; i++) {
        g->vertices[i].id = 0;
        g->vertices[i].name[0] = '\0';
        g->vertices[i].type[0] = '\0';
        g->vertices[i].pos_x = 0.0;
        g->vertices[i].pos_y = 0.0;
        g->vertices[i].head = NULL;
    }
}

int graph_find_vertex_idx(const PipelineGraph *g, int id) {
    if (!g) return -1;
    for (int i = 0; i < g->vertex_count; i++) {
        if (g->vertices[i].id == id) {
            return i;
        }
    }
    return -1;
}

int graph_add_vertex(PipelineGraph *g, int id, const char *name, const char *type, double x, double y) {
    if (!g || g->vertex_count >= MAX_GRAPH_VERTICES) {
        return -1;
    }
    if (graph_find_vertex_idx(g, id) != -1) {
        return -1; /* Duplicate vertex ID */
    }

    int idx = g->vertex_count;
    g->vertices[idx].id = id;
    strncpy(g->vertices[idx].name, name ? name : "Node", sizeof(g->vertices[idx].name) - 1);
    g->vertices[idx].name[sizeof(g->vertices[idx].name) - 1] = '\0';

    strncpy(g->vertices[idx].type, type ? type : "Junction", sizeof(g->vertices[idx].type) - 1);
    g->vertices[idx].type[sizeof(g->vertices[idx].type) - 1] = '\0';

    g->vertices[idx].pos_x = x;
    g->vertices[idx].pos_y = y;
    g->vertices[idx].head = NULL;

    g->vertex_count++;
    return 0;
}

static void add_directed_edge(PipelineGraph *g, int from_idx, int to_id, int edge_id, double dist, double loss) {
    EdgeNode *node = (EdgeNode*)malloc(sizeof(EdgeNode));
    if (!node) return;

    node->edge_id = edge_id;
    node->to_vertex = to_id;
    node->distance_m = dist > 0.0 ? dist : 100.0;
    node->loss_rate = loss >= 0.0 ? loss : 1.0;
    node->is_active = 1;
    node->next = g->vertices[from_idx].head;
    g->vertices[from_idx].head = node;
}

int graph_add_edge(PipelineGraph *g, int from_id, int to_id, double distance_m, double loss_rate) {
    if (!g || from_id == to_id) return -1;

    int from_idx = graph_find_vertex_idx(g, from_id);
    int to_idx = graph_find_vertex_idx(g, to_id);
    if (from_idx == -1 || to_idx == -1) return -1;

    int edge_id = g->next_edge_id++;

    /* Bidirectional pipeline connection */
    add_directed_edge(g, from_idx, to_id, edge_id, distance_m, loss_rate);
    add_directed_edge(g, to_idx, from_id, edge_id, distance_m, loss_rate);

    return edge_id;
}

int graph_set_edge_status(PipelineGraph *g, int edge_id, int is_active) {
    if (!g) return -1;
    int modified = 0;

    for (int i = 0; i < g->vertex_count; i++) {
        EdgeNode *curr = g->vertices[i].head;
        while (curr) {
            if (curr->edge_id == edge_id) {
                curr->is_active = (is_active ? 1 : 0);
                modified = 1;
            }
            curr = curr->next;
        }
    }
    return modified ? 0 : -1;
}

int graph_remove_edge(PipelineGraph *g, int edge_id) {
    if (!g) return -1;
    int removed = 0;

    for (int i = 0; i < g->vertex_count; i++) {
        EdgeNode *curr = g->vertices[i].head;
        EdgeNode *prev = NULL;

        while (curr) {
            if (curr->edge_id == edge_id) {
                EdgeNode *to_free = curr;
                if (!prev) {
                    g->vertices[i].head = curr->next;
                    curr = g->vertices[i].head;
                } else {
                    prev->next = curr->next;
                    curr = curr->next;
                }
                free(to_free);
                removed = 1;
            } else {
                prev = curr;
                curr = curr->next;
            }
        }
    }
    return removed ? 0 : -1;
}

BFSResult graph_bfs(const PipelineGraph *g, int start_id, int target_id) {
    BFSResult res;
    memset(&res, 0, sizeof(res));

    int start_idx = graph_find_vertex_idx(g, start_id);
    int target_idx = graph_find_vertex_idx(g, target_id);
    if (start_idx == -1 || target_idx == -1) {
        return res;
    }

    if (start_id == target_id) {
        res.reachable = 1;
        res.path[0] = start_id;
        res.path_length = 1;
        res.total_distance = 0.0;
        return res;
    }

    int visited[MAX_GRAPH_VERTICES] = {0};
    int parent_idx[MAX_GRAPH_VERTICES];
    double edge_dist_to_parent[MAX_GRAPH_VERTICES] = {0.0};
    for (int i = 0; i < MAX_GRAPH_VERTICES; i++) {
        parent_idx[i] = -1;
    }

    int queue[MAX_GRAPH_VERTICES];
    int q_front = 0, q_rear = 0;

    queue[q_rear++] = start_idx;
    visited[start_idx] = 1;

    int found = 0;
    while (q_front < q_rear) {
        int u = queue[q_front++];
        if (u == target_idx) {
            found = 1;
            break;
        }

        EdgeNode *edge = g->vertices[u].head;
        while (edge) {
            if (edge->is_active) {
                int v_idx = graph_find_vertex_idx(g, edge->to_vertex);
                if (v_idx != -1 && !visited[v_idx]) {
                    visited[v_idx] = 1;
                    parent_idx[v_idx] = u;
                    edge_dist_to_parent[v_idx] = edge->distance_m;
                    queue[q_rear++] = v_idx;
                }
            }
            edge = edge->next;
        }
    }

    if (found) {
        res.reachable = 1;
        int temp_path[MAX_GRAPH_VERTICES];
        int count = 0;
        int curr = target_idx;
        double dist = 0.0;

        while (curr != -1) {
            temp_path[count++] = g->vertices[curr].id;
            if (parent_idx[curr] != -1) {
                dist += edge_dist_to_parent[curr];
            }
            curr = parent_idx[curr];
        }

        /* Reverse to obtain start -> target order */
        res.path_length = count;
        res.total_distance = dist;
        for (int i = 0; i < count; i++) {
            res.path[i] = temp_path[count - 1 - i];
        }
    }

    return res;
}

static void dfs_helper(const PipelineGraph *g, int u_idx, int visited[], DFSResult *res) {
    visited[u_idx] = 1;
    res->visited[res->visited_count++] = g->vertices[u_idx].id;

    EdgeNode *edge = g->vertices[u_idx].head;
    while (edge) {
        if (edge->is_active) {
            int v_idx = graph_find_vertex_idx(g, edge->to_vertex);
            if (v_idx != -1 && !visited[v_idx]) {
                dfs_helper(g, v_idx, visited, res);
            }
        }
        edge = edge->next;
    }
}

DFSResult graph_dfs(const PipelineGraph *g, int start_id) {
    DFSResult res;
    memset(&res, 0, sizeof(res));

    int start_idx = graph_find_vertex_idx(g, start_id);
    if (start_idx == -1) return res;

    int visited[MAX_GRAPH_VERTICES] = {0};
    dfs_helper(g, start_idx, visited, &res);
    return res;
}

FailureSimulationResult graph_simulate_pipeline_failure(PipelineGraph *g, int disabled_edge_id, int reservoir_id) {
    FailureSimulationResult sim;
    memset(&sim, 0, sizeof(sim));

    if (!g) return sim;

    /* Disable the edge */
    graph_set_edge_status(g, disabled_edge_id, 0);

    /* Run DFS from source reservoir */
    DFSResult dfs = graph_dfs(g, reservoir_id);

    /* Collect reachable nodes */
    for (int i = 0; i < dfs.visited_count; i++) {
        sim.reachable_ids[sim.reachable_count++] = dfs.visited[i];
    }

    /* Identify disconnected nodes */
    for (int i = 0; i < g->vertex_count; i++) {
        int v_id = g->vertices[i].id;
        int is_visited = 0;
        for (int j = 0; j < dfs.visited_count; j++) {
            if (dfs.visited[j] == v_id) {
                is_visited = 1;
                break;
            }
        }
        if (!is_visited) {
            sim.unreachable_ids[sim.unreachable_count++] = v_id;
        }
    }

    /* Restore edge */
    graph_set_edge_status(g, disabled_edge_id, 1);

    return sim;
}

void graph_free_all(PipelineGraph *g) {
    if (!g) return;
    for (int i = 0; i < g->vertex_count; i++) {
        EdgeNode *curr = g->vertices[i].head;
        while (curr) {
            EdgeNode *next = curr->next;
            free(curr);
            curr = next;
        }
        g->vertices[i].head = NULL;
    }
    g->vertex_count = 0;
}

void graph_load_defaults(PipelineGraph *g) {
    graph_free_all(g);
    graph_init(g);

    /* Add Vertices: Reservoirs, Junctions, Consumer Areas */
    graph_add_vertex(g, 1, "North Hill Reservoir", "Reservoir", 100, 100);
    graph_add_vertex(g, 2, "East River Reservoir", "Reservoir", 800, 100);
    graph_add_vertex(g, 3, "Junction J-1 (North)", "Junction", 250, 250);
    graph_add_vertex(g, 4, "Junction J-2 (Central Trunk)", "Junction", 450, 280);
    graph_add_vertex(g, 5, "Junction J-3 (East Branch)", "Junction", 650, 250);
    graph_add_vertex(g, 101, "City General Hospital", "Hospital", 200, 420);
    graph_add_vertex(g, 102, "North Residential Block", "Residential", 350, 420);
    graph_add_vertex(g, 103, "East Commercial Complex", "Commercial", 550, 420);
    graph_add_vertex(g, 104, "South Valley Residential", "Residential", 700, 420);
    graph_add_vertex(g, 105, "University Campus & Clinics", "Hospital", 450, 520);
    graph_add_vertex(g, 106, "Industrial Manufacturing Zone", "Commercial", 650, 520);

    /* Add Pipeline Edges */
    /* Res 1 -> J1 */
    graph_add_edge(g, 1, 3, 450.0, 1.2);
    /* Res 2 -> J3 */
    graph_add_edge(g, 2, 5, 520.0, 1.5);
    /* J1 <-> J2 (Central Trunk Backbone) */
    graph_add_edge(g, 3, 4, 380.0, 0.8);
    /* J2 <-> J3 */
    graph_add_edge(g, 4, 5, 410.0, 0.9);
    /* J1 -> Hospital 101 */
    graph_add_edge(g, 3, 101, 290.0, 0.5);
    /* J1 -> North Residential 102 */
    graph_add_edge(g, 3, 102, 310.0, 1.1);
    /* J2 -> East Commercial 103 */
    graph_add_edge(g, 4, 103, 260.0, 0.7);
    /* J2 -> University Clinics 105 */
    graph_add_edge(g, 4, 105, 340.0, 0.6);
    /* J3 -> South Valley 104 */
    graph_add_edge(g, 5, 104, 300.0, 1.4);
    /* J3 -> Industrial Zone 106 */
    graph_add_edge(g, 5, 106, 480.0, 2.0);
    /* Secondary redundancy: 102 <-> 103 */
    graph_add_edge(g, 102, 103, 220.0, 1.0);
}
