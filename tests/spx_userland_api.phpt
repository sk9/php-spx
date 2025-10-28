--TEST--
Userland API - spx_profiler_start/stop
--FILE--
<?php

function computation() {
    $sum = 0;
    for ($i = 0; $i < 1000; $i++) {
        $sum += sqrt($i);
    }
    return $sum;
}

echo "Testing userland API...\n";

// Test starting the profiler
$started = spx_profiler_start();
if ($started === false) {
    echo "SKIP: Could not start profiler\n";
} else {
    echo "Profiler started\n";

    $result = computation();

    // Test stopping the profiler
    spx_profiler_stop();
    echo "Profiler stopped\n";

    echo "Computation result: " . (int)$result . "\n";
}

echo "Test complete\n";
?>
--EXPECTF--
Testing userland API...
Profiler started
Profiler stopped
Computation result: %d
Test complete
