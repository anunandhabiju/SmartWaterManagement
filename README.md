# 💧 Smart Water Distribution and Management System
### UN SDG 6: Clean Water and Sanitation | Data Structures Mini-Project

A full-stack, interactive academic simulation and management system engineered in **Pure C** (with embedded Mongoose HTTP & cJSON) and a modern **HTML5/CSS3/JavaScript** web dashboard powered by Chart.js and Cytoscape.js.

The system demonstrates the practical synthesis of five fundamental data structures to manage municipal reservoirs, priority-based distribution networks, citizen complaints, and maintenance infrastructure.

---

## 🌟 Key Features & Data Structures Mapping

| Feature Area | Data Structure | Implementation Highlights |
| :--- | :--- | :--- |
| **Water Tanks & Reservoirs** | **Arrays** | Fixed/bounded array with upper capacity constraints, non-negative bounds checking, and categorical status (`Normal`, `Low`, `Empty`). |
| **Citizen Complaints** | **Circular Queue** | Fixed-capacity FIFO ring buffer with modulo index arithmetic (`(rear + 1) % MAX`). Overflows and underflows safely handled. |
| **Maintenance Work Orders** | **Singly Linked List** | Heap-allocated dynamic nodes (`malloc`/`free`). Supports $O(1)$ head insertion, status tracking, and memory purging of resolved records. |
| **Consumer Areas & Demand** | **Binary Search Tree** | Keyed on unique integer `area_id`. Average $O(\log n)$ search and insertion. Inorder traversal automatically yields sorted area IDs! |
| **Pipeline Network Grid** | **Adjacency List Graph** | Vertices represent reservoirs, junctions, and consumers; edges represent pipelines. Breadth-First Search (BFS) finds shortest routes; Depth-First Search (DFS) analyzes pipeline cut failures. |
| **Priority Allocation Engine** | **Priority Dispatch** | Prioritizes essential healthcare facilities (Priority 1) before commercial and residential zones during supply shortages. |
| **Leakage Loss Simulator** | **Simulated NRW** | Deducts simulated burst losses from water balances and auto-generates emergency high-priority complaints in the circular queue. |
| **Disk File Persistence** | **C File I/O** | All tanks, BST areas, complaints, maintenance, and distribution history persist in `backend/data/` across application restarts. |

---

## 🚀 Quick Start Guide (Windows)

### Prerequisites
A standard C compiler on Windows:
- **Clang / LLVM** or **MinGW-w64 (GCC)** (The included `build.bat` automatically locates `clang`, `gcc`, or WinGet LLVM-MinGW).

### 1. Build the System
Double-click `build.bat` or run in Command Prompt / PowerShell:
```cmd
build.bat
```
This script will:
1. Detect your C compiler (`clang` or `gcc`).
2. Compile and run the automated test suite (`tests/test_all.exe`) across all five data structures.
3. Compile the Mongoose C HTTP server (`backend/server.exe`) linking with Windows Sockets (`-lws2_32`).

### 2. Run the Application
Double-click `run.bat` or execute:
```cmd
run.bat
```
This starts the C server on `http://localhost:8080` and automatically opens your web browser to:
👉 **`http://localhost:8080/index.html`**

---

## 📂 Project Structure

```text
WaterManagement/
├── backend/
│   ├── main.c              # Application entry point & signal handling
│   ├── server.h / .c       # Mongoose HTTP REST API router & static file server
│   ├── tank.h / .c         # Data Structure 1: Arrays (Tank Management)
│   ├── queue.h / .c        # Data Structure 2: Circular Queue (Complaints)
│   ├── linkedlist.h / .c   # Data Structure 3: Singly Linked List (Maintenance)
│   ├── bst.h / .c          # Data Structure 4: Binary Search Tree (Areas)
│   ├── graph.h / .c        # Data Structure 5: Adjacency List Graph (Pipelines)
│   ├── distribution.h / .c # Water allocation engine & leakage simulation
│   ├── storage.h / .c      # File persistence layer (backend/data/*.dat)
│   ├── mongoose.h / .c     # Single-file embedded C HTTP server
│   ├── cJSON.h / .c        # Single-file C JSON parser & generator
│   └── data/               # Persistent data storage files
├── frontend/
│   ├── index.html          # Single-Page Application interface
│   ├── css/
│   │   └── style.css       # Responsive water-utility theme
│   ├── js/
│   │   ├── api.js          # REST API client with toast alerts
│   │   ├── app.js          # Navigation router & event controllers
│   │   ├── dashboard.js    # Chart.js visualizations & operational KPIs
│   │   ├── tanks.js        # Tank management & fill level meters
│   │   ├── areas.js        # BST area search & sorted display
│   │   ├── distribution.js # Manual & automatic priority allocation
│   │   ├── complaints.js   # Circular queue visualizer & FIFO processing
│   │   ├── maintenance.js  # Linked list work orders & status updating
│   │   ├── network.js      # Cytoscape.js graph rendering & BFS pathfinder
│   │   └── simulation.js   # Pipeline burst & leakage loss simulation
│   └── assets/
│       ├── chart.umd.min.js# Offline Chart.js v4 library
│       └── cytoscape.min.js# Offline Cytoscape.js v3 library
├── tests/
│   ├── test_all.c          # Comprehensive unit test suite (79 tests)
│   └── test_all.exe        # Compiled test binary
├── build.bat               # Automated Windows build script
├── run.bat                 # One-click launch script
├── Makefile                # Cross-platform Makefile
├── PROJECT_REPORT.md       # Full academic report & Viva Q&A Guide
└── README.md               # Project documentation
```

---

## 🧪 Automated Unit Test Suite

Run the standalone unit test suite anytime:
```cmd
tests\test_all.exe
```
**Verification Scope:**
- **Arrays:** Capacity upper bounds, non-negative clamps, status transitions.
- **Circular Queue:** Modulo wrap-around index arithmetic, FIFO dequeue order, empty/full detection.
- **Singly Linked List:** Dynamic `malloc`/`free` safety, head insertion, status updating, resolved node deletion without memory leaks.
- **BST:** Duplicate ID prevention, $O(\log n)$ key search, sorted inorder traversal, recursive cleanup.
- **Graph:** BFS shortest hop discovery, DFS component reachability, simulated pipeline cut and isolation detection.
- **Storage:** File save/reload data consistency verification.

---

## 🌐 REST API Endpoints Overview

| Method | Endpoint | Description |
| :--- | :--- | :--- |
| `GET` | `/api/dashboard` | Aggregated statistics, Chart.js metrics, and alerts |
| `GET` / `POST` / `DELETE` | `/api/tanks` | Reservoir CRUD with capacity validation |
| `POST` | `/api/tanks/add-water` | Refill water into a tank |
| `GET` / `POST` / `PATCH` | `/api/areas` | BST consumer areas management |
| `GET` | `/api/areas/search?id=...` | BST $O(\log n)$ search by unique Area ID |
| `POST` | `/api/distribution/manual` | Manual tank-to-area water dispatch |
| `POST` | `/api/distribution/automatic` | Priority dispatch (Hospitals $\to$ Commercial $\to$ Residential) |
| `GET` / `POST` | `/api/complaints` | Circular queue complaint management |
| `POST` | `/api/complaints/process-next`| Dequeues complaint (FIFO) and creates maintenance order |
| `GET` / `POST` / `PATCH` / `DELETE` | `/api/maintenance` | Singly linked list maintenance records |
| `GET` | `/api/network` | Pipeline graph vertices & edges for Cytoscape.js |
| `POST` | `/api/network/bfs` | C Breadth-First Search route finder |
| `POST` | `/api/network/dfs` | C Depth-First Search connected components |
| `POST` | `/api/network/failure-simulation`| Pipeline cut failure simulation |
| `POST` | `/api/simulation/leakage` | Deducts pipeline loss & auto-queues complaint |
| `POST` | `/api/system/reset` | Resets to default demonstration dataset |

---

## 🎓 Academic Viva Voce Highlights

1. **Why C for the Backend?**  
   Enforces genuine low-level data structure implementations using manual memory management (`malloc`/`free`), pointer manipulation, and algorithmic efficiency without relying on high-level language runtime collections.
2. **Circular Queue vs Linear Queue:**  
   Circular queues prevent "false overflow" by utilizing modulo arithmetic `(index + 1) % capacity` to wrap around and reuse freed memory slots.
3. **BST Inorder Traversal:**  
   Because of the BST ordering invariant (left < root < right), recursive inorder traversal naturally retrieves areas in ascending numerical order of their IDs without requiring any post-sorting.
4. **BFS vs DFS:**  
   BFS explores nodes level by level using a queue to identify the path with the fewest pipeline hops between reservoir and consumer; DFS explores deep into paths to detect disconnected islands when a pipeline fails.

---

## 📄 License & Credits
Developed as an academic mini-project aligned with **United Nations Sustainable Development Goal 6 (Clean Water and Sanitation)**.  
Includes open-source components: Mongoose (Cesanta), cJSON (Dave Gamble), Chart.js, and Cytoscape.js.
