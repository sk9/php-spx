--TEST--
Profiler with multiple metrics enabled
--ENV--
return <<<END
SPX_ENABLED=1
SPX_METRICS=wt,zm,cpu,io
SPX_FP_FOCUS=wall-time
SPX_FP_INC=1
SPX_FP_REL=1
SPX_FP_LIMIT=5
END;
--FILE--
<?php

function memory_intensive() {
    $data = [];
    for ($i = 0; $i < 1000; $i++) {
        $data[] = str_repeat('x', 100);
    }
    return count($data);
}

function cpu_intensive() {
    $sum = 0;
    for ($i = 0; $i < 10000; $i++) {
        $sum += sqrt($i);
    }
    return $sum;
}

echo "Testing multiple metrics...\n";
$m = memory_intensive();
$c = cpu_intensive();
echo "Memory ops: $m, CPU sum: " . (int)$c . "\n";
?>
--EXPECTF--
Testing multiple metrics...
Memory ops: 1000, CPU sum: %d
*** SPX Report ***

Global stats:

  Called functions    :        %d
  Distinct functions  :        %d

  Wall time           :  %s
  ZE memory usage     :  %s
%s
Flat profile:

 Wall time           | ZE memory usage     | CPU time            | I/O wait time       |
 Inc.     | *Exc.    | Inc.     | Exc.     | Inc.     | Exc.     | Inc.     | Exc.     | Called   | Function
----------+----------+----------+----------+----------+----------+----------+----------+----------+----------
%s
