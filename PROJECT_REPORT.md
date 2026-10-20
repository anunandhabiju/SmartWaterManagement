# Academic Project Report: Smart Water Distribution and Management System

**Course:** Data Structures and Algorithms Mini-Project  
**Domain Alignment:** United Nations Sustainable Development Goal 6 (SDG 6: Clean Water and Sanitation)  
**Backend:** Pure C (Standard C99 / C11, Mongoose HTTP Server, cJSON)  
**Frontend:** HTML5, CSS3, JavaScript (ES6+), Chart.js, Cytoscape.js  
**Platform:** Windows 10/11 (Cross-platform POSIX compatible)  

---

## 1. Executive Summary

Water scarcity, non-revenue water (NRW) through underground leakage, and equitable allocation during supply deficits are among the most urgent municipal challenges globally. The **Smart Water Distribution and Management System** is an interactive, full-stack academic simulation that demonstrates how five core data structures in computer science—**Arrays**, **Circular Queues**, **Singly Linked Lists**, **Binary Search Trees (BST)**, and **Adjacency List Graphs**—can be synthesized to manage municipal water reservoirs, priority-based distribution networks, citizen complaints, and maintenance infrastructure.

The backend is engineered **entirely in the C programming language**, utilizing standard memory management (`malloc`/`free`), pointer arithmetic, modular headers, file I/O persistence, and an embedded event-driven HTTP server (Mongoose) speaking REST JSON (cJSON). The browser frontend delivers a real-time reactive dashboard with interactive network graphs and statistics.

---

## 2. United Nations Sustainable Development Goal 6 (SDG 6) Alignment

This project directly aligns with **UN SDG 6: Ensure availability and sustainable management of water and sanitation for all**:

1. **Target 6.1 (Equitable & Affordable Drinking Water):**
   - Simulated through **Automatic Priority Allocation**, ensuring essential healthcare services and critical public clinics receive water ahead of commercial and general residential consumers during scarcity.
2. **Target 6.4 (Substantially Increase Water-Use Efficiency & Address Scarcity):**
   - Simulated through **Shortage Detection & Analytics**, computing real-time unmet demand percentages across all consumer nodes.
   - **Simulated Leakage Loss Modeling**, tracking non-revenue water lost in distribution pipelines and auto-generating priority remediation tickets.
3. **Target 6.5 (Integrated Water Resources Management):**
   - Simulated through **Adjacency List Pipeline Graphs**, utilizing Breadth-First Search (BFS) and Depth-First Search (DFS) for topological reachability and simulated pipe burst isolation analysis.

---

## 3. System Architecture

```
+---------------------------------------------------------------------------------------+
|                                    PRESENTATION LAYER                                 |
|               (Single Page Application: HTML5 / Modern CSS3 / Vanilla JS)             |
|                                                                                       |
|   +-------------------+  +-------------------+  +-------------------+  +----------+   |
|   | Dashboard Metrics |  | Water Tanks Table |  | BST Area Registry |  | Cytoscape|   |
|   |    & Chart.js     |  |  & Visual Gauges  |  |  & Search O(log n)|  |  Network |   |
|   +-------------------+  +-------------------+  +-------------------+  +----------+   |
+-------------------------------------------+-------------------------------------------+
                                            |
                                            | JavaScript fetch() / REST JSON
                                            v
+---------------------------------------------------------------------------------------+
|                                  C BACKEND SERVER LAYER                               |
|                         (Mongoose Event-Driven HTTP Engine :8080)                     |
|                                                                                       |
|       [REST Router] ───> [cJSON Parser] ───> [Activity Logger & Notification Engine]  |
+-------------------------------------------+-------------------------------------------+
                                            |
                                            | Dispatches to Data Structure Subsystems
                                            v
+---------------------------------------------------------------------------------------+
|                                DATA STRUCTURES & ALGORITHMS                           |
|                                                                                       |
|  1. ARRAYS            Tank Management, Capacity Bounds, Water Ledger Arithmetic       |
|  2. CIRCULAR QUEUE    FIFO Citizen Complaints Processing (Modulo Wrap-Around)         |
|  3. LINKED LIST       Dynamic Work Orders (malloc/free, Node Insertion & Deletion)    |
|  4. BINARY SEARCH TREE Inorder Ascending Sorting, Unique Key Insertion, Search O(logn)|
|  5. ADJACENCY GRAPH   BFS Shortest Route, DFS Component Reachability, Pipe Cut Sim    |
+-------------------------------------------+-------------------------------------------+
                                            |
                                            | Standard C File I/O (fopen, fgets, fprintf)
                                            v
+---------------------------------------------------------------------------------------+
|                                  PERSISTENCE STORAGE LAYER                            |
|             (backend/data/: tanks.dat, areas.dat, complaints.dat, maintenance.dat)    |
+---------------------------------------------------------------------------------------+
```

---

## 4. In-Depth Data Structure Analysis & Implementation

### 4.1 Data Structure 1: Arrays (Tank & Water Management)
* **File:** [`backend/tank.h`](file:///d:/projects/WaterManagement/backend/tank.h), [`backend/tank.c`](file:///d:/projects/WaterManagement/backend/tank.c)
* **Description:** Reservoirs and storage tanks are represented as an array of structured entities (`Tank g_tanks[MAX_TANKS]`).
* **Invariants Enforced:**
  1. **Upper Bound Constraint:** $0 \le \text{current\_level} + \Delta \le \text{capacity}$. Any refilling operation exceeding maximum capacity returns error code `-2` and prevents overflow.
  2. **Non-Negative Constraint:** $\text{current\_level} - \Delta \ge 0$. Distribution or leakage draining water below $0.0$ L returns error code `-3` and aborts.
  3. **Categorical Status:** Automatically transitions between `Normal` ($\ge 25\%$), `Low` ($< 25\%$), and `Empty` ($= 0$ L).
* **Viva Explanation:** Arrays offer deterministic $O(1)$ direct index access. Elements are shifted during deletion to preserve contiguous memory layout.

---

### 4.2 Data Structure 2: Circular Queue (Complaints Processing)
* **File:** [`backend/queue.h`](file:///d:/projects/WaterManagement/backend/queue.h), [`backend/queue.c`](file:///d:/projects/WaterManagement/backend/queue.c)
* **Description:** Citizen grievance reports (leaks, low pressure, outages) enter a fixed-size FIFO Circular Queue with modulo index arithmetic.
* **Algorithm Mechanism:**
  - `front`: Index of the oldest unprocessed complaint.
  - `rear`: Index where the most recent complaint was enqueued.
  - **Enqueue:** `rear = (rear + 1) % MAX_CAPACITY`. Checks `count == capacity` to return queue overflow (`-1`).
  - **Dequeue:** `front = (front + 1) % MAX_CAPACITY`. Checks `count == 0` to return queue underflow (`-1`).
* **Inter-Structure Connection:** Dequeuing a complaint automatically transitions it into the Maintenance Singly Linked List without creating duplicate active records.
* **Viva Explanation:** A standard linear queue implemented on an array suffers from false overflow (wasted slots at the front as elements are dequeued). A circular queue reuses freed slots at the beginning via modular arithmetic, ensuring $O(1)$ operations with zero memory reallocation.

---

### 4.3 Data Structure 3: Singly Linked List (Maintenance Management)
* **File:** [`backend/linkedlist.h`](file:///d:/projects/WaterManagement/backend/linkedlist.h), [`backend/linkedlist.c`](file:///d:/projects/WaterManagement/backend/linkedlist.c)
* **Description:** Unresolved engineering work orders, technician dispatches, and pipeline valve repairs are tracked in a dynamic singly linked list on the heap.
* **Algorithm Mechanism:**
  - Nodes: Allocated via `(MaintenanceNode*)malloc(sizeof(MaintenanceNode))` with fields `id`, `complaint_id`, `issue`, `status`, `technician`, `next`.
  - **Insertion:** Inserted at head in $O(1)$ time.
  - **Status Update:** Traverses list to locate node by `id` and modifies status (`Pending` $\to$ `In Progress` $\to$ `Resolved`).
  - **Deletion of Resolved Nodes:** Traverses the list with pointer rewiring (`prev->next = curr->next`), calling `free(curr)` to reclaim heap memory, and correctly adjusting `head` if the first element is resolved.
* **Viva Explanation:** Unlike fixed arrays, linked lists can dynamically grow and shrink at runtime without upfront memory reservations. Proper pointer management prevents memory leaks and dangling pointers.

---

### 4.4 Data Structure 4: Binary Search Tree (Area Registry & Demand)
* **File:** [`backend/bst.h`](file:///d:/projects/WaterManagement/backend/bst.h), [`backend/bst.c`](file:///d:/projects/WaterManagement/backend/bst.c)
* **Description:** Municipal distribution areas are stored in a dynamically allocated Binary Search Tree keyed by unique integer `area_id`.
* **Algorithm Mechanism:**
  - **Insertion:** Compares `id < curr->id` (traverse left) or `id > curr->id` (traverse right). If `id == curr->id`, duplicates are rejected.
  - **Search:** Binary search property: $O(\log n)$ average time complexity.
  - **Inorder Traversal:** Recursively visits `Left Subtree` $\to$ `Root Node` $\to$ `Right Subtree`. This mathematical property guarantees that areas are retrieved in strictly sorted ascending order of their Area IDs!
  - **Deallocation:** Post-order traversal (`Left` $\to$ `Right` $\to$ `Free(Root)`) ensures no memory leaks during server termination.
* **Viva Explanation:** BST provides logarithmic search performance while maintaining elements in dynamically sorted order. If elements are inserted in already sorted sequence without rebalancing, the tree degrades to a degenerate linked list with worst-case $O(n)$ search time.

---

### 4.5 Data Structure 5: Graph (Pipeline Network & BFS / DFS)
* **File:** [`backend/graph.h`](file:///d:/projects/WaterManagement/backend/graph.h), [`backend/graph.c`](file:///d:/projects/WaterManagement/backend/graph.c)
* **Description:** Reservoirs, junctions, and consumer nodes are modeled as vertices, while pipeline links are represented as undirected weighted edges in an Adjacency List.
* **Algorithm Mechanism:**
  - **Breadth-First Search (BFS):** Utilizes an auxiliary FIFO queue and `parent[]` array to find the shortest hop path between a source reservoir and consumer node. Accumulates pipeline length (meters) and returns the full path sequence.
  - **Depth-First Search (DFS):** Traverses the connected component using recursive graph traversal to evaluate full reachability.
  - **Pipeline Failure Simulation:** Temporarily disables an edge (`is_active = 0`), invokes DFS from the primary reservoir, and categorizes vertices into `Reachable` vs `Isolated/Unreachable`.
* **Important Academic Disclaimer:** The graph algorithm models **topological network connectivity and shortest routing**; it does not compute hydraulic head loss (Hazen-Williams / Darcy-Weisbach equations) or pressure gradients.

---

## 5. Complexity Analysis Summary

| Data Structure / Operation | Best Case | Average Case | Worst Case | Space Complexity |
| :--- | :--- | :--- | :--- | :--- |
| **Array (Tank Lookup)** | $O(1)$ | $O(1)$ | $O(1)$ | $O(N)$ contiguous |
| **Array (Tank Insert/Delete)** | $O(1)$ | $O(N)$ | $O(N)$ | $O(1)$ auxiliary |
| **Circular Queue (Enqueue)** | $O(1)$ | $O(1)$ | $O(1)$ | $O(K)$ bounded |
| **Circular Queue (Dequeue)** | $O(1)$ | $O(1)$ | $O(1)$ | $O(1)$ auxiliary |
| **Singly Linked List (Insert)** | $O(1)$ | $O(1)$ | $O(1)$ | $O(M)$ heap |
| **Singly Linked List (Delete)** | $O(1)$ (head) | $O(M)$ | $O(M)$ | $O(1)$ auxiliary |
| **BST (Search / Insert)** | $O(1)$ (root) | $O(\log V)$ | $O(V)$ (skewed) | $O(V)$ nodes |
| **BST (Inorder Traversal)** | $O(V)$ | $O(V)$ | $O(V)$ | $O(H)$ stack |
| **Graph (BFS Shortest Path)** | $O(1)$ | $O(V + E)$ | $O(V + E)$ | $O(V)$ queue |
| **Graph (DFS Reachability)** | $O(1)$ | $O(V + E)$ | $O(V + E)$ | $O(V)$ call stack |

*$V$ = Vertices, $E$ = Edges, $N$ = Tanks, $K$ = Queue Capacity, $M$ = Maintenance records, $H$ = Tree height.*

---

## 6. REST API Contract Specification

All endpoints communicate using standard JSON payloads over HTTP port 8080.

| Method | Endpoint | Description | Data Structure Invoked |
| :--- | :--- | :--- | :--- |
| `GET` | `/api/dashboard` | Returns aggregate metrics, charts, alerts, and recent activities | Arrays, BST, Queue, Ledger |
| `GET` | `/api/tanks` | Retrieves all registered water storage tanks | Array traversal |
| `POST` | `/api/tanks` | Adds new tank with capacity and level checks | Array bounds verification |
| `POST` | `/api/tanks/add-water` | Refills water into a specific tank | Array capacity clamp |
| `DELETE` | `/api/tanks?id=1` | Decommissions a tank | Array element shift |
| `GET` | `/api/areas` | Retrieves all areas in ascending ID order | **BST Inorder Traversal** |
| `POST` | `/api/areas` | Inserts new area into registry | **BST Key Insertion** |
| `GET` | `/api/areas/search?id=101` | Searches area by ID | **BST O(log n) Search** |
| `POST` | `/api/distribution/manual` | Distributes water from tank to area | Array & BST balance updates |
| `POST` | `/api/distribution/automatic` | Priority dispatch (Hospitals first) | Array + Priority Sort + BST |
| `GET` | `/api/complaints` | Returns pending complaints in FIFO order | **Circular Queue Traversal** |
| `POST` | `/api/complaints` | Enqueues a citizen grievance | **Circular Queue Enqueue** |
| `POST` | `/api/complaints/process-next` | Dequeues complaint & creates work order | **Queue Dequeue $\to$ Linked List Insert** |
| `GET` | `/api/maintenance` | Retrieves maintenance work orders | **Linked List Traversal** |
| `POST` | `/api/maintenance` | Creates new work order | **Linked List Head Insertion** |
| `PATCH` | `/api/maintenance/status` | Updates work order progress/resolution | **Linked List Search & Update** |
| `DELETE` | `/api/maintenance?delete_resolved=true` | Purges all resolved records | **Linked List Node Freeing** |
| `GET` | `/api/network` | Retrieves nodes and edges for Cytoscape | **Adjacency List Serialization** |
| `POST` | `/api/network/bfs` | Shortest path route between reservoir & consumer | **Graph BFS Algorithm** |
| `POST` | `/api/network/dfs` | Explores all connected component nodes | **Graph DFS Algorithm** |
| `POST` | `/api/network/failure-simulation` | Disables edge & returns isolated nodes | **Graph Failure Simulation** |
| `POST` | `/api/simulation/leakage` | Simulates pipe loss & auto-queues complaint | Water Ledger + Circular Queue |
| `POST` | `/api/system/reset` | Restores default demonstration dataset | Re-initializes all 5 data structures |

---

## 7. Frontend to Backend Integration Architecture

The web interface is decoupled and communicates with the C backend exclusively via asynchronous JavaScript `fetch()` calls.

1. **Reactive UI Updates:** Operations such as adding water, dispatching distribution, or processing complaints trigger dynamic DOM updates and redraw Chart.js visualizations without full page reloads.
2. **Error and Bounds Handling:** If the C backend rejects an operation (e.g., adding water exceeding maximum capacity, queue full condition, duplicate area ID), the exact error message from the C backend is surfaced as an interactive visual toast alert.
3. **Interactive Graph Topology:** The pipeline grid is rendered dynamically using **Cytoscape.js**. The user can trigger C graph algorithms (BFS / Failure Cuts) through the toolbar, and the visual canvas dynamically colors nodes and highlights edges in real time.

---

## 8. Verification and Test Results

The system includes a standalone unit test suite in [`tests/test_all.c`](file:///d:/projects/WaterManagement/tests/test_all.c).

* **Test Execution Result:**
  ```
  =================================================================
   Smart Water Distribution System - Automated Test Suite
   UN SDG 6: Clean Water and Sanitation Data Structures Mini-Project
  =================================================================

  === Running Test Suite 1: Arrays (Tank Management) ===
    [PASS] Tank 1 added with valid ID
    [PASS] Tank 2 added with valid ID
    [PASS] Tank count equals 2
    [PASS] Retrieve Tank 1 by ID
    [PASS] Tank status is Normal at 50% capacity
    [PASS] Added 20,000 L to Tank 1
    [PASS] Rejected water addition exceeding maximum capacity
    [PASS] Tank level unchanged after rejected overflow
    [PASS] Rejected water deduction that would cause negative level
    [PASS] Deducted water successfully
    [PASS] Tank status correctly transitioned to Low (< 25%)
    [PASS] Tank status correctly transitioned to Empty (0 L)
    [PASS] Aggregate capacity computed correctly (150,000 L)
    [PASS] Aggregate available water computed correctly (40,000 L)

  === Running Test Suite 2: Circular Queue (Complaints) ===
    [PASS] New queue is empty
    [PASS] New queue is not full
    [PASS] Enqueued complaint 1
    [PASS] Enqueued complaint 2
    [PASS] Enqueued complaint 3
    [PASS] Queue count is 3
    [PASS] Peek returned complaint 1 without removal
    [PASS] Count unchanged after peek
    [PASS] First dequeued is Complaint 1 (FIFO)
    [PASS] Second dequeued is Complaint 2 (FIFO)
    [PASS] Count is now 1
    [PASS] Found Complaint 3 by ID
    [PASS] Searching non-existent ID safely returns -1
    [PASS] Queue is empty after dequeuing all
    [PASS] Safe handling of queue underflow

  === Running Test Suite 3: Singly Linked List (Maintenance) ===
    [PASS] New list count is 0
    [PASS] List count is 3 after insertions
    [PASS] Search by maintenance ID succeeded
    [PASS] Updated maintenance status to Resolved
    [PASS] Resolution timestamp recorded
    [PASS] Deleted 2 resolved records
    [PASS] Remaining count is 1
    [PASS] Resolved record m1 no longer exists
    [PASS] Unresolved record m2 still exists
    [PASS] Deleted remaining record m2
    [PASS] List is now empty
    [PASS] Deleting non-existent ID safely returns -1

  === Running Test Suite 4: Binary Search Tree (Areas) ===
    [PASS] Inserted root ID 50
    [PASS] Inserted left child ID 20
    [PASS] Inserted right child ID 70
    [PASS] Inserted ID 10
    [PASS] Inserted ID 30
    [PASS] Duplicate area ID correctly rejected by BST
    [PASS] BST search located node ID 30
    [PASS] BST search returns NULL for missing ID
    [PASS] Inorder traversal returned all 5 areas
    [PASS] Inorder traversal produced strictly ascending Area IDs: 10, 20, 30, 50, 70
    [PASS] Supplied 100,000 L to hospital
    [PASS] Total demand calculated correctly (500,000 L)
    [PASS] Total supplied calculated correctly (100,000 L)
    [PASS] Total unmet demand calculated correctly (400,000 L)
    [PASS] BST recursively deallocated without leaks

  === Running Test Suite 5: Pipeline Graph (BFS & DFS) ===
    [PASS] Pipeline edges created successfully
    [PASS] Hospital 3 is reachable from Reservoir 1 via BFS
    [PASS] BFS shortest path length is 3 hops (1 -> 2 -> 3)
    [PASS] Exact BFS path matches 1 -> 2 -> 3
    [PASS] Total pipeline distance is 800 meters
    [PASS] Isolated node 5 is unreachable via BFS
    [PASS] DFS explored all 4 connected components
    [PASS] 3 nodes remain reachable during pipeline e2 break
    [PASS] 2 nodes isolated during failure (Hospital 3 and Isolated 5)
    [PASS] Hospital 3 accurately identified as disconnected by simulation

  === Running Test Suite 6: File Storage Persistence ===
    [PASS] Default tanks initialized
    [PASS] Default BST areas initialized
    [PASS] Default complaints initialized
    [PASS] Default maintenance initialized
    [PASS] All data successfully saved to disk
    [PASS] Tanks cleared in memory
    [PASS] Areas cleared in memory
    [PASS] Queue cleared in memory
    [PASS] All data reloaded from disk
    [PASS] Reloaded tank count matches
    [PASS] Reloaded BST area count matches
    [PASS] Reloaded complaint queue count matches
    [PASS] Reloaded maintenance list count matches

  =================================================================
   TEST SUITE SUMMARY: 79 PASSED, 0 FAILED
  =================================================================
  ```

---

## 9. Viva Voce Examination Questions & Answers Guide

### Q1: Why did you implement the complaints system as a Circular Queue rather than a simple array?
**A:** A linear array queue causes memory wastage ("false overflow"): once elements are dequeued from the front, those slots cannot be reused without shifting every remaining element left ($O(n)$ cost). A **Circular Queue** wraps around using modulo arithmetic (`(rear + 1) % MAX`), maintaining strict $O(1)$ enqueue and dequeue operations without shifting elements.

### Q2: How does the Binary Search Tree ensure that areas are displayed in numerical order?
**A:** By definition of a Binary Search Tree, for any given node $N$, all nodes in its left subtree have keys smaller than $N$, and all nodes in its right subtree have keys greater than $N$. An **Inorder Traversal** (`Left` $\to$ `Node` $\to$ `Right`) visits keys in strictly monotonic non-decreasing order, naturally sorting the areas without needing an external sorting algorithm.

### Q3: How do you prevent memory leaks when deleting resolved maintenance records?
**A:** In the singly linked list (`linkedlist.c`), we maintain a `prev` pointer while traversing. When a node with `status == "Resolved"` is encountered:
1. `prev->next` is updated to point to `curr->next`.
2. If the head node is resolved, `list->head` is updated to `curr->next`.
3. `free(curr)` is called explicitly to return the dynamic heap memory to the operating system.

### Q4: What is the difference between BFS and DFS in your pipeline graph?
**A:** 
- **Breadth-First Search (BFS)** traverses the network level by level using a FIFO queue. It is guaranteed to find the path with the minimum number of pipeline hops from a reservoir to any consumer node.
- **Depth-First Search (DFS)** traverses as deep as possible along each branch before backtracking. It is used to explore connected components and verify network reachability when a pipeline edge fails.

### Q5: How is this system connected to UN SDG 6?
**A:** UN SDG 6 emphasizes clean water and equitable sanitation. Our system implements priority allocation policies protecting vulnerable facilities (hospitals and healthcare centers) during scarcity, detects unmet demands, tracks non-revenue water (NRW) lost to leakage, and provides structured infrastructure maintenance workflows.

---

## 10. Conclusion and Future Work

The **Smart Water Distribution and Management System** successfully validates how fundamental data structures can be effectively orchestrated to model a critical civic utility. Future enhancements could incorporate:
1. **AVL or Red-Black Tree Balancing:** To guarantee $O(\log n)$ worst-case tree depth regardless of area insertion order.
2. **Dijkstra’s Algorithm or A\* Search:** To calculate lowest-energy water pumping routes considering pipeline friction head loss.
3. **IoT Sensor Ingestion:** Integrating real-time MQTT telemetry from physical pressure and flow sensors into the C event loop.
