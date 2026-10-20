/**
 * Smart Water Distribution and Management System
 * Leakage Simulation Engine
 */

const Simulation = {
    async triggerLeakage(pipelineId, lossLitres) {
        if (!pipelineId || lossLitres <= 0) {
            API.showToast('Please select a pipeline and enter a positive loss volume.', 'warning');
            return;
        }

        try {
            const resp = await API.post('/api/simulation/leakage', {
                pipeline_id: parseInt(pipelineId),
                loss_litres: parseFloat(lossLitres)
            });

            if (resp.success) {
                API.showToast(`Simulated ${Number(lossLitres).toLocaleString()} L loss on Pipeline #${pipelineId}! High-priority complaint auto-queued.`, 'warning');

                Dashboard.refresh();
                Tanks.load();
                Complaints.load();
                Maintenance.load();
                Distribution.load();
            }
        } catch (err) {
            API.showToast(err.message, 'error');
        }
    }
};

window.Simulation = Simulation;
