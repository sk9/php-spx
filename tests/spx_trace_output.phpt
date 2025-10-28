--TEST--
Trace output mode
--ENV--
return <<<END
SPX_ENABLED=1
SPX_REPORT=trace
SPX_TRACE_SAFE=1
SPX_BUILTINS=0
END;
--FILE--
<?php

function level3() {
    return "level3";
}

function level2() {
    return level3() . " -> level2";
}

function level1() {
    return level2() . " -> level1";
}

echo "Testing trace output...\n";
$result = level1();
echo "Result: $result\n";
?>
--EXPECTF--
Testing trace output...
Result: level3 -> level2 -> level1

SPX trace file: %s
