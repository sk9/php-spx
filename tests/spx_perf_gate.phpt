--TEST--
Tracer call-count regression gate: deterministic Called/Distinct counts
--ENV--
return <<<END
SPX_ENABLED=1
SPX_FP_LIMIT=10
END;
--FILE--
<?php

function inner($x) {
    return $x + 1;
}

function outer() {
    $sum = 0;
    for ($i = 0; $i < 99; $i++) {
        $sum += inner($i);
    }
    return $sum;
}

for ($i = 0; $i < 9; $i++) {
    outer();
}

echo "ok\n";
?>
--EXPECTF--
ok

*** SPX Report ***

Global stats:

  Called functions    :      901
  Distinct functions  :        3

%a
