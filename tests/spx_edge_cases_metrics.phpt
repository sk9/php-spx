--TEST--
Edge cases for metric collection and reporting
--ENV--
return <<<END
SPX_ENABLED=1
SPX_BUILTINS=1
SPX_METRICS=wt,ct,it,zm,io
SPX_FP_FOCUS=ct
SPX_FP_INC=1
SPX_FP_REL=1
SPX_FP_LIMIT=20
END;
--FILE--
<?php

// Test with multiple metrics and different focus
// This exercises reporter formatting for various metric types

function empty_function() {
    // Intentionally empty
}

function cpu_intensive() {
    $sum = 0;
    for ($i = 0; $i < 1000; $i++) {
        $sum += sqrt($i);
    }
    return $sum;
}

function memory_intensive() {
    $data = [];
    for ($i = 0; $i < 1000; $i++) {
        $data[] = range(1, 100);
    }
    return count($data);
}

function mixed_workload() {
    empty_function();
    cpu_intensive();
    memory_intensive();
}

echo "Testing edge cases...\n";

for ($i = 0; $i < 10; $i++) {
    mixed_workload();
}

echo "Test complete\n";
?>
--EXPECTF--
Testing edge cases...
Test complete

*** SPX Report ***

Global stats:

  Called functions    : %s
  Distinct functions  : %s

  Wall time           :  %s
  CPU time            :  %s
  Idle time           :  %s
  ZE memory usage     :  %s
  I/O Bytes           :  %s

Flat profile:

 Wall time           | CPU time            | Idle time           | ZE memory usage     | I/O Bytes           |
 Inc.     | Exc.     | *Inc.    | Exc.     | Inc.     | Exc.     | Inc.     | Exc.     | Inc.     | Exc.     | Called   | Function
----------+----------+----------+----------+----------+----------+----------+----------+----------+----------+----------+----------
%a
