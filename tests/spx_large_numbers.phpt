--TEST--
Large numbers and edge cases for string formatting
--ENV--
return <<<END
SPX_ENABLED=1
SPX_METRICS=wt,zm
SPX_FP_FOCUS=wall-time
SPX_FP_INC=1
SPX_FP_REL=1
SPX_FP_LIMIT=10
END;
--FILE--
<?php

// Test formatting of various number types and edge cases
// This exercises spx_str_builder_append_* functions

function test_large_array() {
    // Create a large array to generate large memory numbers
    $arr = range(1, 100000);
    $sum = array_sum($arr);
    return $sum;
}

function test_nested_calls($depth) {
    if ($depth <= 0) {
        return 1;
    }
    return test_nested_calls($depth - 1) + 1;
}

function test_string_operations() {
    $str = str_repeat("x", 10000);
    return strlen($str);
}

echo "Testing large number formatting...\n";

$result1 = test_large_array();
$result2 = test_nested_calls(50);
$result3 = test_string_operations();

echo "Results: $result1, $result2, $result3\n";
echo "Test complete\n";
?>
--EXPECTF--
Testing large number formatting...
Results: 5000050000, 51, 10000
Test complete

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
