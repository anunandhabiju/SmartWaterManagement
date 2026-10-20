/**
 * Smart Water Distribution and Management System
 * Main Application Orchestrator & Event Controller
 */

const App = {
    currentPage: 'dashboard',

    init() {
        this.setupNavigation();
        this.setupModals();
        this.setupForms();
        this.setupActionButtons();

        // Initial Data Load
        this.refreshAll();
    },

    setupNavigation() {
        const navLinks = document.querySelectorAll('.nav-link[data-page]');
        const menuToggle = document.getElementById('menu-toggle');
        const sidebar = document.getElementById('sidebar');

        if (menuToggle && sidebar) {
            menuToggle.addEventListener('click', () => {
                sidebar.classList.toggle('open');
            });
        }

        navLinks.forEach(link => {
            link.addEventListener('click', (e) => {
                e.preventDefault();
                const targetPage = link.getAttribute('data-page');
                this.navigateTo(targetPage);

                if (sidebar) sidebar.classList.remove('open');
            });
        });
    },

    navigateTo(page) {
        this.currentPage = page;

        // Update nav links active state
        document.querySelectorAll('.nav-link').forEach(link => {
            link.classList.toggle('active', link.getAttribute('data-page') === page);
        });

        // Update views visibility
        document.querySelectorAll('.page-view').forEach(view => {
            view.classList.remove('active');
        });

        const targetView = document.getElementById(`view-${page}`);
        if (targetView) {
            targetView.classList.add('active');
        }

        // Update Header Title
        const pageTitles = {
            dashboard: 'Operational Dashboard',
            tanks: 'Water Reservoir & Tank Management (Arrays)',
            areas: 'Consumer Areas & Demand Registry (BST)',
            distribution: 'Water Allocation & Priority Dispatch Engine',
            shortages: 'Water Shortage Detection & UN SDG 6 Compliance Report',
            complaints: 'Consumer Complaints Processing (Circular Queue)',
            maintenance: 'Maintenance Work Orders (Singly Linked List)',
            network: 'Pipeline Network Graph & Connectivity (BFS/DFS)',
            simulation: 'Pipeline Burst & Leakage Loss Simulator'
        };

        const titleEl = document.getElementById('page-title');
        if (titleEl) {
            titleEl.textContent = pageTitles[page] || 'Smart Water Management';
        }

        // If navigating to network graph, re-fit Cytoscape canvas
        if (page === 'network' && Network.cy) {
            setTimeout(() => {
                Network.cy.resize();
                Network.cy.fit();
            }, 100);
        }
    },

    setupModals() {
        // Open buttons
        document.getElementById('btn-open-add-tank')?.addEventListener('click', () => this.openModal('modal-add-tank'));
        document.getElementById('btn-open-refill-tank')?.addEventListener('click', () => this.openModal('modal-refill-tank'));
        document.getElementById('btn-open-add-area')?.addEventListener('click', () => this.openModal('modal-add-area'));
        document.getElementById('btn-open-register-complaint')?.addEventListener('click', () => this.openModal('modal-register-complaint'));
        document.getElementById('btn-open-add-maintenance')?.addEventListener('click', () => this.openModal('modal-add-maintenance'));

        // Close triggers
        document.querySelectorAll('[data-close]').forEach(btn => {
            btn.addEventListener('click', () => {
                const targetId = btn.getAttribute('data-close');
                this.closeModal(targetId);
            });
        });

        // Close on backdrop click
        document.querySelectorAll('.modal-overlay').forEach(modal => {
            modal.addEventListener('click', (e) => {
                if (e.target === modal) {
                    modal.classList.remove('active');
                }
            });
        });

        // Close on ESC key
        document.addEventListener('keydown', (e) => {
            if (e.key === 'Escape') {
                this.closeModals();
            }
        });
    },

    openModal(modalId) {
        const modal = document.getElementById(modalId);
        if (modal) modal.classList.add('active');
    },

    closeModal(modalId) {
        const modal = document.getElementById(modalId);
        if (modal) modal.classList.remove('active');
    },

    closeModals() {
        document.querySelectorAll('.modal-overlay').forEach(m => m.classList.remove('active'));
    },

    setupForms() {
        // Add Tank Form
        document.getElementById('form-add-tank')?.addEventListener('submit', (e) => {
            e.preventDefault();
            Tanks.addTank({
                name: document.getElementById('tank-name').value,
                capacity: parseFloat(document.getElementById('tank-cap').value),
                initial_level: parseFloat(document.getElementById('tank-init-level').value),
                location: document.getElementById('tank-loc').value
            });
        });

        // Refill Tank Form
        document.getElementById('form-refill-tank')?.addEventListener('submit', (e) => {
            e.preventDefault();
            const id = parseInt(document.getElementById('refill-tank-id').value);
            const amount = parseFloat(document.getElementById('refill-amount').value);
            Tanks.refillWater(id, amount);
        });

        // Add Area Form
        document.getElementById('form-add-area')?.addEventListener('submit', (e) => {
            e.preventDefault();
            Areas.addArea({
                id: parseInt(document.getElementById('area-id').value),
                name: document.getElementById('area-name').value,
                demand: parseFloat(document.getElementById('area-demand').value),
                priority: parseInt(document.getElementById('area-priority').value),
                category: document.getElementById('area-cat').value
            });
        });

        // Manual Distribution Form
        document.getElementById('form-manual-dist')?.addEventListener('submit', (e) => {
            e.preventDefault();
            const tankId = document.getElementById('dist-source-tank').value;
            const areaId = document.getElementById('dist-dest-area').value;
            const amount = document.getElementById('dist-amount').value;
            Distribution.dispatchManual(tankId, areaId, amount);
        });

        // Dynamic hint updates on selection
        document.getElementById('dist-source-tank')?.addEventListener('change', (e) => {
            const opt = e.target.selectedOptions[0];
            const level = opt ? opt.getAttribute('data-level') : null;
            document.getElementById('hint-tank-water').textContent = level ? `Available water: ${Number(level).toLocaleString()} L` : 'Available water: -- L';
        });

        document.getElementById('dist-dest-area')?.addEventListener('change', (e) => {
            const opt = e.target.selectedOptions[0];
            const unmet = opt ? opt.getAttribute('data-unmet') : null;
            document.getElementById('hint-area-demand').textContent = unmet ? `Current unmet demand: ${Number(unmet).toLocaleString()} L` : 'Current unmet demand: -- L';
        });

        // Register Complaint Form
        document.getElementById('form-register-complaint')?.addEventListener('submit', (e) => {
            e.preventDefault();
            const areaSelect = document.getElementById('comp-area-id');
            Complaints.registerComplaint({
                area_id: parseInt(areaSelect.value),
                area_name: areaSelect.selectedOptions[0]?.textContent || '',
                issue_type: document.getElementById('comp-type').value,
                description: document.getElementById('comp-desc').value,
                priority: document.getElementById('comp-priority').value
            });
        });

        // Add Maintenance Form
        document.getElementById('form-add-maintenance')?.addEventListener('submit', (e) => {
            e.preventDefault();
            Maintenance.addRecord({
                complaint_id: 0,
                area_name: document.getElementById('maint-area').value,
                issue: document.getElementById('maint-issue').value,
                status: document.getElementById('maint-status').value,
                technician: document.getElementById('maint-tech').value
            });
        });

        // Update Maintenance Status Form
        document.getElementById('form-update-maint-status')?.addEventListener('submit', (e) => {
            e.preventDefault();
            const id = document.getElementById('update-maint-id').value;
            const status = document.getElementById('update-maint-status').value;
            const tech = document.getElementById('update-maint-tech').value;
            Maintenance.updateStatus(id, status, tech);
        });

        // Leakage Simulation Form
        document.getElementById('form-leakage-sim')?.addEventListener('submit', (e) => {
            e.preventDefault();
            const pipeId = document.getElementById('sim-leak-pipeline').value;
            const loss = document.getElementById('sim-leak-loss').value;
            Simulation.triggerLeakage(pipeId, loss);
        });
    },

    setupActionButtons() {
        // Global refresh
        document.getElementById('btn-refresh')?.addEventListener('click', () => this.refreshAll());

        // Quick auto distribute from header
        document.getElementById('btn-quick-auto-dist')?.addEventListener('click', () => Distribution.dispatchAutomatic());
        document.getElementById('btn-run-auto-dist')?.addEventListener('click', () => Distribution.dispatchAutomatic());

        // Reset demo dataset
        document.getElementById('btn-reset-demo')?.addEventListener('click', async () => {
            if (!confirm('Are you sure you want to restore the demonstration dataset to its default state? All manual changes will be reset.')) {
                return;
            }
            try {
                const resp = await API.post('/api/system/reset', {});
                if (resp.success) {
                    API.showToast('Demo dataset reset to initial state!', 'success');
                    this.refreshAll();
                }
            } catch (err) {
                API.showToast(err.message, 'error');
            }
        });

        // Process next complaint
        document.getElementById('btn-process-next-complaint')?.addEventListener('click', () => Complaints.processNext());

        // Purge resolved maintenance records
        document.getElementById('btn-purge-resolved')?.addEventListener('click', () => Maintenance.purgeResolved());

        // BST Search buttons
        document.getElementById('btn-bst-search')?.addEventListener('click', () => {
            const id = document.getElementById('input-bst-search-id').value;
            Areas.searchArea(id);
        });

        document.getElementById('btn-bst-clear-search')?.addEventListener('click', () => Areas.clearSearch());

        // Shortage filter
        document.getElementById('select-shortage-filter')?.addEventListener('change', (e) => {
            Areas.renderShortages(e.target.value);
        });

        // Graph controls
        document.getElementById('btn-run-bfs')?.addEventListener('click', () => {
            const start = document.getElementById('bfs-start-node').value;
            const target = document.getElementById('bfs-target-node').value;
            Network.runBFS(start, target);
        });

        document.getElementById('btn-run-fail-sim')?.addEventListener('click', () => {
            const edge = document.getElementById('sim-fail-pipe').value;
            const res = document.getElementById('bfs-start-node').value;
            Network.runFailureSimulation(edge, res);
        });

        document.getElementById('btn-reconnect-all-pipes')?.addEventListener('click', () => Network.reconnectAllPipelines());
        document.getElementById('btn-cy-fit')?.addEventListener('click', () => {
            if (Network.cy) {
                Network.cy.resize();
                Network.cy.fit();
            }
        });
    },

    async refreshAll() {
        API.showToast('Syncing with C backend engine...', 'info', 1500);
        await Promise.all([
            Dashboard.refresh(),
            Tanks.load(),
            Areas.load(),
            Distribution.load(),
            Complaints.load(),
            Maintenance.load(),
            Network.load()
        ]);
    }
};

window.App = App;

document.addEventListener('DOMContentLoaded', () => {
    App.init();
});
