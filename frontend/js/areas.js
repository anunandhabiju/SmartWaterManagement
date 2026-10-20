/**
 * Smart Water Distribution and Management System
 * Area Management (Binary Search Tree Data Structure) & Shortage Reports
 */

const Areas = {
    areasList: [],

    async load() {
        try {
            const resp = await API.get('/api/areas');
            if (resp.success && resp.data) {
                this.areasList = resp.data.areas || [];
                this.render();
                this.renderShortages();
                this.updateDropdowns();
            }
        } catch (err) {
            API.showToast('Failed to load areas: ' + err.message, 'error');
        }
    },

    render(filteredList = null) {
        const tbody = document.getElementById('table-areas-body');
        if (!tbody) return;

        const list = filteredList || this.areasList;
        if (list.length === 0) {
            tbody.innerHTML = '<tr><td colspan="8" style="text-align:center;">No distribution areas found.</td></tr>';
            return;
        }

        tbody.innerHTML = list.map(a => {
            const priorityBadge = a.priority === 1
                ? '<span class="badge badge-hospital">Priority 1: Critical</span>'
                : (a.priority === 2
                    ? '<span class="badge badge-commercial">Priority 2: Commercial</span>'
                    : '<span class="badge badge-residential">Priority 3: Residential</span>');

            const pct = (a.percentage || 0).toFixed(1);
            let statusBadge = '<span class="badge badge-success">Fully Supplied</span>';
            if (a.supplied <= 0.001) {
                statusBadge = '<span class="badge badge-empty">No Water Supplied</span>';
            } else if (a.unmet > 0.001) {
                statusBadge = '<span class="badge badge-warning">Partially Supplied</span>';
            }

            return `
                <tr id="area-row-${a.id}">
                    <td><strong>#${a.id}</strong></td>
                    <td><strong>${a.name}</strong></td>
                    <td><span class="badge badge-secondary">${a.category}</span></td>
                    <td>${priorityBadge}</td>
                    <td>${Number(a.demand).toLocaleString()} L</td>
                    <td>${Number(a.supplied).toLocaleString()} L</td>
                    <td style="color:${a.unmet > 0 ? 'var(--danger)' : 'var(--success)'};font-weight:600;">
                        ${Number(a.unmet).toLocaleString()} L
                    </td>
                    <td>
                        <div style="font-weight:600;font-size:12px;">${pct}%</div>
                        ${statusBadge}
                    </td>
                </tr>
            `;
        }).join('');
    },

    async searchArea(id) {
        const banner = document.getElementById('bst-search-result-banner');
        if (!id) {
            API.showToast('Please enter an Area ID to search', 'warning');
            return;
        }

        try {
            const resp = await API.get(`/api/areas/search?id=${id}`);
            if (resp.success && resp.data) {
                const a = resp.data;
                banner.style.display = 'block';
                banner.innerHTML = `
                    <div class="alert-banner alert-info" style="display:flex;justify-content:space-between;align-items:center;">
                        <div>
                            <strong>✅ Area Found via BST O(log n) Search:</strong>
                            #${a.id} - ${a.name} (${a.category}) | Demand: ${Number(a.demand).toLocaleString()} L | Supplied: ${Number(a.supplied).toLocaleString()} L | Unmet: ${Number(a.unmet).toLocaleString()} L
                        </div>
                        <button class="btn btn-sm btn-secondary" onclick="document.getElementById('bst-search-result-banner').style.display='none';">Dismiss</button>
                    </div>
                `;

                // Highlight the matching row
                const row = document.getElementById(`area-row-${a.id}`);
                if (row) {
                    row.scrollIntoView({ behavior: 'smooth', block: 'center' });
                    row.style.background = '#e0f2fe';
                    setTimeout(() => { row.style.background = ''; }, 3000);
                }
            }
        } catch (err) {
            banner.style.display = 'block';
            banner.innerHTML = `
                <div class="alert-banner alert-danger" style="display:flex;justify-content:space-between;align-items:center;">
                    <div><strong>❌ Area ID #${id} not found in Binary Search Tree.</strong></div>
                    <button class="btn btn-sm btn-secondary" onclick="document.getElementById('bst-search-result-banner').style.display='none';">Dismiss</button>
                </div>
            `;
        }
    },

    clearSearch() {
        document.getElementById('input-bst-search-id').value = '';
        document.getElementById('bst-search-result-banner').style.display = 'none';
        this.render();
    },

    async addArea(formData) {
        try {
            const resp = await API.post('/api/areas', formData);
            if (resp.success) {
                API.showToast(`Area #${formData.id} successfully inserted into BST!`, 'success');
                App.closeModals();
                this.load();
                Dashboard.refresh();
            }
        } catch (err) {
            API.showToast(err.message, 'error');
        }
    },

    updateDropdowns() {
        // Distribution destination area
        const distAreaSelect = document.getElementById('dist-dest-area');
        if (distAreaSelect) {
            distAreaSelect.innerHTML = '<option value="">-- Choose Consumer Area --</option>' +
                this.areasList.map(a =>
                    `<option value="${a.id}" data-demand="${a.demand}" data-unmet="${a.unmet}">${a.name} (#${a.id}) - Unmet: ${Number(a.unmet).toLocaleString()} L</option>`
                ).join('');
        }

        // Complaint area dropdown
        const compAreaSelect = document.getElementById('comp-area-id');
        if (compAreaSelect) {
            compAreaSelect.innerHTML = this.areasList.map(a =>
                `<option value="${a.id}">${a.name} (#${a.id}) - ${a.category}</option>`
            ).join('');
        }

        // BFS destination area dropdown
        const bfsTargetSelect = document.getElementById('bfs-target-node');
        if (bfsTargetSelect) {
            bfsTargetSelect.innerHTML = this.areasList.map(a =>
                `<option value="${a.id}">${a.name} (#${a.id})</option>`
            ).join('');
        }
    },

    renderShortages(filter = 'ALL') {
        const tbody = document.getElementById('table-shortages-body');
        if (!tbody) return;

        let totalDemand = 0;
        let totalSupplied = 0;
        let totalUnmet = 0;

        this.areasList.forEach(a => {
            totalDemand += a.demand;
            totalSupplied += a.supplied;
            totalUnmet += a.unmet;
        });

        document.getElementById('report-total-demand').textContent = Number(totalDemand).toLocaleString() + ' L';
        document.getElementById('report-total-supplied').textContent = Number(totalSupplied).toLocaleString() + ' L';
        document.getElementById('report-total-unmet').textContent = Number(totalUnmet).toLocaleString() + ' L';

        const satisfaction = totalDemand > 0 ? ((totalSupplied / totalDemand) * 100).toFixed(1) : 100;
        document.getElementById('report-satisfaction-pct').textContent = `${satisfaction}%`;

        // Apply Filter
        let list = this.areasList;
        if (filter === 'SHORTAGE') {
            list = list.filter(a => a.unmet > 0.001);
        } else if (filter === 'HOSPITAL') {
            list = list.filter(a => a.priority === 1);
        } else if (filter === 'FULFILLED') {
            list = list.filter(a => a.unmet <= 0.001);
        }

        if (list.length === 0) {
            tbody.innerHTML = '<tr><td colspan="9" style="text-align:center;">No areas matching the selected filter.</td></tr>';
            return;
        }

        tbody.innerHTML = list.map(a => {
            const pct = (a.percentage || 0).toFixed(1);
            let statusBadge = '<span class="badge badge-success">100% Fully Supplied</span>';
            if (a.supplied <= 0.001) {
                statusBadge = '<span class="badge badge-empty">0% - Critical Shortage</span>';
            } else if (a.unmet > 0.001) {
                statusBadge = `<span class="badge badge-warning">Deficit: ${Number(a.unmet).toLocaleString()} L</span>`;
            }

            const priorityBadge = a.priority === 1
                ? '<span class="badge badge-hospital">Priority 1 (Hospital)</span>'
                : (a.priority === 2 ? '<span class="badge badge-commercial">Priority 2</span>' : '<span class="badge badge-residential">Priority 3</span>');

            return `
                <tr>
                    <td><strong>#${a.id}</strong></td>
                    <td><strong>${a.name}</strong></td>
                    <td>${a.category}</td>
                    <td>${priorityBadge}</td>
                    <td>${Number(a.demand).toLocaleString()} L</td>
                    <td>${Number(a.supplied).toLocaleString()} L</td>
                    <td style="color:${a.unmet > 0 ? 'var(--danger)' : 'var(--success)'};font-weight:700;">
                        ${Number(a.unmet).toLocaleString()} L
                    </td>
                    <td><strong>${pct}%</strong></td>
                    <td>${statusBadge}</td>
                </tr>
            `;
        }).join('');
    }
};

window.Areas = Areas;
