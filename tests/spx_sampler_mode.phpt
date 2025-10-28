--TEST--
Sampler mode profiling
--ENV--
return <<<END
SPX_ENABLED=1
SPX_SAMPLING_PERIOD=10000
SPX_FP_FOCUS=wall-time
SPX_FP_INC=1
SPX_FP_REL=1
SPX_FP_LIMIT=5
END;
--FILE--
<?php

function work_function($iterations) {
    $result = 0;
    for ($i = 0; $i < $iterations; $i++) {
        $result += sin($i) * cos($i);
    }
    return $result;
}

echo "Testing sampler mode...\n";
$result = work_function(100000);
echo "Work done: " . (int)$result . "\n";
?>
--EXPECTF--
Testing sampler mode...
Work done: %d
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
