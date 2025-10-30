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
 * CallGraphNode - Represents a function node in the call graph
 *
 * Wraps a CallTreeStatsNode and adds graph-specific properties
 * like layout coordinates, hot path membership, and parent/child relationships.
 */
export class CallGraphNode {
    constructor(callTreeNode) {
        this.callTreeNode = callTreeNode;      // Original CallTreeStatsNode
        this.children = [];                     // Array of CallGraphNode children
        this.parents = [];                      // Array of CallGraphNode parents
        this.isOnHotPath = false;               // Is this node on the critical path?
        this.hotPathScore = 0.0;                // [0.0-1.0] hot path intensity
        this.criticalPathContribution = 0;      // Contribution to critical path
        this.hotPathRank = 0;                   // Rank in hot paths (1 = hottest)
        this.hotPathNext = null;                // Next node on hot path

        // Layout coordinates (set by layout algorithm)
        this.layout = {
            x: 0,
            y: 0,
            width: 0,
            height: 0
        };
    }

    // Delegate to CallTreeStatsNode
    getFunctionName() {
        return this.callTreeNode.getFunctionName();
    }

    getDepth() {
        return this.callTreeNode.getDepth();
    }

    getCalled() {
        return this.callTreeNode.getCalled();
    }

    getInc() {
        return this.callTreeNode.getInc();
    }

    getInclusiveMetric(metric) {
        return this.callTreeNode.getInc().getValue(metric);
    }

    getExclusiveMetric(metric) {
        // Calculate exclusive by subtracting children's inclusive
        let exclusive = this.getInclusiveMetric(metric);
        for (const child of this.children) {
            exclusive -= child.getInclusiveMetric(metric);
        }
        return Math.max(0, exclusive);
    }

    getMaxCumInc() {
        return this.callTreeNode.getMaxCumInc();
    }
}

/**
 * CallGraphEdge - Represents a directed edge between two function nodes
 *
 * Captures the call relationship, call count, and metrics for this specific edge.
 */
export class CallGraphEdge {
    constructor(from, to, callCount, metrics) {
        this.from = from;                       // CallGraphNode (caller)
        this.to = to;                           // CallGraphNode (callee)
        this.callCount = callCount;             // Number of calls
        this.metrics = metrics;                 // MetricValueSet for this edge
        this.isOnHotPath = false;               // Is this edge on the critical path?
        this.hotPathScore = 0.0;                // [0.0-1.0] hot path intensity
    }
}

/**
 * CallGraphData - Main call graph data structure
 *
 * Builds a directed graph from the call tree, calculates hot paths,
 * and provides methods for sorting and filtering nodes.
 */
export class CallGraphData {
    constructor(profileData, timeRange, currentMetric) {
        this.profileData = profileData;
        this.timeRange = timeRange;
        this.currentMetric = currentMetric;

        this.nodes = new Map();                 // functionName → CallGraphNode
        this.edges = new Map();                 // "from->to" → CallGraphEdge
        this.rootNodes = [];                    // Entry points
        this.hotPath = [];                      // Ordered array of nodes on hot path
        this.criticalPathLength = 0;            // Total metric value of hot path
        this.viewportWidth = 1000;              // Default viewport width for layout

        this.build();
    }

    /**
     * Build the graph from profile data
     */
    build() {
        // 1. Get call tree from profile data
        const callTreeStats = this.profileData
            .getTimeRangeStats(this.timeRange)
            .getCallTreeStats();

        const cgRoot = callTreeStats.getRoot();

        // 2. Build graph structure
        this.traverseAndBuildGraph(cgRoot, null);

        // 3. Calculate hot path
        this.calculateHotPath();
    }

    /**
     * Recursively traverse call tree and build graph
     */
    traverseAndBuildGraph(callTreeNode, parentGraphNode) {
        const functionName = callTreeNode.getFunctionName();

        // Root node has null function name, skip it
        if (functionName === null) {
            // Process children as root nodes
            for (const childCallTreeNode of callTreeNode.getChildren()) {
                this.traverseAndBuildGraph(childCallTreeNode, null);
            }
            return null;
        }

        // Create or get graph node
        let graphNode = this.nodes.get(functionName);
        if (!graphNode) {
            graphNode = new CallGraphNode(callTreeNode);
            this.nodes.set(functionName, graphNode);
        } else {
            // Merge statistics if node already exists (recursive calls)
            // For now, we keep the first occurrence's callTreeNode
            // In a more sophisticated version, we'd aggregate stats
        }

        // Create edge from parent
        if (parentGraphNode) {
            const edgeKey = `${parentGraphNode.getFunctionName()}->${functionName}`;
            if (!this.edges.has(edgeKey)) {
                const edge = new CallGraphEdge(
                    parentGraphNode,
                    graphNode,
                    callTreeNode.getCalled(),
                    callTreeNode.getInc()
                );
                this.edges.set(edgeKey, edge);

                // Update parent-child relationships
                if (!parentGraphNode.children.includes(graphNode)) {
                    parentGraphNode.children.push(graphNode);
                }
                if (!graphNode.parents.includes(parentGraphNode)) {
                    graphNode.parents.push(parentGraphNode);
                }
            }
        } else {
            // This is a root node (no parent)
            if (!this.rootNodes.includes(graphNode)) {
                this.rootNodes.push(graphNode);
            }
        }

        // Recurse into children
        for (const childCallTreeNode of callTreeNode.getChildren()) {
            this.traverseAndBuildGraph(childCallTreeNode, graphNode);
        }

        return graphNode;
    }

    /**
     * Calculate hot path using dynamic programming
     *
     * Finds the critical path (longest weighted path from root to leaf)
     * using the current metric as edge weights.
     */
    calculateHotPath() {
        const visited = new Set();
        const pathScores = new Map();

        // Dynamic programming: for each node, calculate max path score
        const computeMaxPath = (node) => {
            if (visited.has(node)) {
                return pathScores.get(node);
            }

            visited.add(node);

            const nodeValue = node.getInclusiveMetric(this.currentMetric);
            let maxChildPath = 0;
            let maxChild = null;

            for (const child of node.children) {
                const childPath = computeMaxPath(child);
                if (childPath > maxChildPath) {
                    maxChildPath = childPath;
                    maxChild = child;
                }
            }

            const totalPath = nodeValue + maxChildPath;
            pathScores.set(node, totalPath);

            // Mark hot path next pointer
            if (maxChild) {
                node.hotPathNext = maxChild;
            }

            return totalPath;
        };

        // Find root with maximum path
        let maxRootPath = 0;
        let hotRoot = null;

        for (const root of this.rootNodes) {
            const pathScore = computeMaxPath(root);
            if (pathScore > maxRootPath) {
                maxRootPath = pathScore;
                hotRoot = root;
            }
        }

        // Trace hot path
        this.hotPath = [];
        this.criticalPathLength = maxRootPath;
        let current = hotRoot;
        let rank = 1;

        while (current) {
            current.isOnHotPath = true;
            current.hotPathScore = current.getInclusiveMetric(this.currentMetric) / Math.max(1, maxRootPath);
            current.criticalPathContribution = current.getInclusiveMetric(this.currentMetric);
            current.hotPathRank = rank++;

            this.hotPath.push(current);

            // Mark edge as hot
            if (current.hotPathNext) {
                const edgeKey = `${current.getFunctionName()}->${current.hotPathNext.getFunctionName()}`;
                const edge = this.edges.get(edgeKey);
                if (edge) {
                    edge.isOnHotPath = true;
                    edge.hotPathScore = current.hotPathScore;
                }
            }

            current = current.hotPathNext;
        }
    }

    /**
     * Get nodes sorted by specified criteria
     */
    getSortedNodes(sortBy) {
        const nodes = Array.from(this.nodes.values());

        switch (sortBy) {
            case 'calls':
                return nodes.sort((a, b) => b.getCalled() - a.getCalled());

            case 'inc':
                return nodes.sort((a, b) =>
                    b.getInclusiveMetric(this.currentMetric) - a.getInclusiveMetric(this.currentMetric)
                );

            case 'exc':
                return nodes.sort((a, b) =>
                    b.getExclusiveMetric(this.currentMetric) - a.getExclusiveMetric(this.currentMetric)
                );

            case 'name':
                return nodes.sort((a, b) =>
                    a.getFunctionName().localeCompare(b.getFunctionName())
                );

            case 'hotpath':
                return nodes.sort((a, b) => {
                    // Sort by hot path first, then by score
                    if (a.isOnHotPath && !b.isOnHotPath) return -1;
                    if (!a.isOnHotPath && b.isOnHotPath) return 1;
                    return b.hotPathScore - a.hotPathScore;
                });

            default:
                return nodes;
        }
    }

    /**
     * Get top N functions by specified sort criteria
     */
    getTopFunctions(n, sortBy = 'inc') {
        return this.getSortedNodes(sortBy).slice(0, n);
    }

    /**
     * Filter graph by threshold (Blackfire-style pruning)
     *
     * Removes nodes that contribute less than minPercentage to the total
     */
    filterByThreshold(minPercentage) {
        const maxValue = this.criticalPathLength;
        const threshold = maxValue * (minPercentage / 100);

        const filteredNodes = new Map();
        const filteredEdges = new Map();

        // Filter nodes
        for (const [name, node] of this.nodes.entries()) {
            if (node.getInclusiveMetric(this.currentMetric) >= threshold) {
                filteredNodes.set(name, node);
            }
        }

        // Filter edges
        for (const [key, edge] of this.edges.entries()) {
            if (filteredNodes.has(edge.from.getFunctionName()) &&
                filteredNodes.has(edge.to.getFunctionName())) {
                filteredEdges.set(key, edge);
            }
        }

        // Create filtered graph data
        const filtered = Object.create(Object.getPrototypeOf(this));
        filtered.profileData = this.profileData;
        filtered.timeRange = this.timeRange;
        filtered.currentMetric = this.currentMetric;
        filtered.nodes = filteredNodes;
        filtered.edges = filteredEdges;
        filtered.viewportWidth = this.viewportWidth;

        // Update root nodes
        filtered.rootNodes = this.rootNodes.filter(node =>
            filteredNodes.has(node.getFunctionName())
        );

        // Keep hot path info
        filtered.hotPath = this.hotPath.filter(node =>
            filteredNodes.has(node.getFunctionName())
        );
        filtered.criticalPathLength = this.criticalPathLength;

        return filtered;
    }

    /**
     * Search nodes by function name
     */
    searchByName(query) {
        const lowerQuery = query.toLowerCase();
        const results = [];

        for (const node of this.nodes.values()) {
            if (node.getFunctionName().toLowerCase().includes(lowerQuery)) {
                results.push(node);
            }
        }

        return results;
    }

    /**
     * Get nodes on hot path
     */
    getHotPathNodes() {
        return this.hotPath;
    }

    /**
     * Get summary statistics
     */
    getStats() {
        return {
            nodeCount: this.nodes.size,
            edgeCount: this.edges.size,
            rootCount: this.rootNodes.length,
            hotPathLength: this.hotPath.length,
            criticalPathValue: this.criticalPathLength,
            maxDepth: Math.max(...Array.from(this.nodes.values()).map(n => n.getDepth()))
        };
    }
}

/**
 * Hot Path Analyzer - Advanced hot path detection
 *
 * Finds multiple hot paths (not just the single hottest)
 */
export class HotPathAnalyzer {
    constructor(graphData, metric) {
        this.graphData = graphData;
        this.metric = metric;
        this.criticalPaths = [];  // Multiple hot paths (top N)
    }

    /**
     * Analyze and find top N critical paths
     */
    analyzePaths(topN = 5) {
        const allPaths = this.findAllPaths();

        // Sort by total metric value
        allPaths.sort((a, b) => b.totalMetric - a.totalMetric);

        this.criticalPaths = allPaths.slice(0, topN);

        // Assign hot path scores
        const maxMetric = this.criticalPaths[0]?.totalMetric || 1;

        for (let i = 0; i < this.criticalPaths.length; i++) {
            const path = this.criticalPaths[i];
            const pathScore = path.totalMetric / maxMetric;

            for (const node of path.nodes) {
                node.hotPathScore = Math.max(
                    node.hotPathScore || 0,
                    pathScore * (1 - i * 0.15)  // Diminishing for lower-ranked paths
                );
                node.isOnHotPath = true;
                node.hotPathRank = Math.min(node.hotPathRank || i + 1, i + 1);
            }
        }

        return this.criticalPaths;
    }

    /**
     * Find all paths from roots to leaves
     */
    findAllPaths() {
        const paths = [];
        const visited = new Set();

        const dfs = (node, currentPath, currentMetric) => {
            const nodeKey = node.getFunctionName();

            // Detect cycles
            if (visited.has(nodeKey)) {
                return;
            }

            const newPath = [...currentPath, node];
            const newMetric = currentMetric + node.getInclusiveMetric(this.metric);

            if (node.children.length === 0) {
                // Leaf node - complete path
                paths.push({
                    nodes: newPath,
                    totalMetric: newMetric,
                    depth: newPath.length
                });
            } else {
                visited.add(nodeKey);
                for (const child of node.children) {
                    dfs(child, newPath, newMetric);
                }
                visited.delete(nodeKey);
            }
        };

        for (const root of this.graphData.rootNodes) {
            dfs(root, [], 0);
        }

        return paths;
    }

    /**
     * Get summary of hot paths
     */
    getHotPathSummary() {
        return this.criticalPaths.map((path, i) => ({
            rank: i + 1,
            functions: path.nodes.map(n => n.getFunctionName()),
            totalMetric: path.totalMetric,
            percentage: (path.totalMetric / this.graphData.criticalPathLength) * 100,
            depth: path.depth
        }));
    }
}
