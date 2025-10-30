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
const utils = await import(getImportUrl('/js/utils.js'));

/**
 * Color Schemes for Call Graph Visualization
 *
 * Provides multiple coloring strategies inspired by industry-leading profilers:
 * - Hot path (Blackfire-inspired red gradient)
 * - Gradient (Blue to Red spectrum)
 * - Flame (Classic flamegraph with random jitter)
 * - Category (Function type-based coloring)
 */
export class ColorSchemes {
    /**
     * Hot Path Red Gradient (Blackfire-inspired)
     *
     * Uses red color intensity to indicate hot path criticality.
     * Score ranges from 0.0 (cool, dark red) to 1.0 (hot, bright red).
     *
     * @param {number} score - Hot path score [0.0-1.0]
     * @returns {string} RGB color string
     */
    static HOTPATH_RED(score) {
        // score: 0.0 → 1.0
        // Cool: Dark red (#600000)
        // Hot:  Bright red (#FF0000)
        const hue = 0;  // Red
        const saturation = 0.8 + score * 0.2;  // 0.8 → 1.0
        const value = 0.3 + score * 0.7;        // 0.3 → 1.0

        // Convert HSV to RGB
        return this._hsvToRgb(hue, saturation, value);
    }

    /**
     * Gradient Blue-Red (Temperature Map)
     *
     * Maps score to color spectrum from cool (blue) to hot (red).
     * Blue → Cyan → Green → Yellow → Orange → Red
     *
     * @param {number} score - Metric score [0.0-1.0]
     * @returns {string} RGB color string
     */
    static GRADIENT_BLUE_RED(score) {
        // Blue (240°) → Red (0°)
        const hue = (1 - score) * 240 / 360;  // Normalize to [0, 1]
        const saturation = 0.9;
        const value = 0.9;

        return this._hsvToRgb(hue, saturation, value);
    }

    /**
     * Classic Flamegraph Coloring
     *
     * Orange/red base with random jitter for visual separation.
     * Slight depth-based darkening for hierarchical depth perception.
     *
     * @param {number} score - Metric score [0.0-1.0]
     * @param {number} depth - Node depth (optional)
     * @returns {string} RGB color string
     */
    static FLAME_CLASSIC(score, depth = 0) {
        const baseHue = 0.04;  // Orange/red base (14.4°)
        const jitter = (Math.random() - 0.5) * 0.1;
        const hue = math.bound(baseHue + jitter, 0, 1);
        const saturation = 0.5 + score * 0.4;  // 0.5 → 0.9
        const value = math.bound(0.5 + depth * 0.02, 0.5, 1.0);  // Slight darkening with depth

        return this._hsvToRgb(hue, saturation, value);
    }

    /**
     * Category-Based Coloring
     *
     * Colors functions based on their category (internal, user, vendor, etc.)
     * Uses existing function categorization from utils.
     *
     * @param {string} functionName - Function name to categorize
     * @returns {string} RGB color string
     */
    static CATEGORY_BASED(functionName) {
        // Use existing category definitions from utils
        for (const category of utils.functionCategories) {
            if (category.regex.test(functionName)) {
                return category.color;
            }
        }

        // Default gray for uncategorized
        return '#666';
    }

    /**
     * Category-Enhanced with Hot Path Overlay
     *
     * Combines category coloring with hot path highlighting.
     * Hot path nodes get a red tint overlay.
     *
     * @param {string} functionName - Function name
     * @param {boolean} isHot - Is on hot path
     * @param {number} hotScore - Hot path score [0.0-1.0]
     * @returns {string} RGB color string
     */
    static CATEGORY_WITH_HOTPATH(functionName, isHot, hotScore = 0) {
        const baseColor = this.CATEGORY_BASED(functionName);

        if (!isHot || hotScore === 0) {
            return baseColor;
        }

        // Parse base color
        const rgb = this._parseRgb(baseColor);
        if (!rgb) return baseColor;

        // Add red tint based on hot score
        const redBoost = hotScore * 0.5;  // Up to 50% red boost
        rgb.r = Math.min(255, rgb.r + redBoost * 255);

        return `rgb(${Math.round(rgb.r)},${Math.round(rgb.g)},${Math.round(rgb.b)})`;
    }

    /**
     * Depth-Based Gradient
     *
     * Colors nodes based on call depth, useful for understanding nesting.
     *
     * @param {number} depth - Node depth
     * @param {number} maxDepth - Maximum depth in graph
     * @returns {string} RGB color string
     */
    static DEPTH_GRADIENT(depth, maxDepth) {
        const normalizedDepth = maxDepth > 0 ? depth / maxDepth : 0;

        // Purple (deep) → Green (shallow)
        const hue = 0.8 - normalizedDepth * 0.5;  // 0.8 → 0.3
        const saturation = 0.7;
        const value = 0.8;

        return this._hsvToRgb(hue, saturation, value);
    }

    /**
     * Calls-Based Heatmap
     *
     * Colors based on call count - useful for identifying frequently called functions.
     *
     * @param {number} calls - Number of calls
     * @param {number} maxCalls - Maximum calls in graph
     * @returns {string} RGB color string
     */
    static CALLS_HEATMAP(calls, maxCalls) {
        const score = maxCalls > 0 ? Math.min(calls / maxCalls, 1.0) : 0;

        // White (few calls) → Red (many calls)
        if (score < 0.5) {
            // White → Yellow
            const t = score * 2;
            return `rgb(255,255,${Math.round(255 * (1 - t))})`;
        } else {
            // Yellow → Red
            const t = (score - 0.5) * 2;
            return `rgb(255,${Math.round(255 * (1 - t))},0)`;
        }
    }

    /**
     * Memory Gradient
     *
     * Colors based on memory usage for memory profiling analysis.
     *
     * @param {number} memory - Memory metric value
     * @param {number} maxMemory - Maximum memory in graph
     * @returns {string} RGB color string
     */
    static MEMORY_GRADIENT(memory, maxMemory) {
        const score = maxMemory > 0 ? Math.min(memory / maxMemory, 1.0) : 0;

        // Green (low memory) → Red (high memory)
        const hue = (1 - score) * 0.33;  // 0.33 (green) → 0 (red)
        const saturation = 0.8;
        const value = 0.9;

        return this._hsvToRgb(hue, saturation, value);
    }

    /**
     * Differential Coloring (for profile comparison)
     *
     * Colors based on performance change between two profiles.
     *
     * @param {number} percentChange - Percent change (negative = improvement)
     * @returns {string} RGB color string
     */
    static DIFFERENTIAL(percentChange) {
        if (percentChange === 0) {
            return '#666';  // Gray for no change
        }

        const absChange = Math.abs(percentChange);
        const intensity = Math.min(absChange / 100, 1.0);

        if (percentChange > 0) {
            // Regression (slower) - Red
            return this._hsvToRgb(0, 0.8, 0.3 + intensity * 0.7);
        } else {
            // Improvement (faster) - Green
            return this._hsvToRgb(0.33, 0.8, 0.3 + intensity * 0.7);
        }
    }

    /**
     * Helper: Convert HSV to RGB
     *
     * @param {number} h - Hue [0, 1]
     * @param {number} s - Saturation [0, 1]
     * @param {number} v - Value [0, 1]
     * @returns {string} RGB color string
     */
    static _hsvToRgb(h, s, v) {
        let r, g, b;

        const i = Math.floor(h * 6);
        const f = h * 6 - i;
        const p = v * (1 - s);
        const q = v * (1 - f * s);
        const t = v * (1 - (1 - f) * s);

        switch (i % 6) {
            case 0: r = v; g = t; b = p; break;
            case 1: r = q; g = v; b = p; break;
            case 2: r = p; g = v; b = t; break;
            case 3: r = p; g = q; b = v; break;
            case 4: r = t; g = p; b = v; break;
            case 5: r = v; g = p; b = q; break;
        }

        return `rgb(${Math.round(r * 255)},${Math.round(g * 255)},${Math.round(b * 255)})`;
    }

    /**
     * Helper: Parse RGB color string
     *
     * @param {string} colorStr - RGB color string
     * @returns {object|null} {r, g, b} or null if invalid
     */
    static _parseRgb(colorStr) {
        // Handle hex colors
        if (colorStr.startsWith('#')) {
            const hex = colorStr.substring(1);
            if (hex.length === 3) {
                return {
                    r: parseInt(hex[0] + hex[0], 16),
                    g: parseInt(hex[1] + hex[1], 16),
                    b: parseInt(hex[2] + hex[2], 16)
                };
            } else if (hex.length === 6) {
                return {
                    r: parseInt(hex.substring(0, 2), 16),
                    g: parseInt(hex.substring(2, 4), 16),
                    b: parseInt(hex.substring(4, 6), 16)
                };
            }
        }

        // Handle rgb() format
        const match = colorStr.match(/rgb\((\d+),\s*(\d+),\s*(\d+)\)/);
        if (match) {
            return {
                r: parseInt(match[1]),
                g: parseInt(match[2]),
                b: parseInt(match[3])
            };
        }

        return null;
    }

    /**
     * Get color scheme by name
     *
     * @param {string} schemeName - Name of color scheme
     * @returns {function} Color scheme function
     */
    static getScheme(schemeName) {
        const schemes = {
            'hotpath': this.HOTPATH_RED,
            'gradient': this.GRADIENT_BLUE_RED,
            'flame': this.FLAME_CLASSIC,
            'category': this.CATEGORY_BASED,
            'category-hot': this.CATEGORY_WITH_HOTPATH,
            'depth': this.DEPTH_GRADIENT,
            'calls': this.CALLS_HEATMAP,
            'memory': this.MEMORY_GRADIENT,
            'diff': this.DIFFERENTIAL
        };

        return schemes[schemeName] || this.CATEGORY_BASED;
    }

    /**
     * Get all available color scheme names
     *
     * @returns {Array<object>} Array of {value, label} objects
     */
    static getAvailableSchemes() {
        return [
            { value: 'hotpath', label: 'Hot Path (Red Gradient)' },
            { value: 'gradient', label: 'Temperature (Blue-Red)' },
            { value: 'flame', label: 'Flame (Classic)' },
            { value: 'category', label: 'Category (Type-Based)' },
            { value: 'category-hot', label: 'Category + Hot Path' },
            { value: 'depth', label: 'Depth (Purple-Green)' },
            { value: 'calls', label: 'Calls (Heatmap)' },
            { value: 'memory', label: 'Memory (Green-Red)' }
        ];
    }
}

/**
 * Color Scheme Manager
 *
 * Manages current color scheme selection and provides
 * easy access to coloring functions for widgets.
 */
export class ColorSchemeManager {
    constructor() {
        this.currentScheme = 'category';
        this.schemeFunction = ColorSchemes.getScheme(this.currentScheme);
    }

    /**
     * Set current color scheme
     *
     * @param {string} schemeName - Name of scheme to activate
     */
    setScheme(schemeName) {
        this.currentScheme = schemeName;
        this.schemeFunction = ColorSchemes.getScheme(schemeName);
    }

    /**
     * Get current scheme name
     *
     * @returns {string} Current scheme name
     */
    getCurrentScheme() {
        return this.currentScheme;
    }

    /**
     * Get color for a node based on current scheme
     *
     * @param {object} node - CallGraphNode
     * @param {object} graphData - CallGraphData for context
     * @returns {string} RGB color string
     */
    getNodeColor(node, graphData) {
        const functionName = node.getFunctionName();

        switch (this.currentScheme) {
            case 'hotpath':
                if (node.isOnHotPath) {
                    return ColorSchemes.HOTPATH_RED(node.hotPathScore);
                }
                // Non-hot nodes get a muted color
                return '#444';

            case 'gradient':
                const incMetric = node.getInclusiveMetric(graphData.currentMetric);
                const maxMetric = graphData.criticalPathLength;
                const score = maxMetric > 0 ? incMetric / maxMetric : 0;
                return ColorSchemes.GRADIENT_BLUE_RED(score);

            case 'flame':
                const depth = node.getDepth();
                const metricScore = node.getInclusiveMetric(graphData.currentMetric) /
                                   Math.max(1, graphData.criticalPathLength);
                return ColorSchemes.FLAME_CLASSIC(metricScore, depth);

            case 'category':
                return ColorSchemes.CATEGORY_BASED(functionName);

            case 'category-hot':
                return ColorSchemes.CATEGORY_WITH_HOTPATH(
                    functionName,
                    node.isOnHotPath,
                    node.hotPathScore
                );

            case 'depth':
                const stats = graphData.getStats();
                return ColorSchemes.DEPTH_GRADIENT(node.getDepth(), stats.maxDepth);

            case 'calls':
                const maxCalls = Math.max(...Array.from(graphData.nodes.values())
                    .map(n => n.getCalled()));
                return ColorSchemes.CALLS_HEATMAP(node.getCalled(), maxCalls);

            case 'memory':
                // Assume we're using a memory metric
                const memValue = node.getInclusiveMetric(graphData.currentMetric);
                const maxMem = graphData.criticalPathLength;
                return ColorSchemes.MEMORY_GRADIENT(memValue, maxMem);

            default:
                return ColorSchemes.CATEGORY_BASED(functionName);
        }
    }

    /**
     * Get color for an edge based on current scheme
     *
     * @param {object} edge - CallGraphEdge
     * @returns {string} RGB color string
     */
    getEdgeColor(edge) {
        if (edge.isOnHotPath) {
            return ColorSchemes.HOTPATH_RED(edge.hotPathScore);
        }
        return '#444';  // Default edge color
    }

    /**
     * Get appropriate arrowhead marker ID for edge
     *
     * @param {object} edge - CallGraphEdge
     * @param {boolean} isHighlighted - Is edge highlighted
     * @returns {string} Marker ID (e.g., "arrowhead", "arrowhead-hot")
     */
    getEdgeMarker(edge, isHighlighted = false) {
        if (isHighlighted) {
            return 'url(#arrowhead-highlight)';
        }
        if (edge.isOnHotPath) {
            return 'url(#arrowhead-hot)';
        }
        return 'url(#arrowhead)';
    }
}
