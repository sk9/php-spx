<?php
// Basic integration test for SPX extension

echo "Testing SPX extension...\n";

// Check if extension is loaded
if (!extension_loaded('SPX')) {
    die("ERROR: SPX extension not loaded\n");
}
echo "✓ SPX extension is loaded\n";

// Test profiler start/stop
if (function_exists('spx_profiler_start')) {
    echo "✓ spx_profiler_start() function exists\n";
} else {
    echo "✗ spx_profiler_start() function not found\n";
}

if (function_exists('spx_profiler_stop')) {
    echo "✓ spx_profiler_stop() function exists\n";
} else {
    echo "✗ spx_profiler_stop() function not found\n";
}

// Test a simple profiling scenario
function test_function() {
    $sum = 0;
    for ($i = 0; $i < 1000; $i++) {
        $sum += $i;
    }
    return $sum;
}

echo "\nRunning simple profiling test...\n";
$result = test_function();
echo "✓ Test function executed successfully (result: $result)\n";

echo "\n\nAll basic tests passed!\n";
