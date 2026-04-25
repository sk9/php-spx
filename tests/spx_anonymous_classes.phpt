--TEST--
Profiler handles anonymous classes and closures without crashing
--ENV--
return <<<END
SPX_ENABLED=1
SPX_BUILTINS=1
SPX_FP_LIMIT=20
END;
--FILE--
<?php

$obj = new class {
    public function compute($x) {
        return $x * $x;
    }
};

$closure = function ($n) {
    $sum = 0;
    for ($i = 0; $i < $n; $i++) {
        $sum += $i;
    }
    return $sum;
};

$arrow = fn($x) => $x + 1;

$total = 0;
for ($i = 0; $i < 5; $i++) {
    $total += $obj->compute($i);
    $total += $closure($i + 10);
    $total += $arrow($i);
}

echo "total=$total\n";
?>
--EXPECTF--
total=%d

*** SPX Report ***
%a
