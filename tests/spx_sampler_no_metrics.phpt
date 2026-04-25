--TEST--
Sampler mode runs and produces a report when given an explicit metric list
--ENV--
return <<<END
SPX_ENABLED=1
SPX_SAMPLING_PERIOD=5000
SPX_METRICS=wt
SPX_FP_LIMIT=3
END;
--FILE--
<?php
function busy() {
    $s = 0;
    for ($i = 0; $i < 50000; $i++) {
        $s += $i;
    }
    return $s;
}

busy();
busy();
echo "ok\n";
?>
--EXPECTF--
ok

*** SPX Report ***
%a
