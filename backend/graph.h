/**
 * @file graph.h
 * @brief Pipeline Network using Adjacency List Graph (Data Structure 5)
 *
 * Implements a directed/undirected pipeline distribution graph using adjacency
 * lists in C. Includes Breadth-First Search (BFS) for shortest route reachability,
 * Depth-First Search (DFS) for component traversal, and pipeline failure simulation.
 */

#ifndef GRAPH_H
#define GRAPH_H

#define MAX_GRAPH_VERTICES 30
#define MAX_GRAPH_EDGES 100

typedef struct EdgeNode {
    int edge_id;
    int to_vertex;          /* Destination vertex ID */
    double distance_m;      /* Pipe length in meters */
    double loss_rate;       /* Estimated leakage loss percentage (e.g. 2.5%) */
    int is_active;          /* 1 = operational, 0 = broken / isolated */
    struct EdgeNode *next;
} EdgeNode;

typedef struct {
    int id;
    char name[64];
    char type[32];          /* "Reservoir", "Junction", "Hospital", "Commercial", "Residential" */
    double pos_x;           /* Coordinate layout hint */
    double pos_y;
    EdgeNode *head;
} Vertex;

typedef struct {
    Vertex vertices[MAX_GRAPH_VERTICES];
    int vertex_count;
    int next_edge_id;
} PipelineGraph;

typedef struct {
    int path[MAX_GRAPH_VERTICES];
    int path_length;
    double total_distance;
    int reachable;
} BFSResult;

typedef struct {
    int visited[MAX_GRAPH_VERTICES];
    int visited_count;
} DFSResult;

typedef struct {
    int reachable_ids[MAX_GRAPH_VERTICES];
    int reachable_count;
    int unreachable_ids[MAX_GRAPH_VERTICES];
    int unreachable_count;
} FailureSimulationResult;

/* Initializes pipeline network graph */
void graph_init(PipelineGraph *g);

/* Adds a vertex. Returns 0 on success, -1 on overflow or duplicate. */
int graph_add_vertex(PipelineGraph *g, int id, const char *name, const char *type, double x, double y);

/* Finds vertex index by ID. Returns array index (0..vertex_count-1) or -1 if not found. */
int graph_find_vertex_idx(const PipelineGraph *g, int id);

/* Adds a bidirectional pipeline connection (edge). Returns edge ID or -1 on error. */
int graph_add_edge(PipelineGraph *g, int from_id, int to_id, double distance_m, double loss_rate);

/* Sets operational status (1=active, 0=disabled) for an edge by ID */
int graph_set_edge_status(PipelineGraph *g, int edge_id, int is_active);

/* Removes an edge completely by ID. Returns 0 on success, -1 if not found. */
int graph_remove_edge(PipelineGraph *g, int edge_id);

/* Breadth-First Search (BFS) to find reachability and shortest hop route from source to destination */
BFSResult graph_bfs(const PipelineGraph *g, int start_id, int target_id);

/* Depth-First Search (DFS) to explore all reachable nodes from a given source */
DFSResult graph_dfs(const PipelineGraph *g, int start_id);

/* Simulates pipeline failure by temporarily disabling edge_id and checking reachability from reservoir_id */
FailureSimulationResult graph_simulate_pipeline_failure(PipelineGraph *g, int disabled_edge_id, int reservoir_id);

/* Deallocates all dynamic edge nodes in the graph */
void graph_free_all(PipelineGraph *g);

/* Populates default demonstration network (Reservoirs, Junctions, Consumers) */
void graph_load_defaults(PipelineGraph *g);

#endif /* GRAPH_H */
