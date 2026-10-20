/**
 * Smart Water Distribution and Management System
 * Maintenance Management (Singly Linked List Data Structure)
 */

const Maintenance = {
    records: [],

    async load() {
        try {
            const resp = await API.get('/api/maintenance');
            if (resp.success && resp.data) {
                this.records = resp.data.records || [];
                this.render();
            }
        } catch (err) {
            API.showToast('Failed to load maintenance records: ' + err.message, 'error');
        }
    },

    render() {
        const tbody = document.getElementById('table-maintenance-body');
        if (!tbody) return;

        if (this.records.length === 0) {
            tbody.innerHTML = '<tr><td colspan="8" style="text-align:center;">No maintenance records in singly linked list.</td></tr>';
            return;
        }

        tbody.innerHTML = this.records.map(m => {
            let statusBadge = '<span class="badge badge-warning">Pending</span>';
            if (m.status === 'In Progress') {
                statusBadge = '<span class="badge badge-inprogress">In Progress</span>';
            } else if (m.status === 'Resolved') {
                statusBadge = '<span class="badge badge-resolved">✅ Resolved</span>';
            }

            return `
                <tr>
                    <td><strong>#${m.id}</strong></td>
                    <td>${m.complaint_id > 0 ? `Complaint #${m.complaint_id}` : '<span style="color:var(--text-muted);">Preventive</span>'}</td>
                    <td><strong>${m.area_name}</strong></td>
                    <td style="max-width:280px;">${m.issue}</td>
                    <td>${m.technician || 'Unassigned'}</td>
                    <td style="font-family:monospace;font-size:12px;color:var(--text-muted);">${m.created_at}</td>
                    <td>${statusBadge}</td>
                    <td>
                        <div style="display:flex;gap:6px;">
                            <button class="btn btn-secondary btn-sm" onclick="Maintenance.openStatusModal(${m.id}, '${m.status}', '${(m.technician || '').replace(/'/g, "\\'")}')">Update</button>
                            <button class="btn btn-danger btn-sm" onclick="Maintenance.deleteRecord(${m.id})">Delete</button>
                        </div>
                    </td>
                </tr>
            `;
        }).join('');
    },

    openStatusModal(id, currentStatus, currentTech) {
        document.getElementById('update-maint-id').value = id;
        document.getElementById('update-maint-status').value = currentStatus;
        document.getElementById('update-maint-tech').value = currentTech;
        App.openModal('modal-update-maint-status');
    },

    async updateStatus(id, newStatus, technician) {
        try {
            const resp = await API.patch('/api/maintenance/status', {
                id: parseInt(id),
                status: newStatus,
                technician: technician
            });

            if (resp.success) {
                API.showToast(`Updated Work Order #${id} to ${newStatus}!`, 'success');
                App.closeModals();
                this.load();
                Dashboard.refresh();
            }
        } catch (err) {
            API.showToast(err.message, 'error');
        }
    },

    async addRecord(formData) {
        try {
            const resp = await API.post('/api/maintenance', formData);
            if (resp.success) {
                API.showToast(`Created Maintenance Work Order #${resp.data.id}!`, 'success');
                App.closeModals();
                this.load();
                Dashboard.refresh();
            }
        } catch (err) {
            API.showToast(err.message, 'error');
        }
    },

    async deleteRecord(id) {
        if (!confirm(`Delete maintenance record #${id} from singly linked list?`)) return;

        try {
            const resp = await API.delete(`/api/maintenance?id=${id}`);
            if (resp.success) {
                API.showToast(`Maintenance node #${id} deallocated and removed from linked list`, 'info');
                this.load();
                Dashboard.refresh();
            }
        } catch (err) {
            API.showToast(err.message, 'error');
        }
    },

    async purgeResolved() {
        if (!confirm('Purge and free memory for all RESOLVED maintenance records?')) return;

        try {
            const resp = await API.delete('/api/maintenance?delete_resolved=true');
            if (resp.success) {
                const count = resp.data.deleted_count || 0;
                API.showToast(`Purged ${count} resolved records from Singly Linked List memory!`, 'success');
                this.load();
                Dashboard.refresh();
            }
        } catch (err) {
            API.showToast(err.message, 'error');
        }
    }
};

window.Maintenance = Maintenance;
