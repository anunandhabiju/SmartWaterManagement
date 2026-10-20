/**
 * @file test_all.c
 * @brief Comprehensive Unit Test Suite for All 5 Data Structures & Persistence
 *
 * Verifies:
 * 1. Arrays (Tanks & capacity/bounds checking)
 * 2. Circular Queue (FIFO order, overflow & underflow checks)
 * 3. Singly Linked List (Dynamic allocation, status updates, resolved deletion)
 * 4. Binary Search Tree (Insert, duplicate check, O(log n) search, inorder sorting)
 * 5. Adjacency List Graph (BFS pathfinding, DFS exploration, pipeline cut simulation)
 * 6. File Storage persistence & data integrity
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "../backend/tank.h"
#include "../backend/queue.h"
#include "../backend/linkedlist.h"
#include "../backend/bst.h"
#include "../backend/graph.h"
#include "../backend/storage.h"
#include "../backend/distribution.h"

static int g_tests_passed = 0;
static int g_tests_failed = 0;

#define TEST_ASSERT(cond, msg) do { \
    if (cond) { \
        g_tests_passed++; \
        printf("  [PASS] %s\n", msg); \
    } else { \
        g_tests_failed++; \
        printf("  [FAIL] %s (Line %d)\n", msg, __LINE__); \
    } \
} while (0)

void test_arrays_tanks(void) {
    printf("\n=== Running Test Suite 1: Arrays (Tank Management) ===\n");
    tank_init();

    int id1 = tank_add("Reservoir Alpha", 100000.0, 50000.0, "North");
    TEST_ASSERT(id1 > 0, "Tank 1 added with valid ID");
    int id2 = tank_add("Reservoir Beta", 50000.0, 40000.0, "South");
    TEST_ASSERT(id2 > 0, "Tank 2 added with valid ID");
    TEST_ASSERT(tank_get_count() == 2, "Tank count equals 2");

    Tank *t1 = tank_get_by_id(id1);
    TEST_ASSERT(t1 != NULL && strcmp(t1->name, "Reservoir Alpha") == 0, "Retrieve Tank 1 by ID");
    TEST_ASSERT(strcmp(t1->status, "Normal") == 0, "Tank status is Normal at 50% capacity");

    /* Add water */
    int res = tank_update_water(id1, 20000.0);
    TEST_ASSERT(res == 0 && t1->current_level == 70000.0, "Added 20,000 L to Tank 1");

    /* Exceed capacity check */
    res = tank_update_water(id1, 50000.0);
    TEST_ASSERT(res == -2, "Rejected water addition exceeding maximum capacity");
    TEST_ASSERT(t1->current_level == 70000.0, "Tank level unchanged after rejected overflow");

    /* Negative water level check */
    res = tank_update_water(id1, -80000.0);
    TEST_ASSERT(res == -3, "Rejected water deduction that would cause negative level");

    /* Deduct valid amount */
    res = tank_update_water(id1, -60000.0);
    TEST_ASSERT(res == 0 && t1->current_level == 10000.0, "Deducted water successfully");
    TEST_ASSERT(strcmp(t1->status, "Low") == 0, "Tank status correctly transitioned to Low (< 25%)");

    /* Empty tank test */
    tank_update_water(id1, -10000.0);
    TEST_ASSERT(strcmp(t1->status, "Empty") == 0, "Tank status correctly transitioned to Empty (0 L)");

    /* Aggregate capacity & available water */
    double total_cap = 0.0, total_avail = 0.0;
    tank_get_totals(&total_cap, &total_avail);
    TEST_ASSERT(total_cap == 150000.0, "Aggregate capacity computed correctly (150,000 L)");
    TEST_ASSERT(total_avail == 40000.0, "Aggregate available water computed correctly (40,000 L)");
}

void test_circular_queue(void) {
    printf("\n=== Running Test Suite 2: Circular Queue (Complaints) ===\n");
    ComplaintQueue q;
    queue_init(&q);

    TEST_ASSERT(queue_is_empty(&q) == 1, "New queue is empty");
    TEST_ASSERT(queue_is_full(&q) == 0, "New queue is not full");

    Complaint c1 = {.id = 1, .area_id = 10, .area_name = "Area A", .issue_type = "Leakage", .priority = "High"};
    Complaint c2 = {.id = 2, .area_id = 20, .area_name = "Area B", .issue_type = "Low Pressure", .priority = "Medium"};
    Complaint c3 = {.id = 3, .area_id = 30, .area_name = "Area C", .issue_type = "No Water Supply", .priority = "High"};

    TEST_ASSERT(queue_enqueue(&q, &c1) == 0, "Enqueued complaint 1");
    TEST_ASSERT(queue_enqueue(&q, &c2) == 0, "Enqueued complaint 2");
    TEST_ASSERT(queue_enqueue(&q, &c3) == 0, "Enqueued complaint 3");
    TEST_ASSERT(queue_get_count(&q) == 3, "Queue count is 3");

    /* Peek test */
    Complaint peeked;
    TEST_ASSERT(queue_peek(&q, &peeked) == 0 && peeked.id == 1, "Peek returned complaint 1 without removal");
    TEST_ASSERT(queue_get_count(&q) == 3, "Count unchanged after peek");

    /* FIFO Dequeue test */
    Complaint out1;
    TEST_ASSERT(queue_dequeue(&q, &out1) == 0 && out1.id == 1, "First dequeued is Complaint 1 (FIFO)");
    Complaint out2;
    TEST_ASSERT(queue_dequeue(&q, &out2) == 0 && out2.id == 2, "Second dequeued is Complaint 2 (FIFO)");
    TEST_ASSERT(queue_get_count(&q) == 1, "Count is now 1");

    /* Search by ID */
    Complaint found;
    TEST_ASSERT(queue_search_by_id(&q, 3, &found) == 0 && found.id == 3, "Found Complaint 3 by ID");
    TEST_ASSERT(queue_search_by_id(&q, 999, &found) == -1, "Searching non-existent ID safely returns -1");

    /* Underflow test */
    Complaint out3;
    queue_dequeue(&q, &out3);
    TEST_ASSERT(queue_is_empty(&q) == 1, "Queue is empty after dequeuing all");
    TEST_ASSERT(queue_dequeue(&q, &out3) == -1, "Safe handling of queue underflow");
}

void test_linked_list(void) {
    printf("\n=== Running Test Suite 3: Singly Linked List (Maintenance) ===\n");
    MaintenanceList list;
    list_init(&list);

    TEST_ASSERT(list_get_count(&list) == 0, "New list count is 0");

    int m1 = list_insert(&list, 101, "Sector 1", "Pipe valve seal leak", "Pending", "Tech A");
    int m2 = list_insert(&list, 102, "Sector 2", "Booster pump motor replacement", "In Progress", "Tech B");
    int m3 = list_insert(&list, 0, "Reservoir North", "Sediment cleaning", "Resolved", "Tech C");
    (void)m3;

    TEST_ASSERT(list_get_count(&list) == 3, "List count is 3 after insertions");

    /* Search by ID */
    MaintenanceNode *node = list_search(&list, m2);
    TEST_ASSERT(node != NULL && strcmp(node->issue, "Booster pump motor replacement") == 0, "Search by maintenance ID succeeded");

    /* Update status */
    int up_res = list_update_status(&list, m1, "Resolved", "Tech Alex");
    TEST_ASSERT(up_res == 0, "Updated maintenance status to Resolved");
    MaintenanceNode *m1_node = list_search(&list, m1);
    TEST_ASSERT(strcmp(m1_node->status, "Resolved") == 0 && strlen(m1_node->resolved_at) > 0, "Resolution timestamp recorded");

    /* Delete resolved records */
    int deleted = list_delete_resolved(&list);
    TEST_ASSERT(deleted == 2, "Deleted 2 resolved records");
    TEST_ASSERT(list_get_count(&list) == 1, "Remaining count is 1");
    TEST_ASSERT(list_search(&list, m1) == NULL, "Resolved record m1 no longer exists");
    TEST_ASSERT(list_search(&list, m2) != NULL, "Unresolved record m2 still exists");

    /* Delete specific ID */
    TEST_ASSERT(list_delete(&list, m2) == 0, "Deleted remaining record m2");
    TEST_ASSERT(list_get_count(&list) == 0, "List is now empty");
    TEST_ASSERT(list_delete(&list, 999) == -1, "Deleting non-existent ID safely returns -1");

    list_free_all(&list);
}

void test_bst_areas(void) {
    printf("\n=== Running Test Suite 4: Binary Search Tree (Areas) ===\n");
    AreaBST tree;
    bst_init(&tree);

    /* Insert areas out of order */
    TEST_ASSERT(bst_insert(&tree, 50, "Central District", 100000.0, 2, "Commercial") == 0, "Inserted root ID 50");
    TEST_ASSERT(bst_insert(&tree, 20, "District Hospital", 150000.0, 1, "Hospital") == 0, "Inserted left child ID 20");
    TEST_ASSERT(bst_insert(&tree, 70, "East Colony", 80000.0, 3, "Residential") == 0, "Inserted right child ID 70");
    TEST_ASSERT(bst_insert(&tree, 10, "Clinic West", 50000.0, 1, "Hospital") == 0, "Inserted ID 10");
    TEST_ASSERT(bst_insert(&tree, 30, "Tech Park", 120000.0, 2, "Commercial") == 0, "Inserted ID 30");

    /* Duplicate rejection test */
    int dup_res = bst_insert(&tree, 20, "Duplicate Hospital", 99999.0, 1, "Hospital");
    TEST_ASSERT(dup_res == -1, "Duplicate area ID correctly rejected by BST");

    /* Search test O(log n) */
    AreaNode *s = bst_search(&tree, 30);
    TEST_ASSERT(s != NULL && strcmp(s->name, "Tech Park") == 0, "BST search located node ID 30");
    TEST_ASSERT(bst_search(&tree, 999) == NULL, "BST search returns NULL for missing ID");

    /* Inorder traversal verification (must be sorted ascending) */
    AreaNode sorted[10];
    int count = bst_inorder_to_array(&tree, sorted, 10);
    TEST_ASSERT(count == 5, "Inorder traversal returned all 5 areas");
    int is_sorted = 1;
    for (int i = 0; i < count - 1; i++) {
        if (sorted[i].id >= sorted[i + 1].id) {
            is_sorted = 0;
            break;
        }
    }
    TEST_ASSERT(is_sorted == 1, "Inorder traversal produced strictly ascending Area IDs: 10, 20, 30, 50, 70");

    /* Supply and Unmet Demand checks */
    bst_add_supply(&tree, 20, 100000.0);
    AreaNode *hosp = bst_search(&tree, 20);
    TEST_ASSERT(hosp->supplied == 100000.0, "Supplied 100,000 L to hospital");

    double total_demand = 0.0, total_supplied = 0.0, unmet = 0.0;
    bst_get_totals(&tree, &total_demand, &total_supplied, &unmet);
    TEST_ASSERT(total_demand == 500000.0, "Total demand calculated correctly (500,000 L)");
    TEST_ASSERT(total_supplied == 100000.0, "Total supplied calculated correctly (100,000 L)");
    TEST_ASSERT(unmet == 400000.0, "Total unmet demand calculated correctly (400,000 L)");

    bst_free_all(&tree);
    TEST_ASSERT(tree.root == NULL && tree.count == 0, "BST recursively deallocated without leaks");
}

void test_graph_pipelines(void) {
    printf("\n=== Running Test Suite 5: Pipeline Graph (BFS & DFS) ===\n");
    PipelineGraph g;
    graph_init(&g);

    /* Build a mini-topology:
     * Reservoir(1) --[e1]-- Junction(2) --[e2]-- Hospital(3)
     *                            |--[e3]-- Residential(4)
     * Isolated Area(5)
     */
    graph_add_vertex(&g, 1, "North Reservoir", "Reservoir", 0, 0);
    graph_add_vertex(&g, 2, "Main Junction", "Junction", 1, 1);
    graph_add_vertex(&g, 3, "General Hospital", "Hospital", 2, 0);
    graph_add_vertex(&g, 4, "Residential South", "Residential", 2, 2);
    graph_add_vertex(&g, 5, "Isolated Suburb", "Residential", 4, 4);

    int e1 = graph_add_edge(&g, 1, 2, 500.0, 1.0);
    int e2 = graph_add_edge(&g, 2, 3, 300.0, 0.5);
    int e3 = graph_add_edge(&g, 2, 4, 400.0, 0.8);

    TEST_ASSERT(e1 > 0 && e2 > 0 && e3 > 0, "Pipeline edges created successfully");

    /* BFS Reachability & Shortest Path */
    BFSResult bfs_hosp = graph_bfs(&g, 1, 3);
    TEST_ASSERT(bfs_hosp.reachable == 1, "Hospital 3 is reachable from Reservoir 1 via BFS");
    TEST_ASSERT(bfs_hosp.path_length == 3, "BFS shortest path length is 3 hops (1 -> 2 -> 3)");
    TEST_ASSERT(bfs_hosp.path[0] == 1 && bfs_hosp.path[1] == 2 && bfs_hosp.path[2] == 3, "Exact BFS path matches 1 -> 2 -> 3");
    TEST_ASSERT(bfs_hosp.total_distance == 800.0, "Total pipeline distance is 800 meters");

    /* BFS to disconnected node */
    BFSResult bfs_iso = graph_bfs(&g, 1, 5);
    TEST_ASSERT(bfs_iso.reachable == 0, "Isolated node 5 is unreachable via BFS");

    /* DFS Exploration */
    DFSResult dfs = graph_dfs(&g, 1);
    TEST_ASSERT(dfs.visited_count == 4, "DFS explored all 4 connected components");

    /* Pipeline Cut / Failure Simulation */
    /* If e2 (Junction -> Hospital) breaks: */
    FailureSimulationResult sim = graph_simulate_pipeline_failure(&g, e2, 1);
    TEST_ASSERT(sim.reachable_count == 3, "3 nodes remain reachable during pipeline e2 break");
    TEST_ASSERT(sim.unreachable_count == 2, "2 nodes isolated during failure (Hospital 3 and Isolated 5)");

    int hosp_isolated = 0;
    for (int i = 0; i < sim.unreachable_count; i++) {
        if (sim.unreachable_ids[i] == 3) hosp_isolated = 1;
    }
    TEST_ASSERT(hosp_isolated == 1, "Hospital 3 accurately identified as disconnected by simulation");

    graph_free_all(&g);
}

void test_persistence(void) {
    printf("\n=== Running Test Suite 6: File Storage Persistence ===\n");
    AreaBST areas;
    memset(&areas, 0, sizeof(areas));
    ComplaintQueue queue;
    memset(&queue, 0, sizeof(queue));
    MaintenanceList list;
    memset(&list, 0, sizeof(list));
    PipelineGraph graph;
    memset(&graph, 0, sizeof(graph));
    DistributionManager dm;
    memset(&dm, 0, sizeof(dm));

    storage_reset_demo_data(&areas, &queue, &list, &graph, &dm);
    int original_tanks = tank_get_count();
    int original_areas = bst_get_count(&areas);
    int original_complaints = queue_get_count(&queue);
    int original_maintenance = list_get_count(&list);

    TEST_ASSERT(original_tanks > 0, "Default tanks initialized");
    TEST_ASSERT(original_areas > 0, "Default BST areas initialized");
    TEST_ASSERT(original_complaints > 0, "Default complaints initialized");
    TEST_ASSERT(original_maintenance > 0, "Default maintenance initialized");

    /* Save to disk */
    int save_res = storage_save_all(&areas, &queue, &list, &graph, &dm);
    TEST_ASSERT(save_res == 0, "All data successfully saved to disk");

    /* Clear in-memory state */
    tank_clear_all();
    bst_free_all(&areas);
    queue_clear(&queue);
    list_free_all(&list);

    TEST_ASSERT(tank_get_count() == 0, "Tanks cleared in memory");
    TEST_ASSERT(bst_get_count(&areas) == 0, "Areas cleared in memory");
    TEST_ASSERT(queue_get_count(&queue) == 0, "Queue cleared in memory");

    /* Reload from disk */
    int load_res = storage_load_all(&areas, &queue, &list, &graph, &dm);
    TEST_ASSERT(load_res == 0, "All data reloaded from disk");

    TEST_ASSERT(tank_get_count() == original_tanks, "Reloaded tank count matches");
    TEST_ASSERT(bst_get_count(&areas) == original_areas, "Reloaded BST area count matches");
    TEST_ASSERT(queue_get_count(&queue) == original_complaints, "Reloaded complaint queue count matches");
    TEST_ASSERT(list_get_count(&list) == original_maintenance, "Reloaded maintenance list count matches");

    bst_free_all(&areas);
    list_free_all(&list);
    graph_free_all(&graph);
}

int main(void) {
    setvbuf(stdout, NULL, _IONBF, 0);
    setvbuf(stderr, NULL, _IONBF, 0);
    printf("=================================================================\n");
    printf(" Smart Water Distribution System - Automated Test Suite\n");
    printf(" UN SDG 6: Clean Water and Sanitation Data Structures Mini-Project\n");
    printf("=================================================================\n");

    test_arrays_tanks();
    test_circular_queue();
    test_linked_list();
    test_bst_areas();
    test_graph_pipelines();
    test_persistence();

    printf("\n=================================================================\n");
    printf(" TEST SUITE SUMMARY: %d PASSED, %d FAILED\n", g_tests_passed, g_tests_failed);
    printf("=================================================================\n");

    return (g_tests_failed == 0) ? 0 : 1;
}
