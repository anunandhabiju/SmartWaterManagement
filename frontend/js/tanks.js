/**
 * Smart Water Distribution and Management System
 * Water Tank Management (Arrays Data Structure)
 */

const Tanks = {
    tanksList: [],

    async load() {
        try {
            const resp = await API.get('/api/tanks');
            if (resp.success && resp.data) {
                this.tanksList = resp.data.tanks || [];
                this.render();
                this.updateDropdowns();
            }
        } catch (err) {
            API.showToast('Failed to load tanks: ' + err.message, 'error');
        }
    },

    render() {
        const tbody = document.getElementById('table-tanks-body');
        if (!tbody) return;

        if (this.tanksList.length === 0) {
            tbody.innerHTML = '<tr><td colspan="8" style="text-align:center;">No tanks registered.</td></tr>';
            return;
        }

        tbody.innerHTML = this.tanksList.map(t => {
            const pct = t.percentage.toFixed(1);
            let fillClass = '';
            let badgeClass = 'badge-normal';

            if (t.status === 'Empty') {
                fillClass = 'empty';
                badgeClass = 'badge-empty';
            } else if (t.status === 'Low') {
                fillClass = 'low';
                badgeClass = 'badge-low';
            }

            return `
                <tr>
                    <td><strong>#${t.id}</strong></td>
                    <td><strong>${t.name}</strong></td>
                    <td style="color:var(--text-muted);">${t.location || 'General Zone'}</td>
                    <td>${Number(t.capacity).toLocaleString()} L</td>
                    <td><strong>${Number(t.current_level).toLocaleString()} L</strong></td>
                    <td class="water-gauge-cell">
                        <div style="display:flex;justify-content:space-between;font-size:11.5px;font-weight:600;">
                            <span>${pct}%</span>
                            <span style="color:var(--text-muted);">${Number(t.current_level).toLocaleString()} L</span>
                        </div>
                        <div class="water-bar-bg">
                            <div class="water-bar-fill ${fillClass}" style="width: ${Math.min(100, pct)}%;"></div>
                        </div>
                    </td>
                    <td><span class="badge ${badgeClass}">${t.status}</span></td>
                    <td>
                        <button class="btn btn-danger btn-sm" onclick="Tanks.deleteTank(${t.id}, '${t.name.replace(/'/g, "\\'")}')">Delete</button>
                    </td>
                </tr>
            `;
        }).join('');
    },

    updateDropdowns() {
        // Refill tank dropdown
        const refillSelect = document.getElementById('refill-tank-id');
        if (refillSelect) {
            refillSelect.innerHTML = this.tanksList.map(t =>
                `<option value="${t.id}">${t.name} (Available: ${Number(t.current_level).toLocaleString()} L / ${Number(t.capacity).toLocaleString()} L)</option>`
            ).join('');
        }

        // Distribution source tank dropdown
        const distTankSelect = document.getElementById('dist-source-tank');
        if (distTankSelect) {
            distTankSelect.innerHTML = '<option value="">-- Choose Reservoir --</option>' +
                this.tanksList.map(t =>
                    `<option value="${t.id}" data-level="${t.current_level}">${t.name} (${Number(t.current_level).toLocaleString()} L available)</option>`
                ).join('');
        }
    },

    async addTank(formData) {
        try {
            const resp = await API.post('/api/tanks', formData);
            if (resp.success) {
                API.showToast('New water tank created successfully!', 'success');
                App.closeModals();
                this.load();
                Dashboard.refresh();
            }
        } catch (err) {
            API.showToast(err.message, 'error');
        }
    },

    async refillWater(tankId, amount) {
        try {
            const resp = await API.post('/api/tanks/add-water', { tank_id: tankId, amount });
            if (resp.success) {
                API.showToast(`Successfully added ${Number(amount).toLocaleString()} L of water!`, 'success');
                App.closeModals();
                this.load();
                Dashboard.refresh();
            }
        } catch (err) {
            API.showToast(err.message, 'error');
        }
    },

    async deleteTank(tankId, tankName) {
        if (!confirm(`Are you sure you want to decommission and delete "${tankName}" (#${tankId})?`)) {
            return;
        }

        try {
            const resp = await API.delete(`/api/tanks?id=${tankId}`);
            if (resp.success) {
                API.showToast(`Tank #${tankId} decommissioned successfully`, 'warning');
                this.load();
                Dashboard.refresh();
            }
        } catch (err) {
            API.showToast(err.message, 'error');
        }
    }
};

window.Tanks = Tanks;
