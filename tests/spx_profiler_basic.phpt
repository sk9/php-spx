--TEST--
Basic profiler start/stop functionality
--ENV--
return <<<END
SPX_ENABLED=1
SPX_FP_FOCUS=wall-time
SPX_FP_INC=1
SPX_FP_REL=1
SPX_FP_LIMIT=10
END;
--FILE--
<?php

function fibonacci($n) {
    if ($n <= 1) return $n;
    return fibonacci($n - 1) + fibonacci($n - 2);
}

function factorial($n) {
    $result = 1;
    for ($i = 2; $i <= $n; $i++) {
        $result *= $i;
    }
    return $result;
}

function complex_calculation() {
    $arr = [];
    for ($i = 0; $i < 100; $i++) {
        $arr[] = fibonacci(10);
    }
    return factorial(10);
}

echo "Testing profiler...\n";
$result = complex_calculation();
echo "Result: $result\n";
echo "Test complete\n";
?>
--EXPECTF--
Testing profiler...
Result: 3628800
Test complete
*** SPX Report ***

Global stats:

  Called functions    :        %d
  Distinct functions  :        %d

  Wall time           :  %s
  ZE memory usage     :  %s

Flat profile:

 Wall time           | ZE memory usage     |
 Inc.     | *Exc.    | Inc.     | Exc.     | Called   | Function
----------+----------+----------+----------+----------+----------
%s
