/* SPX - A simple profiler for PHP
 * Copyright (C) 2017-2025 Sylvain Lassaut <NoiseByNorthwest@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

function getImportUrl(path) {
    const rootUrl = new URL(import.meta.url);
    rootUrl.searchParams.set('SPX_UI_URI', path);
    return rootUrl.toString();
}

const math = await import(getImportUrl('/js/math.js'));

/**
 * HierarchicalLayout - Depth-based layout for call graphs
 *
 * Simple and fast layout that positions nodes by depth level.
 * Suitable for call graphs where parent-child relationships are clear.
 */
export class HierarchicalLayout {
    constructor(graphData, options = {}) {
        this.graphData = graphData;

        // Layout parameters
        this.nodeWidth = options.nodeWidth || 150;
        this.nodeHeight = options.nodeHeight || 40;
        this.horizontalSpacing = options.horizontalSpacing || 30;
        this.verticalSpacing = options.verticalSpacing || 80;
        this.viewportWidth = options.viewportWidth || 1000;
        this.viewportHeight = options.viewportHeight || 800;

        this.layers = [];  // depth → [nodes]
    }

    /**
     * Compute layout for all nodes
     */
    compute() {
        // 1. Group nodes by depth
        this.groupByDepth();

        // 2. Sort nodes within each layer
        this.sortLayers();

        // 3. Assign coordinates
        this.assignCoordinates();
    }

    /**
     * Group nodes by their depth in the call tree
     */
    groupByDepth() {
        const depthMap = new Map();  // depth → [nodes]

        for (const node of this.graphData.nodes.values()) {
            const depth = node.getDepth();
            if (!depthMap.has(depth)) {
                depthMap.set(depth, []);
            }
            depthMap.get(depth).push(node);
        }

        // Convert to sorted array
        const maxDepth = Math.max(...depthMap.keys());
        this.layers = [];

        for (let i = 0; i <= maxDepth; i++) {
            this.layers[i] = depthMap.get(i) || [];
        }
    }

    /**
     * Sort nodes within each layer
     */
    sortLayers() {
        for (const layer of this.layers) {
            // Sort by inclusive metric (descending)
            layer.sort((a, b) =>
                b.getInclusiveMetric(this.graphData.currentMetric) -
                a.getInclusiveMetric(this.graphData.currentMetric)
            );
        }
    }

    /**
     * Assign x, y coordinates to all nodes
     */
    assignCoordinates() {
        let y = this.verticalSpacing / 2;

        for (let depth = 0; depth < this.layers.length; depth++) {
            const layer = this.layers[depth];
            if (layer.length === 0) continue;

            const layerWidth = layer.length * (this.nodeWidth + this.horizontalSpacing) - this.horizontalSpacing;
            let x = Math.max(this.horizontalSpacing, (this.viewportWidth - layerWidth) / 2);

            for (const node of layer) {
                node.layout.x = x;
                node.layout.y = y;
                node.layout.width = this.nodeWidth;
                node.layout.height = this.nodeHeight;

                x += this.nodeWidth + this.horizontalSpacing;
            }

            y += this.nodeHeight + this.verticalSpacing;
        }
    }

    /**
     * Get total layout dimensions
     */
    getDimensions() {
        let maxX = 0;
        let maxY = 0;

        for (const node of this.graphData.nodes.values()) {
            maxX = Math.max(maxX, node.layout.x + node.layout.width);
            maxY = Math.max(maxY, node.layout.y + node.layout.height);
        }

        return {
            width: maxX + this.horizontalSpacing,
            height: maxY + this.verticalSpacing
        };
    }
}

/**
 * SugiyamaLayout - Advanced hierarchical layout with crossing minimization
 *
 * Based on the Sugiyama framework:
 * 1. Layer assignment (topological sort)
 * 2. Crossing minimization (barycenter heuristic)
 * 3. Coordinate assignment
 */
export class SugiyamaLayout {
    constructor(graphData, options = {}) {
        this.graphData = graphData;

        // Layout parameters
        this.nodeWidth = options.nodeWidth || 150;
        this.nodeHeight = options.nodeHeight || 40;
        this.horizontalSpacing = options.horizontalSpacing || 30;
        this.verticalSpacing = options.verticalSpacing || 80;
        this.viewportWidth = options.viewportWidth || 1000;

        this.layers = [];  // Array of arrays of nodes
        this.nodeToLayer = new Map();  // node → layer index
    }

    /**
     * Compute layout using Sugiyama framework
     */
    compute() {
        // 1. Assign nodes to layers (topological sort)
        this.assignLayers();

        // 2. Minimize edge crossings
        this.minimizeCrossings();

        // 3. Assign x-coordinates
        this.assignCoordinates();
    }

    /**
     * Assign nodes to layers based on longest path from roots
     */
    assignLayers() {
        const visited = new Set();
        const layerMap = new Map();  // node → layer

        const assignLayer = (node, layer) => {
            const nodeKey = node.getFunctionName();

            if (visited.has(nodeKey)) {
                // Update layer to max of current and new
                const currentLayer = layerMap.get(node);
                if (layer > currentLayer) {
                    layerMap.set(node, layer);
                }
                return;
            }

            visited.add(nodeKey);
            layerMap.set(node, layer);

            for (const child of node.children) {
                assignLayer(child, layer + 1);
            }
        };

        // Start from root nodes
        for (const root of this.graphData.rootNodes) {
            assignLayer(root, 0);
        }

        // Group nodes by layer
        const maxLayer = Math.max(...layerMap.values());
        this.layers = [];

        for (let i = 0; i <= maxLayer; i++) {
            this.layers[i] = [];
        }

        for (const [node, layer] of layerMap.entries()) {
            this.layers[layer].push(node);
            this.nodeToLayer.set(node, layer);
        }
    }

    /**
     * Minimize edge crossings using barycenter heuristic
     */
    minimizeCrossings() {
        const maxIterations = 10;

        for (let iteration = 0; iteration < maxIterations; iteration++) {
            let improved = false;

            // Forward pass (top to bottom)
            for (let i = 1; i < this.layers.length; i++) {
                if (this.sortLayerByBarycenter(this.layers[i], this.layers[i - 1], true)) {
                    improved = true;
                }
            }

            // Backward pass (bottom to top)
            for (let i = this.layers.length - 2; i >= 0; i--) {
                if (this.sortLayerByBarycenter(this.layers[i], this.layers[i + 1], false)) {
                    improved = true;
                }
            }

            if (!improved) break;
        }
    }

    /**
     * Sort layer by barycenter (average position of neighbors)
     */
    sortLayerByBarycenter(layer, neighborLayer, useParents) {
        const barycenters = new Map();
        const neighborPositions = new Map();

        // Build position map for neighbor layer
        for (let i = 0; i < neighborLayer.length; i++) {
            neighborPositions.set(neighborLayer[i], i);
        }

        // Calculate barycenter for each node
        for (const node of layer) {
            const neighbors = useParents ? node.parents : node.children;
            const relevantNeighbors = neighbors.filter(n => neighborPositions.has(n));

            if (relevantNeighbors.length === 0) {
                // No neighbors in adjacent layer, use current position
                barycenters.set(node, layer.indexOf(node));
            } else {
                const avgPos = relevantNeighbors.reduce((sum, n) =>
                    sum + neighborPositions.get(n), 0
                ) / relevantNeighbors.length;
                barycenters.set(node, avgPos);
            }
        }

        // Sort by barycenter
        const oldOrder = [...layer];
        layer.sort((a, b) => barycenters.get(a) - barycenters.get(b));

        // Check if order changed
        return !oldOrder.every((node, i) => node === layer[i]);
    }

    /**
     * Assign x-coordinates to nodes
     */
    assignCoordinates() {
        let y = this.verticalSpacing / 2;

        for (const layer of this.layers) {
            if (layer.length === 0) continue;

            const layerWidth = layer.length * (this.nodeWidth + this.horizontalSpacing) - this.horizontalSpacing;
            let x = Math.max(this.horizontalSpacing, (this.viewportWidth - layerWidth) / 2);

            for (const node of layer) {
                node.layout.x = x;
                node.layout.y = y;
                node.layout.width = this.nodeWidth;
                node.layout.height = this.nodeHeight;

                x += this.nodeWidth + this.horizontalSpacing;
            }

            y += this.nodeHeight + this.verticalSpacing;
        }
    }

    /**
     * Get total layout dimensions
     */
    getDimensions() {
        let maxX = 0;
        let maxY = 0;

        for (const node of this.graphData.nodes.values()) {
            maxX = Math.max(maxX, node.layout.x + node.layout.width);
            maxY = Math.max(maxY, node.layout.y + node.layout.height);
        }

        return {
            width: maxX + this.horizontalSpacing,
            height: maxY + this.verticalSpacing
        };
    }
}

/**
 * ForceDirectedLayout - Physics-based layout for complex graphs
 *
 * Uses force simulation to position nodes:
 * - Repulsion between all nodes
 * - Attraction along edges
 * - Damping to stabilize
 */
export class ForceDirectedLayout {
    constructor(graphData, options = {}) {
        this.graphData = graphData;

        // Simulation parameters
        this.iterations = options.iterations || 100;
        this.repulsionStrength = options.repulsionStrength || 1000;
        this.attractionStrength = options.attractionStrength || 0.01;
        this.damping = options.damping || 0.85;
        this.nodeWidth = options.nodeWidth || 150;
        this.nodeHeight = options.nodeHeight || 40;
        this.viewportWidth = options.viewportWidth || 1000;
        this.viewportHeight = options.viewportHeight || 800;
    }

    /**
     * Compute layout using force simulation
     */
    compute() {
        // Initialize random positions
        this.initializePositions();

        // Run simulation
        for (let i = 0; i < this.iterations; i++) {
            this.computeForces();
            this.updatePositions();
        }

        // Center and scale to viewport
        this.centerLayout();
    }

    /**
     * Initialize node positions randomly
     */
    initializePositions() {
        for (const node of this.graphData.nodes.values()) {
            node.layout.x = Math.random() * this.viewportWidth;
            node.layout.y = Math.random() * this.viewportHeight;
            node.layout.width = this.nodeWidth;
            node.layout.height = this.nodeHeight;
            node.velocity = { x: 0, y: 0 };
        }
    }

    /**
     * Compute repulsion and attraction forces
     */
    computeForces() {
        const nodes = Array.from(this.graphData.nodes.values());

        // Reset velocities
        for (const node of nodes) {
            node.velocity = { x: 0, y: 0 };
        }

        // Repulsion between all nodes
        for (let i = 0; i < nodes.length; i++) {
            for (let j = i + 1; j < nodes.length; j++) {
                const dx = nodes[j].layout.x - nodes[i].layout.x;
                const dy = nodes[j].layout.y - nodes[i].layout.y;
                const dist = Math.sqrt(dx * dx + dy * dy) || 1;

                const force = this.repulsionStrength / (dist * dist);
                const fx = (dx / dist) * force;
                const fy = (dy / dist) * force;

                nodes[i].velocity.x -= fx;
                nodes[i].velocity.y -= fy;
                nodes[j].velocity.x += fx;
                nodes[j].velocity.y += fy;
            }
        }

        // Attraction along edges
        for (const edge of this.graphData.edges.values()) {
            const dx = edge.to.layout.x - edge.from.layout.x;
            const dy = edge.to.layout.y - edge.from.layout.y;
            const dist = Math.sqrt(dx * dx + dy * dy) || 1;

            const force = dist * this.attractionStrength;
            const fx = (dx / dist) * force;
            const fy = (dy / dist) * force;

            edge.from.velocity.x += fx;
            edge.from.velocity.y += fy;
            edge.to.velocity.x -= fx;
            edge.to.velocity.y -= fy;
        }
    }

    /**
     * Update positions based on velocities
     */
    updatePositions() {
        for (const node of this.graphData.nodes.values()) {
            node.layout.x += node.velocity.x;
            node.layout.y += node.velocity.y;

            node.velocity.x *= this.damping;
            node.velocity.y *= this.damping;
        }
    }

    /**
     * Center layout in viewport
     */
    centerLayout() {
        let minX = Infinity, minY = Infinity;
        let maxX = -Infinity, maxY = -Infinity;

        for (const node of this.graphData.nodes.values()) {
            minX = Math.min(minX, node.layout.x);
            minY = Math.min(minY, node.layout.y);
            maxX = Math.max(maxX, node.layout.x + node.layout.width);
            maxY = Math.max(maxY, node.layout.y + node.layout.height);
        }

        const layoutWidth = maxX - minX;
        const layoutHeight = maxY - minY;

        const offsetX = (this.viewportWidth - layoutWidth) / 2 - minX;
        const offsetY = (this.viewportHeight - layoutHeight) / 2 - minY;

        for (const node of this.graphData.nodes.values()) {
            node.layout.x += offsetX;
            node.layout.y += offsetY;
        }
    }

    /**
     * Get total layout dimensions
     */
    getDimensions() {
        let maxX = 0;
        let maxY = 0;

        for (const node of this.graphData.nodes.values()) {
            maxX = Math.max(maxX, node.layout.x + node.layout.width);
            maxY = Math.max(maxY, node.layout.y + node.layout.height);
        }

        return {
            width: maxX,
            height: maxY
        };
    }
}
