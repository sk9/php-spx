# SVG Call Graph Visualization Design

**Version:** 1.0
**Date:** 2025-10-30
**Status:** Design Phase - 3 Iterations Complete

---

## Executive Summary

This document presents a comprehensive design for an SVG-based call graph visualization system for PHP-SPX, inspired by industry-leading profilers (Blackfire, Speedscope, py-spy) and modern D3.js flamegraph implementations. The design introduces three complementary visualization modes, advanced hot path detection, and interactive features while maintaining backward compatibility with the existing flamegraph.

**Key Innovations:**
- **Hot Path Visualization**: Blackfire-inspired red color gradient highlighting critical execution paths
- **Multi-View Architecture**: Three visualization modes (Call Graph, Flamegraph, Timeline)
- **Advanced Sorting**: Sort by call count, inclusive/exclusive time, memory usage
- **Performance Optimizations**: SVG node pooling, viewport culling, lazy rendering (10-20x improvement for large profiles)
- **Interactive Features**: Zoom, pan, search, function filtering, metric switching

---

## Research Findings: State-of-the-Art Profiler Visualizations

### 1. Blackfire Profiler
**Key Learnings:**
- **Hot Path Detection**: Uses red color intensity to highlight the most time-consuming call chain (critical path)
- **Graph Pruning**: Automatically prunes nodes consuming <1% of global costs across all dimensions
- **Node Grouping**: Groups adjacent "proxy nodes" to simplify visualization
- **Directed Acyclic Graph (DAG)**: Edges represent metrics, nodes represent function calls
- **Dual Visualization**: Offers both call-graphs and timeline views

### 2. D3.js Flamegraph Libraries (spiermar/d3-flame-graph, cimi/d3-flame-graphs)
**Key Learnings:**
- **Interactive Zooming**: Click to zoom into subgraphs, with breadcrumb navigation
- **Performance Optimization**: Only render visible samples (10-20x speedup on large profiles)
- **Tooltip System**: Contextual information on hover with customizable callbacks
- **Inverted Layouts**: Support for icicle plots (inverted flamegraphs)
- **SVG-Based**: Fully scalable vector graphics with smooth interactions

### 3. Speedscope (py-spy output format)
**Key Learnings:**
- **Three View Modes**:
  - **Time Order**: Displays frames in chronological order
  - **Left Heavy**: Heavy frames on left (flamegraph-style)
  - **Sandwich View**: Shows both callers and callees
- **JSON-Based**: Structured data format for portability
- **Sampling-Based**: Works with sampling profilers (100 samples/sec default)

### 4. Brendan Gregg's Flamegraphs (Industry Standard)
**Key Learnings:**
- **X-axis = Metric (not time)**: Width represents accumulated metric value (time/memory)
- **Y-axis = Stack Depth**: Each level is a stack frame
- **Interactive SVG**: Search highlighting, zoom, tooltips
- **Color Coding**: Random jitter for visual separation, optional heatmap mode
- **Designed for Quick Identification**: Most frequent code paths immediately visible

### 5. py-spy (Rust-Based Python Profiler)
**Key Learnings:**
- **Extremely Low Overhead**: Written in Rust, separate process from profiled program
- **Multiple Output Formats**: Flamegraph SVG, Speedscope JSON, raw data
- **Sampling-Based**: Default 100Hz sampling rate
- **Real-Time**: Can attach to running processes

---

## Current Implementation Analysis

### Architecture (assets/web-ui/)
```
report.html
├── profileData.js      → Data loading & call tree construction
├── widget.js           → Visualization widgets (FlameGraph, Timeline, FlatProfile)
├── svg.js              → SVG utilities & node pooling
├── math.js             → Vec3, lerp, color interpolation
├── utils.js            → Function categorization, truncation
├── fmt.js              → Metric formatting (time, memory, quantity)
└── dataTable.js        → Sortable data tables
```

### Current FlameGraph Implementation (widget.js:1445-1648)
**Strengths:**
- ✅ SVG-based with efficient node pooling (`svgRectPool`, `svgTextPool`)
- ✅ Interactive hover with stroke highlighting (`#0ff` cyan border)
- ✅ Info viewport showing function details (name, depth, calls, metrics)
- ✅ Category-based coloring with depth/metric interpolation
- ✅ Dynamic sizing based on inclusive metric values
- ✅ Search integration via `spx-highlighted-function-update` event
- ✅ Handles releasable metrics gracefully

**Limitations Identified:**
- ❌ No hot path detection or highlighting
- ❌ No call count sorting or alternative sort orders
- ❌ No zoom/pan functionality (only resize via layout splitters)
- ❌ No call graph view (only flamegraph hierarchy)
- ❌ No edge visualization showing call relationships
- ❌ Limited color schemes (HSV interpolation only)
- ❌ No viewport culling for extremely large profiles
- ❌ No breadcrumb navigation for drilldown

### Data API Compatibility (VERIFIED ✅)
After refactoring, all endpoints remain functional:
- `/data/metrics` → JSON array of available metrics (php_spx.c:874-918)
- `/data/reports/metadata` → List of report metadata files (php_spx.c:921-934)
- `/data/reports/metadata/{key}` → Individual report metadata (php_spx.c:936-946)
- `/data/reports/get/{key}` → Compressed report data .txt.gz (php_spx.c:948-957)

**Data Format:**
- **Events**: Space-separated numeric values (time, metrics, function indices)
- **Functions**: Newline-separated function names
- **Metadata**: JSON with exec_ts, host, process info, enabled_metrics, call counts, wall_time, memory

**Refactoring Impact:** ✅ **NO BREAKING CHANGES**
- HTTP UI handler intact (php_spx.c:816-1070)
- Path validation enhanced with `spx_validate_path()` (security++)
- Content-Type detection unchanged
- Streaming with `read_stream_content()` preserved
- Security improvements (constant-time comparison, safe string operations)

---

# DESIGN ITERATION 1: Core SVG Call Graph Architecture

## 1.1 Objectives
- Design a **directed graph visualization** showing function call relationships
- Maintain **backward compatibility** with existing data APIs
- Create **extensible architecture** for multiple visualization modes
- Establish **data model** for hot path calculations

## 1.2 Core Architecture

### Multi-Visualization System
```
┌─────────────────────────────────────────────────────────────┐
│                    report.html (Main UI)                     │
├─────────────────────────────────────────────────────────────┤
│  Controls: Metric Selector | View Mode | Search | Filters   │
├─────────────────────────────────────────────────────────────┤
│                                                               │
│  ┌───────────────────────────────────────────────────────┐  │
│  │         Visualization Container (Switchable)          │  │
│  │                                                         │  │
│  │  Mode 1: CALL GRAPH (NEW)                             │  │
│  │    - Directed graph with nodes and edges              │  │
│  │    - Hot path highlighting (red gradient)             │  │
│  │    - Layout: Hierarchical top-down                    │  │
│  │                                                         │  │
│  │  Mode 2: FLAMEGRAPH (Enhanced)                        │  │
│  │    - Existing implementation + zoom                   │  │
│  │    - Hot path overlay                                 │  │
│  │                                                         │  │
│  │  Mode 3: TIMELINE (Enhanced)                          │  │
│  │    - Temporal view with call stacks                   │  │
│  │    - Scrubbing with flamegraph sync                   │  │
│  └───────────────────────────────────────────────────────┘  │
│                                                               │
│  ┌───────────────────────────────────────────────────────┐  │
│  │            FlatProfile Table (Enhanced)               │  │
│  │  - Sort by: Calls | Inc. Time | Exc. Time | Memory   │  │
│  │  - Click to highlight in visualization                │  │
│  └───────────────────────────────────────────────────────┘  │
└─────────────────────────────────────────────────────────────┘
```

### Data Model Extensions

#### CallGraphNode (New Class)
```javascript
class CallGraphNode {
    constructor(cgNode, stats) {
        this.cgNode = cgNode;              // Original call tree node
        this.stats = stats;                // Statistics (inc, exc, calls)
        this.children = [];                // Direct callees
        this.parents = [];                 // Direct callers
        this.isOnHotPath = false;          // Hot path membership
        this.hotPathScore = 0.0;           // [0.0-1.0] hot path intensity
        this.criticalPathContribution = 0; // Contribution to critical path
        this.layout = {                    // Layout coordinates
            x: 0,
            y: 0,
            width: 0,
            height: 0
        };
    }

    getFunctionName() { return this.cgNode.getFunctionName(); }
    getDepth() { return this.cgNode.getDepth(); }
    getCalled() { return this.cgNode.getCalled(); }
    getInclusiveMetric(metric) { return this.cgNode.getInc().getValue(metric); }
    getExclusiveMetric(metric) { return this.cgNode.getExc().getValue(metric); }
}
```

#### CallGraphEdge (New Class)
```javascript
class CallGraphEdge {
    constructor(from, to, callCount, metrics) {
        this.from = from;                  // CallGraphNode (caller)
        this.to = to;                      // CallGraphNode (callee)
        this.callCount = callCount;        // Number of calls
        this.metrics = metrics;            // MetricValueSet for this edge
        this.isOnHotPath = false;          // Hot path membership
        this.hotPathScore = 0.0;           // [0.0-1.0] hot path intensity
    }
}
```

#### CallGraphData (New Class)
```javascript
class CallGraphData {
    constructor(profileData, timeRange, metric) {
        this.profileData = profileData;
        this.timeRange = timeRange;
        this.currentMetric = metric;

        this.nodes = new Map();            // functionName → CallGraphNode
        this.edges = new Map();            // "from->to" → CallGraphEdge
        this.rootNodes = [];               // Entry points
        this.hotPath = [];                 // Ordered array of nodes on hot path
        this.criticalPathLength = 0;       // Total metric value of hot path

        this.build();
    }

    build() {
        // 1. Build node graph from call tree
        const cgRoot = this.profileData
            .getTimeRangeStats(this.timeRange)
            .getCallTreeStats(this.timeRange)
            .getRoot();

        this.traverseAndBuildGraph(cgRoot);

        // 2. Calculate hot path
        this.calculateHotPath();

        // 3. Compute layout
        this.computeLayout();
    }

    traverseAndBuildGraph(cgNode, parent = null) {
        // Create or update node
        let node = this.nodes.get(cgNode.getFunctionName());
        if (!node) {
            node = new CallGraphNode(cgNode, this.calculateStats(cgNode));
            this.nodes.set(cgNode.getFunctionName(), node);
        }

        // Create edge from parent
        if (parent) {
            const edgeKey = `${parent.getFunctionName()}->${cgNode.getFunctionName()}`;
            if (!this.edges.has(edgeKey)) {
                const edge = new CallGraphEdge(
                    parent,
                    node,
                    cgNode.getCalled(),
                    cgNode.getInc()
                );
                this.edges.set(edgeKey, edge);
                parent.children.push(node);
                node.parents.push(parent);
            }
        } else {
            this.rootNodes.push(node);
        }

        // Recurse
        for (const child of cgNode.getChildren()) {
            this.traverseAndBuildGraph(child, node);
        }
    }

    calculateStats(cgNode) {
        return {
            inclusive: cgNode.getInc().getValue(this.currentMetric),
            exclusive: cgNode.getExc().getValue(this.currentMetric),
            calls: cgNode.getCalled(),
        };
    }

    calculateHotPath() {
        // Find critical path (longest weighted path from root to leaf)
        // This is the "hot path" - the most expensive execution path

        const visited = new Set();
        const pathScores = new Map();

        // Dynamic programming: for each node, calculate max path score
        const computeMaxPath = (node) => {
            if (visited.has(node)) return pathScores.get(node);

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

            // Mark hot path
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

        while (current) {
            current.isOnHotPath = true;
            current.hotPathScore = current.getInclusiveMetric(this.currentMetric) / maxRootPath;
            current.criticalPathContribution = current.getInclusiveMetric(this.currentMetric);

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

    computeLayout() {
        // Hierarchical layout using depth-based positioning
        // This is a simplified version; real implementation would use
        // Sugiyama framework or force-directed layout

        const levels = new Map(); // depth → [nodes]
        const nodeWidth = 150;
        const nodeHeight = 40;
        const horizontalSpacing = 30;
        const verticalSpacing = 80;

        // Group nodes by depth
        for (const node of this.nodes.values()) {
            const depth = node.getDepth();
            if (!levels.has(depth)) {
                levels.set(depth, []);
            }
            levels.get(depth).push(node);
        }

        // Assign coordinates
        let y = 20;
        for (const [depth, nodesAtLevel] of [...levels.entries()].sort((a, b) => a[0] - b[0])) {
            const levelWidth = nodesAtLevel.length * (nodeWidth + horizontalSpacing);
            let x = (this.viewportWidth - levelWidth) / 2;

            for (const node of nodesAtLevel) {
                node.layout.x = x;
                node.layout.y = y;
                node.layout.width = nodeWidth;
                node.layout.height = nodeHeight;

                x += nodeWidth + horizontalSpacing;
            }

            y += nodeHeight + verticalSpacing;
        }
    }

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
                return nodes.sort((a, b) => b.hotPathScore - a.hotPathScore);
            default:
                return nodes;
        }
    }

    getTopFunctions(n, sortBy = 'inc') {
        return this.getSortedNodes(sortBy).slice(0, n);
    }

    filterByThreshold(minPercentage) {
        // Prune nodes below threshold (Blackfire-style)
        const maxValue = this.criticalPathLength;
        const threshold = maxValue * (minPercentage / 100);

        const filtered = new CallGraphData(this.profileData, this.timeRange, this.currentMetric);

        for (const [name, node] of this.nodes.entries()) {
            if (node.getInclusiveMetric(this.currentMetric) >= threshold) {
                filtered.nodes.set(name, node);
            }
        }

        // Update edges
        for (const [key, edge] of this.edges.entries()) {
            if (filtered.nodes.has(edge.from.getFunctionName()) &&
                filtered.nodes.has(edge.to.getFunctionName())) {
                filtered.edges.set(key, edge);
            }
        }

        return filtered;
    }
}
```

### New Widget: CallGraphView

```javascript
export class CallGraphView extends SVGWidget {
    constructor(container, profileData) {
        super(container, profileData);

        this.graphData = null;
        this.viewportTransform = { x: 0, y: 0, scale: 1.0 };
        this.minZoom = 0.1;
        this.maxZoom = 5.0;

        this.svgNodePool = new svg.NodePool('rect');
        this.svgTextPool = new svg.NodePool('text');
        this.svgLinePool = new svg.NodePool('line');
        this.svgPathPool = new svg.NodePool('path');

        this.selectedNode = null;
        this.hoveredNode = null;
        this.filterThreshold = 1.0; // Prune nodes <1%

        this.initializeInteractions();
    }

    initializeInteractions() {
        // Zoom with mouse wheel
        this.viewPort.node.addEventListener('wheel', e => {
            e.preventDefault();
            const delta = e.deltaY > 0 ? 0.9 : 1.1;
            this.zoom(delta, e.clientX, e.clientY);
        });

        // Pan with drag
        let isDragging = false;
        let dragStart = { x: 0, y: 0 };

        this.viewPort.node.addEventListener('mousedown', e => {
            if (e.button === 0 && !e.target.dataset.nodeId) {
                isDragging = true;
                dragStart = { x: e.clientX, y: e.clientY };
                e.preventDefault();
            }
        });

        window.addEventListener('mousemove', e => {
            if (isDragging) {
                const dx = e.clientX - dragStart.x;
                const dy = e.clientY - dragStart.y;
                this.pan(dx, dy);
                dragStart = { x: e.clientX, y: e.clientY };
            }
        });

        window.addEventListener('mouseup', () => {
            isDragging = false;
        });

        // Node hover
        this.viewPort.node.addEventListener('mousemove', e => {
            const element = document.elementFromPoint(e.clientX, e.clientY);
            const nodeId = element?.dataset?.nodeId;

            if (nodeId) {
                this.setHoveredNode(nodeId);
            } else {
                this.setHoveredNode(null);
            }
        });

        // Node click
        this.viewPort.node.addEventListener('click', e => {
            const element = document.elementFromPoint(e.clientX, e.clientY);
            const nodeId = element?.dataset?.nodeId;

            if (nodeId) {
                this.setSelectedNode(nodeId);
            }
        });
    }

    zoom(factor, centerX, centerY) {
        const newScale = math.bound(
            this.viewportTransform.scale * factor,
            this.minZoom,
            this.maxZoom
        );

        // Zoom toward cursor position
        const rect = this.viewPort.node.getBoundingClientRect();
        const x = centerX - rect.left;
        const y = centerY - rect.top;

        this.viewportTransform.x = x - (x - this.viewportTransform.x) * (newScale / this.viewportTransform.scale);
        this.viewportTransform.y = y - (y - this.viewportTransform.y) * (newScale / this.viewportTransform.scale);
        this.viewportTransform.scale = newScale;

        this.repaint();
    }

    pan(dx, dy) {
        this.viewportTransform.x += dx;
        this.viewportTransform.y += dy;
        this.repaint();
    }

    render() {
        this.graphData = new CallGraphData(
            this.profileData,
            this.timeRange,
            this.currentMetric
        );

        if (this.filterThreshold > 0) {
            this.graphData = this.graphData.filterByThreshold(this.filterThreshold);
        }

        this.renderGraph();
    }

    renderGraph() {
        this.viewPort.clear();
        this.svgNodePool.releaseAll();
        this.svgTextPool.releaseAll();
        this.svgLinePool.releaseAll();
        this.svgPathPool.releaseAll();

        // Apply viewport transform
        const g = svg.createNode('g', {
            transform: `translate(${this.viewportTransform.x}, ${this.viewportTransform.y}) scale(${this.viewportTransform.scale})`
        });

        // Render edges first (so they appear behind nodes)
        for (const edge of this.graphData.edges.values()) {
            this.renderEdge(g, edge);
        }

        // Render nodes
        for (const node of this.graphData.nodes.values()) {
            this.renderNode(g, node);
        }

        this.viewPort.appendChild(g);
    }

    renderEdge(container, edge) {
        const fromNode = edge.from;
        const toNode = edge.to;

        const x1 = fromNode.layout.x + fromNode.layout.width / 2;
        const y1 = fromNode.layout.y + fromNode.layout.height;
        const x2 = toNode.layout.x + toNode.layout.width / 2;
        const y2 = toNode.layout.y;

        // Bezier curve for smoother edges
        const midY = (y1 + y2) / 2;
        const path = `M ${x1} ${y1} C ${x1} ${midY}, ${x2} ${midY}, ${x2} ${y2}`;

        let strokeColor = '#444';
        let strokeWidth = 1;

        if (edge.isOnHotPath) {
            // Hot path edges are thicker and red-gradient
            strokeWidth = 3;
            strokeColor = this.getHotPathColor(edge.hotPathScore);
        }

        const pathElement = this.svgPathPool.acquire({
            d: path,
            stroke: strokeColor,
            'stroke-width': strokeWidth,
            fill: 'none',
            'marker-end': 'url(#arrowhead)',
        });

        container.appendChild(pathElement);
    }

    renderNode(container, node) {
        const rect = this.svgNodePool.acquire({
            x: node.layout.x,
            y: node.layout.y,
            width: node.layout.width,
            height: node.layout.height,
            fill: this.getNodeColor(node),
            stroke: this.getNodeStroke(node),
            'stroke-width': node === this.selectedNode ? 3 : (node === this.hoveredNode ? 2 : 1),
            rx: 5,
            ry: 5,
            'data-node-id': node.getFunctionName(),
        });

        const text = this.svgTextPool.acquire({
            x: node.layout.x + node.layout.width / 2,
            y: node.layout.y + node.layout.height / 2,
            'text-anchor': 'middle',
            'dominant-baseline': 'middle',
            'font-size': 12,
            fill: '#fff',
            'pointer-events': 'none',
        });

        text.textContent = utils.truncateFunctionName(node.getFunctionName(), node.layout.width / 7);

        container.appendChild(rect);
        container.appendChild(text);
    }

    getNodeColor(node) {
        if (node.isOnHotPath) {
            return this.getHotPathColor(node.hotPathScore);
        }

        // Use existing color resolver from parent class
        return this.functionColorResolver(
            node.getFunctionName(),
            new math.Vec3(0.6, 0.5, 0.5).toHTMLColor()
        );
    }

    getHotPathColor(score) {
        // Blackfire-inspired red gradient
        // score: 0.0 (cool) → 1.0 (hot)
        const hue = 0;     // Red
        const saturation = 0.7 + score * 0.3;  // 0.7 → 1.0
        const value = 0.4 + score * 0.6;        // 0.4 → 1.0

        return new math.Vec3(hue, saturation, value).toHTMLColor();
    }

    getNodeStroke(node) {
        if (node === this.selectedNode) return '#0ff';
        if (node === this.hoveredNode) return '#fff';
        if (node.isOnHotPath) return '#ff0';
        return '#000';
    }

    setHoveredNode(nodeId) {
        const node = nodeId ? this.graphData.nodes.get(nodeId) : null;
        if (this.hoveredNode !== node) {
            this.hoveredNode = node;
            this.updateInfoPanel();
            this.repaint();
        }
    }

    setSelectedNode(nodeId) {
        const node = this.graphData.nodes.get(nodeId);
        this.selectedNode = node;
        this.updateInfoPanel();
        this.repaint();

        // Trigger global event for cross-widget synchronization
        $(window).trigger('spx-highlighted-function-update', [nodeId]);
    }

    updateInfoPanel() {
        const node = this.hoveredNode || this.selectedNode;
        if (!node) return;

        // Render info in separate viewport (similar to existing flamegraph)
        const info = [
            `Function: ${node.getFunctionName()}`,
            `Depth: ${node.getDepth()}`,
            `Calls: ${node.getCalled()}`,
            `Inclusive: ${this.profileData.getMetricFormatter(this.currentMetric)(node.getInclusiveMetric(this.currentMetric))}`,
            `Exclusive: ${this.profileData.getMetricFormatter(this.currentMetric)(node.getExclusiveMetric(this.currentMetric))}`,
        ];

        if (node.isOnHotPath) {
            info.push(`🔥 HOT PATH (${(node.hotPathScore * 100).toFixed(1)}%)`);
        }

        // Update info viewport (implementation similar to existing FlameGraph)
    }

    onTimeRangeUpdate() {
        this.repaint();
    }
}
```

## 1.3 File Structure

```
assets/web-ui/js/
├── callGraph.js (NEW)         → CallGraphData, CallGraphNode, CallGraphEdge
├── widget.js (MODIFIED)       → Add CallGraphView class
├── profileData.js (ENHANCED)  → Add edge extraction methods
├── svg.js (ENHANCED)          → Add path/line pools, arrowhead markers
├── layouts.js (NEW)           → Graph layout algorithms (hierarchical, force-directed)
└── (existing files unchanged)
```

## 1.4 Backend Requirements

**No backend changes required** - all data is already available via existing APIs:
- Call tree structure: `/data/reports/get/{key}` provides full event stream
- Metrics: `/data/metrics` lists all available metrics
- Metadata: `/data/reports/metadata/{key}` for context

**Data Processing:** All graph construction and hot path calculation happens client-side in JavaScript.

---

# DESIGN ITERATION 2: Interactive Features & Hot Path Visualization

## 2.1 Objectives
- Implement advanced interaction patterns from D3.js flamegraphs
- Create Blackfire-inspired hot path visualization with red gradient
- Add multi-metric support with dynamic recalculation
- Implement sorting and filtering capabilities

## 2.2 Hot Path Detection Algorithm

### Critical Path Algorithm (Enhanced)
```javascript
class HotPathAnalyzer {
    constructor(graphData, metric) {
        this.graphData = graphData;
        this.metric = metric;
        this.criticalPaths = [];  // Multiple hot paths (top N)
    }

    analyzePaths(topN = 5) {
        // Find top N critical paths (not just the single hottest)
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
                node.hotPathRank = i + 1;
            }
        }
    }

    findAllPaths() {
        const paths = [];
        const visited = new Set();

        const dfs = (node, currentPath, currentMetric) => {
            if (visited.has(node)) return;

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
                visited.add(node);
                for (const child of node.children) {
                    dfs(child, newPath, newMetric);
                }
                visited.delete(node);
            }
        };

        for (const root of this.graphData.rootNodes) {
            dfs(root, [], 0);
        }

        return paths;
    }

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
```

## 2.3 Color Schemes

### Hot Path Color Scheme (Blackfire-inspired)
```javascript
class ColorSchemes {
    static HOTPATH_RED(score) {
        // score: 0.0 → 1.0
        // Cool: Dark red (#8B0000)
        // Hot:  Bright red (#FF0000)
        const hue = 0;  // Red
        const saturation = 0.8 + score * 0.2;
        const value = 0.3 + score * 0.7;
        return new math.Vec3(hue, saturation, value).toHTMLColor();
    }

    static GRADIENT_BLUE_RED(score) {
        // Blue (cool) → Green → Yellow → Orange → Red (hot)
        const hue = (1 - score) * 240 / 360;  // 240° (blue) → 0° (red)
        return new math.Vec3(hue, 0.9, 0.9).toHTMLColor();
    }

    static FLAME_CLASSIC(score, depth) {
        // Classic flamegraph coloring with random jitter
        const baseHue = 0.04;  // Orange/red base
        const jitter = (Math.random() - 0.5) * 0.1;
        const hue = baseHue + jitter;
        const saturation = 0.5 + score * 0.4;
        const value = 0.5 + depth * 0.02;  // Slight depth-based darkening
        return new math.Vec3(hue, saturation, value).toHTMLColor();
    }

    static CATEGORY_BASED(functionName) {
        // Existing category-based coloring
        for (const category of utils.functionCategories) {
            if (category.regex.test(functionName)) {
                return category.color;
            }
        }
        return '#666';  // Default gray
    }
}
```

## 2.4 Sorting & Filtering UI

### Enhanced FlatProfile with Sorting
```javascript
export class FlatProfile extends Widget {
    constructor(container, profileData) {
        super(container, profileData);
        this.sortBy = 'inc';  // Default sort
        this.sortOrder = 'desc';
        this.filterQuery = '';
        this.minCallThreshold = 0;
    }

    render() {
        const graphData = new CallGraphData(
            this.profileData,
            this.timeRange,
            this.currentMetric
        );

        let nodes = graphData.getSortedNodes(this.sortBy);

        if (this.sortOrder === 'asc') {
            nodes.reverse();
        }

        // Apply filters
        if (this.filterQuery) {
            const query = this.filterQuery.toLowerCase();
            nodes = nodes.filter(n =>
                n.getFunctionName().toLowerCase().includes(query)
            );
        }

        if (this.minCallThreshold > 0) {
            nodes = nodes.filter(n => n.getCalled() >= this.minCallThreshold);
        }

        const table = makeDataTable({
            columns: [
                {
                    name: 'Function',
                    accessor: n => n.getFunctionName(),
                    sortable: true,
                    sortKey: 'name'
                },
                {
                    name: 'Calls',
                    accessor: n => n.getCalled(),
                    formatter: fmt.quantity,
                    sortable: true,
                    sortKey: 'calls',
                    align: 'right'
                },
                {
                    name: 'Inc. Time',
                    accessor: n => n.getInclusiveMetric(this.currentMetric),
                    formatter: this.profileData.getMetricFormatter(this.currentMetric),
                    sortable: true,
                    sortKey: 'inc',
                    align: 'right'
                },
                {
                    name: 'Inc. %',
                    accessor: n => (n.getInclusiveMetric(this.currentMetric) / graphData.criticalPathLength) * 100,
                    formatter: v => fmt.pct(v / 100),
                    sortable: true,
                    sortKey: 'inc_pct',
                    align: 'right'
                },
                {
                    name: 'Exc. Time',
                    accessor: n => n.getExclusiveMetric(this.currentMetric),
                    formatter: this.profileData.getMetricFormatter(this.currentMetric),
                    sortable: true,
                    sortKey: 'exc',
                    align: 'right'
                },
                {
                    name: 'Hot Path',
                    accessor: n => n.isOnHotPath ? `🔥 #${n.hotPathRank}` : '',
                    sortable: true,
                    sortKey: 'hotpath',
                    align: 'center'
                },
            ],
            rows: nodes,
            onRowClick: (node) => {
                $(window).trigger('spx-highlighted-function-update', [node.getFunctionName()]);
            },
            onHeaderClick: (sortKey) => {
                if (this.sortBy === sortKey) {
                    this.sortOrder = this.sortOrder === 'asc' ? 'desc' : 'asc';
                } else {
                    this.sortBy = sortKey;
                    this.sortOrder = 'desc';
                }
                this.repaint();
            }
        });

        this.container.empty().append(table);
    }
}
```

### Filter Controls
```html
<!-- Add to report.html -->
<div id="filter-controls" class="widget" style="flex: 0 0 auto;">
    <label>
        Min Calls:
        <input type="number" id="filter-min-calls" value="0" min="0" step="1">
    </label>
    <label>
        Min %:
        <input type="number" id="filter-min-percent" value="1.0" min="0" max="100" step="0.1">
    </label>
    <label>
        View Mode:
        <select id="view-mode-selector">
            <option value="callgraph">Call Graph</option>
            <option value="flamegraph" selected>Flame Graph</option>
            <option value="timeline">Timeline</option>
        </select>
    </label>
    <label>
        Color Scheme:
        <select id="color-scheme-selector">
            <option value="hotpath">Hot Path (Red)</option>
            <option value="gradient">Gradient (Blue-Red)</option>
            <option value="flame">Flame (Classic)</option>
            <option value="category">Category</option>
        </select>
    </label>
</div>
```

## 2.5 Zoom & Navigation

### Breadcrumb Navigation (D3.js-inspired)
```javascript
class BreadcrumbNavigator {
    constructor(container) {
        this.container = container;
        this.stack = [];  // [{node, label}, ...]
    }

    push(node, label) {
        this.stack.push({ node, label });
        this.render();
    }

    pop() {
        if (this.stack.length > 1) {
            this.stack.pop();
            this.render();
            return this.stack[this.stack.length - 1].node;
        }
        return null;
    }

    navigateTo(index) {
        this.stack = this.stack.slice(0, index + 1);
        this.render();
        return this.stack[this.stack.length - 1].node;
    }

    render() {
        this.container.empty();

        for (let i = 0; i < this.stack.length; i++) {
            const item = this.stack[i];

            const crumb = $('<span>')
                .addClass('breadcrumb-item')
                .text(item.label)
                .on('click', () => {
                    const node = this.navigateTo(i);
                    $(window).trigger('spx-breadcrumb-navigate', [node]);
                });

            if (i < this.stack.length - 1) {
                crumb.addClass('clickable');
            } else {
                crumb.addClass('active');
            }

            this.container.append(crumb);

            if (i < this.stack.length - 1) {
                this.container.append($('<span>').addClass('separator').text(' → '));
            }
        }
    }

    clear() {
        this.stack = [];
        this.render();
    }
}
```

### Zoom on Click (Subgraph Focus)
```javascript
// In CallGraphView class
zoomToNode(node) {
    // Build subgraph rooted at node
    const subgraph = this.buildSubgraph(node);

    // Recompute layout for subgraph
    subgraph.computeLayout();

    // Update breadcrumb
    this.breadcrumbNavigator.push(node, node.getFunctionName());

    // Render subgraph
    this.graphData = subgraph;
    this.renderGraph();
}

buildSubgraph(rootNode) {
    const subgraph = new CallGraphData(this.profileData, this.timeRange, this.currentMetric);

    const visited = new Set();
    const queue = [rootNode];

    while (queue.length > 0) {
        const node = queue.shift();
        if (visited.has(node)) continue;

        visited.add(node);
        subgraph.nodes.set(node.getFunctionName(), node);

        for (const child of node.children) {
            queue.push(child);

            // Add edge
            const edgeKey = `${node.getFunctionName()}->${child.getFunctionName()}`;
            const edge = this.graphData.edges.get(edgeKey);
            if (edge) {
                subgraph.edges.set(edgeKey, edge);
            }
        }
    }

    subgraph.rootNodes = [rootNode];
    subgraph.calculateHotPath();

    return subgraph;
}
```

## 2.6 Cross-Widget Synchronization

### Event Bus
```javascript
// Global event handling
$(window).on('spx-highlighted-function-update', (event, functionName) => {
    // Update all widgets to highlight the selected function
    for (const widget of activeWidgets) {
        widget.highlightFunction(functionName);
    }
});

$(window).on('spx-metric-update', (event, metricKey) => {
    // Recalculate hot paths with new metric
    for (const widget of activeWidgets) {
        widget.setCurrentMetric(metricKey);
        widget.repaint();
    }
});

$(window).on('spx-filter-update', (event, filters) => {
    // Apply filters to all visualizations
    for (const widget of activeWidgets) {
        widget.applyFilters(filters);
        widget.repaint();
    }
});

$(window).on('spx-breadcrumb-navigate', (event, node) => {
    // Zoom to node in call graph
    callGraphWidget.zoomToNode(node);
});
```

---

# DESIGN ITERATION 3: Performance Optimization & Advanced Features

## 3.1 Objectives
- Implement viewport culling for large graphs (cimi/d3-flame-graphs technique)
- Add progressive rendering for graphs with 1000+ nodes
- Create export functionality (SVG, PNG, JSON)
- Implement advanced layout algorithms
- Add diff mode for comparing two profiles

## 3.2 Performance Optimizations

### Viewport Culling (10-20x Speedup)
```javascript
class ViewportCuller {
    constructor(viewport) {
        this.viewport = viewport;
        this.visibleNodes = new Set();
        this.visibleEdges = new Set();
    }

    cullGraph(graphData, transform) {
        this.visibleNodes.clear();
        this.visibleEdges.clear();

        const viewBox = this.getViewBox(transform);

        // Check which nodes are visible
        for (const node of graphData.nodes.values()) {
            if (this.isRectVisible(node.layout, viewBox)) {
                this.visibleNodes.add(node);
            }
        }

        // Check which edges connect visible nodes
        for (const edge of graphData.edges.values()) {
            if (this.visibleNodes.has(edge.from) && this.visibleNodes.has(edge.to)) {
                this.visibleEdges.add(edge);
            }
        }

        return {
            nodes: Array.from(this.visibleNodes),
            edges: Array.from(this.visibleEdges)
        };
    }

    getViewBox(transform) {
        return {
            x: -transform.x / transform.scale,
            y: -transform.y / transform.scale,
            width: this.viewport.width / transform.scale,
            height: this.viewport.height / transform.scale
        };
    }

    isRectVisible(rect, viewBox) {
        return !(
            rect.x + rect.width < viewBox.x ||
            rect.x > viewBox.x + viewBox.width ||
            rect.y + rect.height < viewBox.y ||
            rect.y > viewBox.y + viewBox.height
        );
    }
}

// In CallGraphView.renderGraph():
renderGraph() {
    this.viewPort.clear();

    // Cull invisible nodes/edges
    const culler = new ViewportCuller(this.viewPort);
    const visible = culler.cullGraph(this.graphData, this.viewportTransform);

    console.log(`Rendering ${visible.nodes.length}/${this.graphData.nodes.size} nodes (${((visible.nodes.length / this.graphData.nodes.size) * 100).toFixed(1)}% visible)`);

    const g = svg.createNode('g', {
        transform: `translate(${this.viewportTransform.x}, ${this.viewportTransform.y}) scale(${this.viewportTransform.scale})`
    });

    // Render only visible elements
    for (const edge of visible.edges) {
        this.renderEdge(g, edge);
    }

    for (const node of visible.nodes) {
        this.renderNode(g, node);
    }

    this.viewPort.appendChild(g);
}
```

### Progressive Rendering
```javascript
class ProgressiveRenderer {
    constructor(widget) {
        this.widget = widget;
        this.renderBatchSize = 50;  // Render 50 nodes per frame
        this.currentBatch = 0;
        this.totalBatches = 0;
        this.rendering = false;
    }

    renderProgressive(nodes, edges) {
        this.nodes = nodes;
        this.edges = edges;
        this.totalBatches = Math.ceil(nodes.length / this.renderBatchSize);
        this.currentBatch = 0;
        this.rendering = true;

        this.renderNextBatch();
    }

    renderNextBatch() {
        if (!this.rendering || this.currentBatch >= this.totalBatches) {
            this.rendering = false;
            return;
        }

        const start = this.currentBatch * this.renderBatchSize;
        const end = Math.min(start + this.renderBatchSize, this.nodes.length);

        for (let i = start; i < end; i++) {
            const node = this.nodes[i];
            this.widget.renderNode(this.widget.viewPort.node, node);
        }

        this.currentBatch++;

        // Update progress
        const progress = (this.currentBatch / this.totalBatches) * 100;
        this.widget.updateRenderProgress(progress);

        // Schedule next batch
        requestAnimationFrame(() => this.renderNextBatch());
    }

    cancel() {
        this.rendering = false;
    }
}
```

### Level-of-Detail (LOD) Rendering
```javascript
class LODRenderer {
    constructor() {
        this.lodLevels = [
            { maxScale: 0.3, renderDetail: 'minimal' },   // Tiny boxes only
            { maxScale: 0.7, renderDetail: 'low' },       // Boxes + shortened names
            { maxScale: 1.5, renderDetail: 'medium' },    // Full names, no details
            { maxScale: Infinity, renderDetail: 'high' }, // Full details
        ];
    }

    getDetailLevel(scale) {
        for (const level of this.lodLevels) {
            if (scale <= level.maxScale) {
                return level.renderDetail;
            }
        }
        return 'high';
    }

    renderNode(node, scale, container) {
        const detail = this.getDetailLevel(scale);

        switch (detail) {
            case 'minimal':
                // Just a colored rectangle
                return this.renderMinimalNode(node, container);

            case 'low':
                // Rectangle + abbreviated name
                return this.renderLowDetailNode(node, container);

            case 'medium':
                // Rectangle + full name
                return this.renderMediumDetailNode(node, container);

            case 'high':
                // Rectangle + name + metrics
                return this.renderHighDetailNode(node, container);
        }
    }

    renderMinimalNode(node, container) {
        const rect = svg.createNode('rect', {
            x: node.layout.x,
            y: node.layout.y,
            width: node.layout.width,
            height: node.layout.height,
            fill: node.color,
            'data-node-id': node.getFunctionName(),
        });
        container.appendChild(rect);
    }

    // ... other detail level implementations
}
```

## 3.3 Advanced Layout Algorithms

### Force-Directed Layout (for complex call graphs)
```javascript
class ForceDirectedLayout {
    constructor(graphData) {
        this.graphData = graphData;
        this.iterations = 100;
        this.repulsionStrength = 1000;
        this.attractionStrength = 0.01;
        this.damping = 0.85;
    }

    compute() {
        // Initialize random positions
        for (const node of this.graphData.nodes.values()) {
            node.layout.x = Math.random() * 1000;
            node.layout.y = Math.random() * 1000;
            node.velocity = { x: 0, y: 0 };
        }

        // Iterate
        for (let i = 0; i < this.iterations; i++) {
            this.computeForces();
            this.updatePositions();
        }
    }

    computeForces() {
        const nodes = Array.from(this.graphData.nodes.values());

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

    updatePositions() {
        for (const node of this.graphData.nodes.values()) {
            node.layout.x += node.velocity.x;
            node.layout.y += node.velocity.y;

            node.velocity.x *= this.damping;
            node.velocity.y *= this.damping;
        }
    }
}
```

### Hierarchical Layout (Sugiyama Framework)
```javascript
class SugiyamaLayout {
    constructor(graphData) {
        this.graphData = graphData;
        this.layers = [];
        this.nodeWidth = 150;
        this.nodeHeight = 40;
        this.layerSpacing = 80;
        this.nodeSpacing = 30;
    }

    compute() {
        // 1. Layer assignment (topological sort)
        this.assignLayers();

        // 2. Minimize edge crossings
        this.minimizeCrossings();

        // 3. Assign x-coordinates
        this.assignCoordinates();
    }

    assignLayers() {
        const visited = new Set();
        const layers = new Map();  // node → layer

        const assignLayer = (node, layer) => {
            if (visited.has(node)) {
                // Update layer to max of current and new
                layers.set(node, Math.max(layers.get(node), layer));
                return;
            }

            visited.add(node);
            layers.set(node, layer);

            for (const child of node.children) {
                assignLayer(child, layer + 1);
            }
        };

        for (const root of this.graphData.rootNodes) {
            assignLayer(root, 0);
        }

        // Group nodes by layer
        this.layers = [];
        const maxLayer = Math.max(...layers.values());

        for (let i = 0; i <= maxLayer; i++) {
            this.layers[i] = [];
        }

        for (const [node, layer] of layers.entries()) {
            this.layers[layer].push(node);
        }
    }

    minimizeCrossings() {
        // Barycenter heuristic for crossing minimization
        for (let iteration = 0; iteration < 10; iteration++) {
            for (let i = 1; i < this.layers.length; i++) {
                this.sortLayerByBarycenter(this.layers[i], this.layers[i - 1]);
            }
        }
    }

    sortLayerByBarycenter(layer, previousLayer) {
        // Calculate barycenter for each node based on parent positions
        const barycenters = new Map();

        for (const node of layer) {
            const parents = node.parents.filter(p => previousLayer.includes(p));
            if (parents.length === 0) {
                barycenters.set(node, 0);
            } else {
                const avgPos = parents.reduce((sum, p) => sum + previousLayer.indexOf(p), 0) / parents.length;
                barycenters.set(node, avgPos);
            }
        }

        layer.sort((a, b) => barycenters.get(a) - barycenters.get(b));
    }

    assignCoordinates() {
        let y = this.nodeSpacing;

        for (const layer of this.layers) {
            const layerWidth = layer.length * (this.nodeWidth + this.nodeSpacing);
            let x = (this.graphData.viewportWidth - layerWidth) / 2;

            for (const node of layer) {
                node.layout.x = x;
                node.layout.y = y;
                node.layout.width = this.nodeWidth;
                node.layout.height = this.nodeHeight;

                x += this.nodeWidth + this.nodeSpacing;
            }

            y += this.nodeHeight + this.layerSpacing;
        }
    }
}
```

## 3.4 Export Functionality

### SVG Export
```javascript
class SVGExporter {
    static exportToSVG(widget) {
        const svgElement = widget.viewPort.node.cloneNode(true);

        // Add CSS styles inline
        const styleElement = document.createElementNS('http://www.w3.org/2000/svg', 'style');
        styleElement.textContent = this.getInlineStyles();
        svgElement.insertBefore(styleElement, svgElement.firstChild);

        // Set viewBox to capture entire graph
        const bbox = svgElement.getBBox();
        svgElement.setAttribute('viewBox', `${bbox.x} ${bbox.y} ${bbox.width} ${bbox.height}`);
        svgElement.setAttribute('xmlns', 'http://www.w3.org/2000/svg');

        // Serialize to string
        const serializer = new XMLSerializer();
        const svgString = serializer.serializeToString(svgElement);

        return svgString;
    }

    static downloadSVG(widget, filename) {
        const svgString = this.exportToSVG(widget);
        const blob = new Blob([svgString], { type: 'image/svg+xml' });
        const url = URL.createObjectURL(blob);

        const a = document.createElement('a');
        a.href = url;
        a.download = filename;
        a.click();

        URL.revokeObjectURL(url);
    }

    static getInlineStyles() {
        return `
            rect { stroke: #000; stroke-width: 1; }
            text { font-family: monospace; font-size: 12px; }
            path { stroke: #444; fill: none; }
        `;
    }
}
```

### PNG Export (via Canvas)
```javascript
class PNGExporter {
    static async exportToPNG(widget, scale = 2) {
        const svgString = SVGExporter.exportToSVG(widget);

        // Create canvas
        const canvas = document.createElement('canvas');
        const ctx = canvas.getContext('2d');

        // Load SVG into image
        const img = new Image();
        const svgBlob = new Blob([svgString], { type: 'image/svg+xml' });
        const url = URL.createObjectURL(svgBlob);

        return new Promise((resolve) => {
            img.onload = () => {
                canvas.width = img.width * scale;
                canvas.height = img.height * scale;

                ctx.scale(scale, scale);
                ctx.drawImage(img, 0, 0);

                URL.revokeObjectURL(url);

                canvas.toBlob((blob) => {
                    resolve(blob);
                }, 'image/png');
            };

            img.src = url;
        });
    }

    static async downloadPNG(widget, filename, scale = 2) {
        const blob = await this.exportToPNG(widget, scale);
        const url = URL.createObjectURL(blob);

        const a = document.createElement('a');
        a.href = url;
        a.download = filename;
        a.click();

        URL.revokeObjectURL(url);
    }
}
```

### JSON Export (Speedscope Format)
```javascript
class SpeedscopeExporter {
    static exportToSpeedscope(profileData) {
        const frames = [];
        const frameIndex = new Map();  // functionName → index

        // Build frame list
        for (const [name, node] of profileData.nodes.entries()) {
            frameIndex.set(name, frames.length);
            frames.push({
                name: name,
                file: '',  // Not available in SPX data
                line: 0,
                col: 0
            });
        }

        // Build sample stacks
        const samples = [];
        const weights = [];

        // Note: This requires temporal data from events, which would need
        // to be extracted from the profile data builder
        // Simplified version here

        return {
            version: '0.0.1',
            shared: { frames },
            profiles: [{
                type: 'sampled',
                name: 'SPX Profile',
                unit: 'microseconds',
                startValue: 0,
                endValue: profileData.getMetadata().wall_time_ms * 1000,
                samples: samples,
                weights: weights
            }]
        };
    }

    static downloadJSON(profileData, filename) {
        const data = this.exportToSpeedscope(profileData);
        const json = JSON.stringify(data, null, 2);
        const blob = new Blob([json], { type: 'application/json' });
        const url = URL.createObjectURL(blob);

        const a = document.createElement('a');
        a.href = url;
        a.download = filename;
        a.click();

        URL.revokeObjectURL(url);
    }
}
```

## 3.5 Diff Mode (Compare Two Profiles)

### ProfileDiffer
```javascript
class ProfileDiffer {
    constructor(baselineProfile, comparisonProfile, metric) {
        this.baseline = baselineProfile;
        this.comparison = comparisonProfile;
        this.metric = metric;
        this.diff = this.computeDiff();
    }

    computeDiff() {
        const diff = new Map();  // functionName → DiffNode

        const baselineNodes = new Map();
        for (const [name, node] of this.baseline.nodes.entries()) {
            baselineNodes.set(name, node.getInclusiveMetric(this.metric));
        }

        const comparisonNodes = new Map();
        for (const [name, node] of this.comparison.nodes.entries()) {
            comparisonNodes.set(name, node.getInclusiveMetric(this.metric));
        }

        // Find all unique function names
        const allNames = new Set([...baselineNodes.keys(), ...comparisonNodes.keys()]);

        for (const name of allNames) {
            const baseValue = baselineNodes.get(name) || 0;
            const compValue = comparisonNodes.get(name) || 0;
            const delta = compValue - baseValue;
            const percentChange = baseValue > 0 ? (delta / baseValue) * 100 : (compValue > 0 ? Infinity : 0);

            diff.set(name, {
                functionName: name,
                baseline: baseValue,
                comparison: compValue,
                delta: delta,
                percentChange: percentChange,
                isNew: baseValue === 0,
                isRemoved: compValue === 0,
                isRegression: delta > 0,
                isImprovement: delta < 0
            });
        }

        return diff;
    }

    getTopRegressions(n = 10) {
        return Array.from(this.diff.values())
            .filter(d => d.isRegression)
            .sort((a, b) => b.delta - a.delta)
            .slice(0, n);
    }

    getTopImprovements(n = 10) {
        return Array.from(this.diff.values())
            .filter(d => d.isImprovement)
            .sort((a, b) => a.delta - b.delta)
            .slice(0, n);
    }

    getNewFunctions() {
        return Array.from(this.diff.values()).filter(d => d.isNew);
    }

    getRemovedFunctions() {
        return Array.from(this.diff.values()).filter(d => d.isRemoved);
    }
}
```

### DiffVisualization
```javascript
export class DiffCallGraphView extends CallGraphView {
    constructor(container, baselineProfile, comparisonProfile) {
        super(container, comparisonProfile);

        this.differ = new ProfileDiffer(
            new CallGraphData(baselineProfile, this.timeRange, this.currentMetric),
            new CallGraphData(comparisonProfile, this.timeRange, this.currentMetric),
            this.currentMetric
        );
    }

    getNodeColor(node) {
        const diff = this.differ.diff.get(node.getFunctionName());
        if (!diff) return '#666';

        if (diff.isNew) return '#0f0';  // Green for new
        if (diff.isRemoved) return '#f00';  // Red for removed

        // Color based on percent change
        if (diff.isRegression) {
            const intensity = Math.min(Math.abs(diff.percentChange) / 100, 1);
            return new math.Vec3(0, 0.8, 0.3 + intensity * 0.7).toHTMLColor();  // Red
        } else if (diff.isImprovement) {
            const intensity = Math.min(Math.abs(diff.percentChange) / 100, 1);
            return new math.Vec3(0.33, 0.8, 0.3 + intensity * 0.7).toHTMLColor();  // Green
        }

        return '#666';  // Gray for no significant change
    }

    renderDiffLegend() {
        // Render legend explaining color coding
        const legend = `
            <div class="diff-legend">
                <div><span style="background: #0f0"></span> New function</div>
                <div><span style="background: #f00"></span> Removed function</div>
                <div><span style="background: #ff0"></span> Regression (slower)</div>
                <div><span style="background: #0ff"></span> Improvement (faster)</div>
            </div>
        `;
        this.container.prepend(legend);
    }
}
```

## 3.6 Search & Filter Enhancements

### Regex Search
```javascript
class AdvancedSearch {
    constructor(graphData) {
        this.graphData = graphData;
    }

    searchByRegex(pattern, options = {}) {
        const regex = new RegExp(pattern, options.caseSensitive ? '' : 'i');
        const results = [];

        for (const node of this.graphData.nodes.values()) {
            if (regex.test(node.getFunctionName())) {
                results.push(node);
            }
        }

        return results;
    }

    searchByCategory(category) {
        const results = [];
        const categoryDef = utils.functionCategories.find(c => c.name === category);

        if (!categoryDef) return results;

        for (const node of this.graphData.nodes.values()) {
            if (categoryDef.regex.test(node.getFunctionName())) {
                results.push(node);
            }
        }

        return results;
    }

    searchByMetricRange(metric, min, max) {
        const results = [];

        for (const node of this.graphData.nodes.values()) {
            const value = node.getInclusiveMetric(metric);
            if (value >= min && value <= max) {
                results.push(node);
            }
        }

        return results;
    }

    searchByCallCountRange(min, max) {
        const results = [];

        for (const node of this.graphData.nodes.values()) {
            const calls = node.getCalled();
            if (calls >= min && calls <= max) {
                results.push(node);
            }
        }

        return results;
    }

    searchOnHotPath() {
        return Array.from(this.graphData.nodes.values())
            .filter(node => node.isOnHotPath);
    }
}
```

---

# IMPLEMENTATION PLAN

## Phase 1: Core Infrastructure (Week 1)
1. **Create callGraph.js module**
   - Implement `CallGraphNode`, `CallGraphEdge`, `CallGraphData` classes
   - Build graph from existing call tree data
   - Test with sample profiles

2. **Enhance svg.js**
   - Add `NodePool` for `line` and `path` elements
   - Add SVG marker definitions (arrowheads)
   - Test pooling performance

3. **Create layouts.js module**
   - Implement hierarchical layout (Sugiyama)
   - Test layout with various graph sizes

## Phase 2: Hot Path Detection (Week 2)
1. **Implement hot path algorithm**
   - Add `HotPathAnalyzer` class
   - Integrate with `CallGraphData`
   - Test on real profiles

2. **Add color schemes**
   - Implement `ColorSchemes` class
   - Add hot path red gradient
   - Add gradient blue-red scheme

## Phase 3: Call Graph Visualization (Week 3)
1. **Implement CallGraphView widget**
   - Basic node/edge rendering
   - Interactive hover/click
   - Info panel display

2. **Add zoom & pan**
   - Mouse wheel zoom
   - Drag to pan
   - Zoom to fit button

3. **Add breadcrumb navigation**
   - Implement `BreadcrumbNavigator`
   - Integrate with CallGraphView
   - Test subgraph drilling

## Phase 4: Enhanced Interactions (Week 4)
1. **Update FlatProfile**
   - Add sorting controls
   - Add hot path column
   - Add click-to-highlight

2. **Add filter controls**
   - Min calls filter
   - Min percentage filter
   - View mode selector
   - Color scheme selector

3. **Cross-widget synchronization**
   - Implement event bus
   - Sync highlighting across widgets
   - Test with all widgets

## Phase 5: Performance Optimizations (Week 5)
1. **Implement viewport culling**
   - Add `ViewportCuller` class
   - Integrate with CallGraphView
   - Benchmark performance

2. **Add progressive rendering**
   - Implement `ProgressiveRenderer`
   - Show render progress
   - Test with large profiles (1000+ nodes)

3. **Add LOD rendering**
   - Implement `LODRenderer`
   - Test at various zoom levels

## Phase 6: Advanced Features (Week 6)
1. **Add export functionality**
   - SVG export
   - PNG export
   - JSON/Speedscope export

2. **Implement diff mode**
   - Add `ProfileDiffer` class
   - Create `DiffCallGraphView` widget
   - Test comparison visualization

3. **Enhanced search**
   - Implement `AdvancedSearch` class
   - Add regex search UI
   - Add metric range filters

## Phase 7: Polish & Documentation (Week 7)
1. **UI polish**
   - Responsive design
   - Keyboard shortcuts
   - Accessibility (ARIA labels)

2. **Documentation**
   - User guide
   - API documentation
   - Performance tuning guide

3. **Testing**
   - Unit tests for core classes
   - Integration tests for widgets
   - Performance benchmarks

---

# SUCCESS METRICS

1. **Performance**
   - ✅ Render 1000+ node graphs in <2 seconds
   - ✅ 60 FPS during zoom/pan interactions
   - ✅ 10-20x speedup with viewport culling on large graphs

2. **Usability**
   - ✅ Hot path immediately visible (red coloring)
   - ✅ <3 clicks to drill down to any function
   - ✅ Sort by any metric with 1 click
   - ✅ Export to SVG/PNG with 1 click

3. **Compatibility**
   - ✅ No backend changes required
   - ✅ Existing flamegraph remains functional
   - ✅ All existing data APIs unchanged

4. **Advanced Features**
   - ✅ Diff mode for comparing profiles
   - ✅ Multiple color schemes
   - ✅ Advanced search & filtering
   - ✅ Multiple view modes (call graph, flamegraph, timeline)

---

# LESSONS FROM OTHER LANGUAGES

1. **From py-spy (Rust)**: Use separate rendering process for UI to avoid blocking profiled application
2. **From D3.js**: Declarative data binding and smooth transitions make complex visualizations intuitive
3. **From Blackfire**: Automatic pruning (<1% threshold) prevents information overload
4. **From Speedscope**: Multiple view modes cater to different analysis workflows
5. **From Brendan Gregg**: X-axis should represent metric accumulation, not chronological time
6. **From cimi/d3-flame-graphs**: Viewport culling is essential for handling 10K+ samples

---

# CONCLUSION

This design presents a comprehensive, state-of-the-art SVG-based call graph visualization system for PHP-SPX. By incorporating techniques from industry-leading profilers (Blackfire, Speedscope, py-spy) and modern D3.js implementations, we achieve:

- **Enhanced Analysis**: Hot path detection makes performance bottlenecks immediately visible
- **Flexibility**: Multiple view modes (call graph, enhanced flamegraph, timeline) support different workflows
- **Performance**: Viewport culling and progressive rendering handle profiles with 1000+ functions
- **Interactivity**: Zoom, pan, search, and filtering provide powerful exploration capabilities
- **Compatibility**: Zero backend changes; builds entirely on existing data APIs

The phased implementation plan ensures steady progress with testable milestones. Each iteration builds upon the previous, allowing for early feedback and course correction.

**Next Steps**: Begin Phase 1 implementation with core infrastructure and graph data model.
