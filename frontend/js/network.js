/**
 * Smart Water Distribution and Management System
 * Pipeline Network Graph Visualization (Cytoscape.js & C Graph Algorithms)
 */

const Network = {
    cy: null,
    graphData: null,

    async load() {
        try {
            const resp = await API.get('/api/network');
            if (resp.success && resp.data) {
                this.graphData = resp.data;
                this.renderGraph();
                this.updatePipelineDropdowns();
            }
        } catch (err) {
            API.showToast('Failed to load pipeline network: ' + err.message, 'error');
        }
    },

    renderGraph() {
        const container = document.getElementById('cy-network');
        if (!container || !window.cytoscape) return;

        const elements = [];

        // Build Cytoscape Nodes
        (this.graphData.nodes || []).forEach(n => {
            elements.push({
                group: 'nodes',
                data: {
                    id: String(n.id),
                    label: `${n.name}\n(#${n.id})`,
                    type: n.type,
                    rawId: n.id
                },
                position: { x: n.x, y: n.y }
            });
        });

        // Build Cytoscape Edges
        (this.graphData.edges || []).forEach(e => {
            elements.push({
                group: 'edges',
                data: {
                    id: `e-${e.edge_id}`,
                    edgeId: e.edge_id,
                    source: String(e.source),
                    target: String(e.target),
                    distance: e.distance_m,
                    label: `${e.distance_m}m (${e.loss_rate}%)`,
                    is_active: e.is_active
                }
            });
        });

        if (this.cy) {
            this.cy.destroy();
        }

        this.cy = cytoscape({
            container: container,
            elements: elements,
            layout: {
                name: 'preset',
                fit: true,
                padding: 40
            },
            style: [
                {
                    selector: 'node',
                    style: {
                        'label': 'data(label)',
                        'text-valign': 'bottom',
                        'text-margin-y': 6,
                        'font-size': '11px',
                        'font-weight': 600,
                        'color': '#1e293b',
                        'text-wrap': 'wrap',
                        'text-max-width': '100px',
                        'border-width': 2,
                        'border-color': '#ffffff'
                    }
                },
                {
                    selector: 'node[type = "Reservoir"]',
                    style: {
                        'shape': 'hexagon',
                        'background-color': '#0284c7',
                        'width': 50,
                        'height': 50
                    }
                },
                {
                    selector: 'node[type = "Junction"]',
                    style: {
                        'shape': 'ellipse',
                        'background-color': '#0d9488',
                        'width': 36,
                        'height': 36
                    }
                },
                {
                    selector: 'node[type = "Hospital"]',
                    style: {
                        'shape': 'diamond',
                        'background-color': '#8b5cf6',
                        'width': 45,
                        'height': 45
                    }
                },
                {
                    selector: 'node[type = "Commercial"]',
                    style: {
                        'shape': 'rectangle',
                        'background-color': '#3b82f6',
                        'width': 40,
                        'height': 40
                    }
                },
                {
                    selector: 'node[type = "Residential"]',
                    style: {
                        'shape': 'round-rectangle',
                        'background-color': '#64748b',
                        'width': 40,
                        'height': 40
                    }
                },
                {
                    selector: 'edge',
                    style: {
                        'width': 3,
                        'line-color': '#94a3b8',
                        'curve-style': 'bezier',
                        'label': 'data(label)',
                        'font-size': '9px',
                        'color': '#64748b',
                        'text-background-opacity': 0.8,
                        'text-background-color': '#ffffff',
                        'text-background-padding': 2,
                        'text-background-shape': 'roundrectangle'
                    }
                },
                {
                    selector: 'edge[!is_active]',
                    style: {
                        'line-color': '#ef4444',
                        'line-style': 'dashed',
                        'width': 3,
                        'opacity': 0.6
                    }
                },
                {
                    selector: '.bfs-highlight',
                    style: {
                        'line-color': '#f59e0b',
                        'width': 6,
                        'background-color': '#f59e0b',
                        'z-index': 99
                    }
                },
                {
                    selector: '.isolated-node',
                    style: {
                        'background-color': '#ef4444',
                        'border-color': '#b91c1c',
                        'border-width': 4
                    }
                }
            ]
        });

        this.cy.fit();
    },

    updatePipelineDropdowns() {
        const simSelect = document.getElementById('sim-fail-pipe');
        const leakSelect = document.getElementById('sim-leak-pipeline');

        if (!this.graphData || !this.graphData.edges) return;

        const options = this.graphData.edges.map(e => {
            const srcNode = (this.graphData.nodes || []).find(n => n.id === e.source);
            const tgtNode = (this.graphData.nodes || []).find(n => n.id === e.target);
            const srcName = srcNode ? srcNode.name : `#${e.source}`;
            const tgtName = tgtNode ? tgtNode.name : `#${e.target}`;
            return `<option value="${e.edge_id}">Pipe #${e.edge_id}: ${srcName} ⟷ ${tgtName} (${e.distance_m}m)</option>`;
        }).join('');

        if (simSelect) simSelect.innerHTML = options;
        if (leakSelect) leakSelect.innerHTML = options;
    },

    async runBFS(startId, targetId) {
        if (!startId || !targetId) {
            API.showToast('Please select start and target nodes for BFS route discovery', 'warning');
            return;
        }

        try {
            const resp = await API.post('/api/network/bfs', {
                start_id: parseInt(startId),
                target_id: parseInt(targetId)
            });

            if (resp.success && resp.data) {
                const bfs = resp.data;
                const banner = document.getElementById('graph-algo-banner');
                banner.style.display = 'block';

                if (!bfs.reachable) {
                    banner.innerHTML = `
                        <div class="alert-banner alert-danger">
                            <strong>❌ No Path Found via BFS:</strong> Target area is NOT reachable from this reservoir due to broken or disconnected pipelines.
                        </div>
                    `;
                    return;
                }

                banner.innerHTML = `
                    <div class="alert-banner alert-info" style="display:flex;justify-content:space-between;align-items:center;">
                        <div>
                            <strong>✅ BFS Shortest Hop Route Found in C Backend:</strong><br>
                            <strong>Route:</strong> ${bfs.path.join(' ➔ ')}<br>
                            <strong>Total Distance:</strong> ${Number(bfs.total_distance_m).toLocaleString()} meters (${bfs.hop_count} hops)
                        </div>
                        <button class="btn btn-sm btn-secondary" onclick="Network.clearHighlights(); document.getElementById('graph-algo-banner').style.display='none';">Clear</button>
                    </div>
                `;

                // Highlight BFS Path in Cytoscape
                this.clearHighlights();
                for (let i = 0; i < bfs.path.length; i++) {
                    const node = this.cy.getElementById(String(bfs.path[i]));
                    if (node) node.addClass('bfs-highlight');

                    if (i < bfs.path.length - 1) {
                        const u = String(bfs.path[i]);
                        const v = String(bfs.path[i + 1]);
                        const edge = this.cy.edges().filter(e =>
                            (e.data('source') === u && e.data('target') === v) ||
                            (e.data('source') === v && e.data('target') === u)
                        );
                        if (edge) edge.addClass('bfs-highlight');
                    }
                }
            }
        } catch (err) {
            API.showToast(err.message, 'error');
        }
    },

    async runFailureSimulation(disabledEdgeId, reservoirId) {
        if (!disabledEdgeId || !reservoirId) return;

        try {
            const resp = await API.post('/api/network/failure-simulation', {
                disabled_edge_id: parseInt(disabledEdgeId),
                reservoir_id: parseInt(reservoirId)
            });

            if (resp.success && resp.data) {
                const sim = resp.data;
                const banner = document.getElementById('graph-algo-banner');
                banner.style.display = 'block';

                banner.innerHTML = `
                    <div class="alert-banner alert-warning" style="display:flex;justify-content:space-between;align-items:center;">
                        <div>
                            <strong>⚠️ Pipeline Cut Simulation Results (C Backend):</strong><br>
                            Disabling Pipeline #${disabledEdgeId} isolates <strong>${sim.unreachable_count} areas</strong> from Reservoir #${reservoirId}!<br>
                            <strong>Disconnected Areas:</strong> ${sim.unreachable_ids.length > 0 ? sim.unreachable_ids.map(id => '#' + id).join(', ') : 'None (redundant path available)'}
                        </div>
                        <button class="btn btn-sm btn-secondary" onclick="Network.clearHighlights(); document.getElementById('graph-algo-banner').style.display='none';">Clear</button>
                    </div>
                `;

                this.clearHighlights();

                // Highlight the disabled edge
                const edge = this.cy.getElementById(`e-${disabledEdgeId}`);
                if (edge) edge.addClass('bfs-highlight');

                // Highlight unreachable nodes in warning red
                (sim.unreachable_ids || []).forEach(id => {
                    const node = this.cy.getElementById(String(id));
                    if (node) node.addClass('isolated-node');
                });
            }
        } catch (err) {
            API.showToast(err.message, 'error');
        }
    },

    async reconnectAllPipelines() {
        if (!this.graphData || !this.graphData.edges) return;

        try {
            for (const e of this.graphData.edges) {
                if (!e.is_active) {
                    await API.patch('/api/network/pipelines/toggle', {
                        edge_id: e.edge_id,
                        is_active: true
                    });
                }
            }
            API.showToast('All pipeline valves reopened and connections active!', 'success');
            this.clearHighlights();
            document.getElementById('graph-algo-banner').style.display = 'none';
            this.load();
            Dashboard.refresh();
        } catch (err) {
            API.showToast(err.message, 'error');
        }
    },

    clearHighlights() {
        if (!this.cy) return;
        this.cy.elements().removeClass('bfs-highlight isolated-node');
    }
};

window.Network = Network;
