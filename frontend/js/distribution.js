/**
 * Smart Water Distribution and Management System
 * Water Distribution & Allocation Engine (Priority Dispatch)
 */

const Distribution = {
    history: [],

    async load() {
        try {
            const resp = await API.get('/api/distribution');
            if (resp.success && resp.data) {
                this.history = resp.data.history || [];
                this.renderHistory();
            }
        } catch (err) {
            API.showToast('Failed to load distribution history: ' + err.message, 'error');
        }
    },

    renderHistory() {
        const tbody = document.getElementById('table-distribution-history');
        if (!tbody) return;

        if (this.history.length === 0) {
            tbody.innerHTML = '<tr><td colspan="7" style="text-align:center;">No water distribution records yet.</td></tr>';
            return;
        }

        tbody.innerHTML = this.history.map(rec => `
            <tr>
                <td><strong>#${rec.id}</strong></td>
                <td style="font-family:monospace;font-size:12px;color:var(--text-muted);">${rec.timestamp}</td>
                <td><strong>${rec.source_tank_name}</strong> (#${rec.source_tank_id})</td>
                <td><strong>${rec.dest_area_name}</strong> (#${rec.dest_area_id})</td>
                <td><strong style="color:var(--primary);">${Number(rec.amount_litres).toLocaleString()} L</strong></td>
                <td>
                    <span class="badge ${rec.mode === 'Automatic' ? 'badge-hospital' : 'badge-info'}">
                        ${rec.mode}
                    </span>
                </td>
                <td><span class="badge badge-success">${rec.status}</span></td>
            </tr>
        `).join('');
    },

    async dispatchManual(tankId, areaId, amount) {
        try {
            const resp = await API.post('/api/distribution/manual', {
                tank_id: parseInt(tankId),
                area_id: parseInt(areaId),
                amount: parseFloat(amount)
            });

            if (resp.success) {
                API.showToast(`Successfully dispatched ${Number(amount).toLocaleString()} L!`, 'success');
                document.getElementById('form-manual-dist').reset();
                document.getElementById('hint-tank-water').textContent = 'Available water: -- L';
                document.getElementById('hint-area-demand').textContent = 'Current unmet demand: -- L';

                this.load();
                Tanks.load();
                Areas.load();
                Dashboard.refresh();
            }
        } catch (err) {
            API.showToast(err.message, 'error');
        }
    },

    async dispatchAutomatic() {
        const resultContainer = document.getElementById('auto-dist-result');
        if (resultContainer) {
            resultContainer.innerHTML = '<span style="color:var(--primary);">Computing priority allocation in C backend...</span>';
        }

        try {
            const resp = await API.post('/api/distribution/automatic', {});
            if (resp.success && resp.data) {
                const allocated = resp.data.total_allocated_litres;
                const count = resp.data.areas_served;

                API.showToast(`Automatic Allocation Complete: ${Number(allocated).toLocaleString()} L across ${count} areas!`, 'success');

                if (resultContainer) {
                    resultContainer.innerHTML = `
                        <div class="alert-banner alert-info" style="margin-top:10px;">
                            <strong>✅ Automatic Allocation Summary:</strong><br>
                            Supplied: <strong>${Number(allocated).toLocaleString()} L</strong> across <strong>${count}</strong> priority consumer areas.<br>
                            <em>Critical healthcare facilities (Priority 1) were fulfilled before commercial and residential zones.</em>
                        </div>
                    `;
                }

                this.load();
                Tanks.load();
                Areas.load();
                Dashboard.refresh();
            }
        } catch (err) {
            if (resultContainer) {
                resultContainer.innerHTML = `<span style="color:var(--danger);">${err.message}</span>`;
            }
            API.showToast(err.message, 'error');
        }
    }
};

window.Distribution = Distribution;
