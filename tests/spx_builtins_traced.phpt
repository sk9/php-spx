--TEST--
Built-in functions traced
--ENV--
return <<<END
SPX_ENABLED=1
SPX_BUILTINS=1
SPX_FP_FOCUS=wall-time
SPX_FP_INC=1
SPX_FP_REL=1
SPX_FP_LIMIT=20
END;
--FILE--
<?php

function test_builtins() {
    $arr = range(1, 100);
    $sum = array_sum($arr);
    $filtered = array_filter($arr, function($n) { return $n % 2 == 0; });
    $mapped = array_map(function($n) { return $n * 2; }, $filtered);
    return count($mapped);
}

echo "Testing builtin tracing...\n";
$result = test_builtins();
echo "Result: $result\n";
?>
--EXPECTF--
Testing builtin tracing...
Result: 50

*** SPX Report ***

Global stats:

  Called functions    : %s
  Distinct functions  : %s

  Wall time           :  %s
  ZE memory usage     :  %s

Flat profile:

 Wall time           | ZE memory usage     |
 *Inc.    | Exc.     | Inc.     | Exc.     | Called   | Function
----------+----------+----------+----------+----------+----------
%a
