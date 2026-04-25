--TEST--
Sampler mode handles deep recursion without crashing
--ENV--
return <<<END
SPX_ENABLED=1
SPX_SAMPLING_PERIOD=1000
SPX_FP_LIMIT=10
END;
--FILE--
<?php

function recurse($n) {
    if ($n <= 0) {
        return 0;
    }
    $sum = 0;
    for ($i = 0; $i < 50; $i++) {
        $sum += sin($i) * cos($i);
    }
    return $sum + recurse($n - 1);
}

echo "starting\n";
$result = recurse(100);
echo "done: ", (int) $result, "\n";
?>
--EXPECTF--
starting
done: %d

*** SPX Report ***
%a
