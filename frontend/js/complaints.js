/**
 * Smart Water Distribution and Management System
 * Complaint Processing (Circular Queue Data Structure)
 */

const Complaints = {
    complaintsList: [],
    queueMeta: {
        count: 0,
        capacity: 50,
        is_full: false,
        is_empty: true,
        next_to_process: null
    },

    async load() {
        try {
            const resp = await API.get('/api/complaints');
            if (resp.success && resp.data) {
                this.complaintsList = resp.data.complaints || [];
                this.queueMeta = {
                    count: resp.data.count || 0,
                    capacity: resp.data.capacity || 50,
                    is_full: resp.data.is_full,
                    is_empty: resp.data.is_empty,
                    next_to_process: resp.data.next_to_process
                };

                this.renderTable();
                this.renderVisualQueue();
            }
        } catch (err) {
            API.showToast('Failed to load complaints: ' + err.message, 'error');
        }
    },

    renderTable() {
        const tbody = document.getElementById('table-complaints-body');
        if (!tbody) return;

        if (this.complaintsList.length === 0) {
            tbody.innerHTML = '<tr><td colspan="8" style="text-align:center;">No pending complaints in circular queue.</td></tr>';
            return;
        }

        tbody.innerHTML = this.complaintsList.map((c, idx) => {
            const isNext = (idx === 0);
            const prioBadge = c.priority === 'High'
                ? '<span class="badge badge-empty">High Priority</span>'
                : (c.priority === 'Medium' ? '<span class="badge badge-warning">Medium</span>' : '<span class="badge badge-secondary">Low</span>');

            return `
                <tr style="${isNext ? 'background:#f0fdf4;' : ''}">
                    <td>
                        ${isNext ? '<span class="badge badge-success">NEXT (FIFO)</span>' : `Queue #${idx + 1}`}
                    </td>
                    <td><strong>#${c.id}</strong></td>
                    <td style="font-family:monospace;font-size:12px;color:var(--text-muted);">${c.timestamp}</td>
                    <td><strong>${c.area_name}</strong></td>
                    <td><span class="badge badge-info">${c.issue_type}</span></td>
                    <td style="max-width:280px;">${c.description}</td>
                    <td>${prioBadge}</td>
                    <td><span class="badge badge-warning">Pending</span></td>
                </tr>
            `;
        }).join('');
    },

    renderVisualQueue() {
        const row = document.getElementById('queue-slots-row');
        if (!row) return;

        // Render visible representative slots of the circular queue buffer
        const visibleSlotsCount = 12; // Display a clean ring representation
        const count = this.queueMeta.count;

        let html = '';
        for (let i = 0; i < visibleSlotsCount; i++) {
            const isOccupied = i < count;
            const isFront = (i === 0 && count > 0);
            const isRear = (i === count - 1 && count > 0);

            let slotClasses = 'queue-slot';
            if (isOccupied) slotClasses += ' occupied';
            if (isFront) slotClasses += ' front-slot';
            if (isRear) slotClasses += ' rear-slot';

            const item = isOccupied ? this.complaintsList[i] : null;

            html += `
                <div class="${slotClasses}">
                    <div style="font-size:10px;font-weight:700;color:var(--text-muted);margin-bottom:4px;">
                        Slot [${i}]
                    </div>
                    ${item ? `
                        <div style="font-size:12px;font-weight:700;color:var(--primary-dark);">#${item.id}</div>
                        <div style="font-size:10px;color:var(--text-secondary);text-align:center;overflow:hidden;text-overflow:ellipsis;white-space:nowrap;max-width:80px;">
                            ${item.issue_type}
                        </div>
                    ` : `
                        <div style="font-size:11px;color:var(--text-muted);">Empty</div>
                    `}
                </div>
            `;
        }

        row.innerHTML = html;
    },

    async registerComplaint(formData) {
        try {
            const resp = await API.post('/api/complaints', formData);
            if (resp.success) {
                API.showToast(`Complaint #${resp.data.complaint_id} enqueued into Circular Queue!`, 'success');
                App.closeModals();
                this.load();
                Dashboard.refresh();
            }
        } catch (err) {
            API.showToast(err.message, 'error');
        }
    },

    async processNext() {
        if (this.queueMeta.count === 0) {
            API.showToast('Complaint Queue is empty! No pending complaints to process.', 'info');
            return;
        }

        try {
            const resp = await API.post('/api/complaints/process-next', {});
            if (resp.success && resp.data) {
                const compId = resp.data.processed_complaint_id;
                const maintId = resp.data.created_maintenance_id;
                const area = resp.data.area_name;

                API.showToast(`Processed Complaint #${compId} (FIFO) ➔ Created Work Order #${maintId} for ${area}!`, 'success');

                this.load();
                Maintenance.load();
                Dashboard.refresh();
            }
        } catch (err) {
            API.showToast(err.message, 'error');
        }
    }
};

window.Complaints = Complaints;
