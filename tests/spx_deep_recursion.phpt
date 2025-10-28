--TEST--
Deep recursion handling
--ENV--
return <<<END
SPX_ENABLED=1
SPX_FP_FOCUS=wall-time
SPX_FP_INC=1
SPX_FP_REL=1
SPX_FP_LIMIT=5
SPX_DEPTH=100
END;
--FILE--
<?php

function deep_recursion($depth) {
    if ($depth <= 0) {
        return 1;
    }
    return 1 + deep_recursion($depth - 1);
}

echo "Testing deep recursion...\n";
$result = deep_recursion(50);
echo "Result: $result\n";
?>
--EXPECTF--
Testing deep recursion...
Result: 51
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
