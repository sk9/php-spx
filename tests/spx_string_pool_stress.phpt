--TEST--
String pool stress test - many unique function names
--ENV--
return <<<END
SPX_ENABLED=1
SPX_FP_FOCUS=wall-time
SPX_FP_INC=1
SPX_FP_REL=1
SPX_FP_LIMIT=200
END;
--FILE--
<?php

// Generate many unique function names to stress the string pool
// Each function name is ~50 chars, so 200 functions = 10KB of names
// This should trigger string pool block allocation (POOL_BLOCK_SIZE = 8192)

echo "Testing string pool with many unique names...\n";

// Create functions dynamically using eval to generate many unique names
for ($i = 0; $i < 200; $i++) {
    $funcName = 'test_function_' . str_pad($i, 30, '0', STR_PAD_LEFT);
    eval("function $funcName(\$x) { return \$x * 2; }");
}

// Call each function to ensure they're profiled and their names are interned
for ($i = 0; $i < 200; $i++) {
    $funcName = 'test_function_' . str_pad($i, 30, '0', STR_PAD_LEFT);
    $funcName(42);
}

echo "All functions called\n";
?>
--EXPECTF--
Testing string pool with many unique names...
All functions called

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
