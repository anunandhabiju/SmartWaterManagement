/**
 * Smart Water Distribution and Management System
 * Dashboard Visualizations and Operational Metrics
 */

const Dashboard = {
    charts: {
        tanks: null,
        demandSupply: null,
        waterBalance: null,
        complaints: null
    },

    formatLitres(val) {
        if (val === undefined || val === null) return '0 L';
        return Number(val).toLocaleString() + ' L';
    },

    async refresh() {
        try {
            const resp = await API.get('/api/dashboard');
            if (resp.success && resp.data) {
                this.render(resp.data);
            }
        } catch (err) {
            API.showToast('Failed to load dashboard metrics: ' + err.message, 'error');
        }
    },

    render(data) {
        // Update Stat Cards
        document.getElementById('stat-total-capacity').textContent = this.formatLitres(data.total_capacity_litres);
        document.getElementById('stat-available-water').textContent = this.formatLitres(data.total_available_litres);

        const availPct = data.total_capacity_litres > 0
            ? ((data.total_available_litres / data.total_capacity_litres) * 100).toFixed(1)
            : 0;
        document.getElementById('stat-available-pct').textContent = `${availPct}% stored`;

        document.getElementById('stat-distributed').textContent = this.formatLitres(data.total_distributed_litres);
        document.getElementById('stat-unmet-demand').textContent = this.formatLitres(data.total_unmet_demand_litres);
        document.getElementById('stat-fulfillment-pct').textContent = `${(data.fulfillment_percentage || 0).toFixed(1)}% demand fulfilled`;

        document.getElementById('stat-pending-complaints').textContent = data.pending_complaints_count || 0;
        document.getElementById('stat-leakage-losses').textContent = this.formatLitres(data.simulated_leakage_loss_litres || 0);

        // Update Notification Banners
        this.renderNotifications(data.notifications || []);

        // Update Charts
        this.renderTanksChart(data.tank_levels || []);
        this.renderDemandSupplyChart(data.area_demands || []);
        this.renderWaterBalanceChart(data);
        this.renderComplaintsChart(data.pending_complaints_count || 0);

        // Update Recent Activities Table
        this.renderActivities(data.recent_activities || []);
    },

    renderNotifications(notifs) {
        const container = document.getElementById('notification-container');
        if (!container) return;

        if (notifs.length === 0) {
            container.innerHTML = '';
            return;
        }

        container.innerHTML = notifs.map(n => `
            <div class="alert-banner alert-${n.level === 'danger' ? 'danger' : 'warning'}">
                <div style="display:flex;align-items:center;gap:8px;">
                    <span>${n.level === 'danger' ? '🚨' : '⚠️'}</span>
                    <span><strong>System Alert:</strong> ${n.message}</span>
                </div>
                <span class="badge badge-${n.level === 'danger' ? 'danger' : 'warning'}">Active Warning</span>
            </div>
        `).join('');
    },

    renderTanksChart(tanks) {
        const ctx = document.getElementById('chart-tanks');
        if (!ctx) return;

        const labels = tanks.map(t => t.name);
        const levels = tanks.map(t => t.current_level);
        const capacities = tanks.map(t => t.capacity);
        const backgroundColors = tanks.map(t => {
            if (t.status === 'Empty') return '#ef4444';
            if (t.status === 'Low') return '#f59e0b';
            return '#0284c7';
        });

        if (this.charts.tanks) {
            this.charts.tanks.destroy();
        }

        this.charts.tanks = new Chart(ctx, {
            type: 'bar',
            data: {
                labels,
                datasets: [
                    {
                        label: 'Current Water (L)',
                        data: levels,
                        backgroundColor: backgroundColors,
                        borderRadius: 6
                    },
                    {
                        label: 'Maximum Capacity (L)',
                        data: capacities,
                        backgroundColor: '#e2e8f0',
                        borderRadius: 6
                    }
                ]
            },
            options: {
                responsive: true,
                maintainAspectRatio: false,
                scales: {
                    y: {
                        beginAtZero: true,
                        ticks: {
                            callback: val => val >= 1000 ? (val / 1000) + 'k L' : val + ' L'
                        }
                    }
                },
                plugins: {
                    legend: { position: 'top' }
                }
            }
        });
    },

    renderDemandSupplyChart(areas) {
        const ctx = document.getElementById('chart-demand-supply');
        if (!ctx) return;

        const labels = areas.map(a => `${a.name} (#${a.id})`);
        const demands = areas.map(a => a.demand);
        const supplied = areas.map(a => a.supplied);

        if (this.charts.demandSupply) {
            this.charts.demandSupply.destroy();
        }

        this.charts.demandSupply = new Chart(ctx, {
            type: 'bar',
            data: {
                labels,
                datasets: [
                    {
                        label: 'Demanded (L)',
                        data: demands,
                        backgroundColor: '#94a3b8',
                        borderRadius: 4
                    },
                    {
                        label: 'Supplied (L)',
                        data: supplied,
                        backgroundColor: '#10b981',
                        borderRadius: 4
                    }
                ]
            },
            options: {
                responsive: true,
                maintainAspectRatio: false,
                scales: {
                    x: {
                        ticks: {
                            maxRotation: 25,
                            minRotation: 25,
                            font: { size: 10 }
                        }
                    },
                    y: {
                        beginAtZero: true,
                        ticks: {
                            callback: val => val >= 1000 ? (val / 1000) + 'k L' : val + ' L'
                        }
                    }
                },
                plugins: {
                    legend: { position: 'top' }
                }
            }
        });
    },

    renderWaterBalanceChart(data) {
        const ctx = document.getElementById('chart-water-balance');
        if (!ctx) return;

        const stored = data.total_available_litres || 0;
        const distributed = data.total_distributed_litres || 0;
        const leakage = data.simulated_leakage_loss_litres || 0;

        if (this.charts.waterBalance) {
            this.charts.waterBalance.destroy();
        }

        this.charts.waterBalance = new Chart(ctx, {
            type: 'doughnut',
            data: {
                labels: ['Available in Reservoirs', 'Distributed to Consumers', 'Simulated Leakage Loss'],
                datasets: [{
                    data: [stored, distributed, leakage],
                    backgroundColor: ['#0284c7', '#10b981', '#ef4444'],
                    borderWidth: 2
                }]
            },
            options: {
                responsive: true,
                maintainAspectRatio: false,
                plugins: {
                    legend: { position: 'bottom' }
                }
            }
        });
    },

    renderComplaintsChart(pendingCount) {
        const ctx = document.getElementById('chart-complaints-breakdown');
        if (!ctx) return;

        if (this.charts.complaints) {
            this.charts.complaints.destroy();
        }

        this.charts.complaints = new Chart(ctx, {
            type: 'pie',
            data: {
                labels: ['Pending in Circular Queue', 'Processed / In Maintenance'],
                datasets: [{
                    data: [pendingCount, Math.max(1, 5 - pendingCount)],
                    backgroundColor: ['#f59e0b', '#0d9488'],
                    borderWidth: 2
                }]
            },
            options: {
                responsive: true,
                maintainAspectRatio: false,
                plugins: {
                    legend: { position: 'bottom' }
                }
            }
        });
    },

    renderActivities(activities) {
        const tbody = document.getElementById('table-recent-activities');
        if (!tbody) return;

        if (activities.length === 0) {
            tbody.innerHTML = '<tr><td colspan="4" style="text-align:center;">No recent activities logged.</td></tr>';
            return;
        }

        tbody.innerHTML = activities.slice(0, 8).map(act => `
            <tr>
                <td style="font-family:monospace;font-size:12px;color:var(--text-muted);">${act.timestamp}</td>
                <td><span class="badge badge-info">${act.category}</span></td>
                <td>${act.description}</td>
                <td><span class="badge badge-${act.status === 'Success' ? 'success' : (act.status === 'Warning' ? 'warning' : 'danger')}">${act.status}</span></td>
            </tr>
        `).join('');
    }
};

window.Dashboard = Dashboard;
