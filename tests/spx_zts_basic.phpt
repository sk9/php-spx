--TEST--
ZTS build: profiler initialises and produces a report (skipped on NTS)
--SKIPIF--
<?php
if (!ZEND_THREAD_SAFE) {
    die('skip ZTS-only test');
}
?>
--ENV--
return <<<END
SPX_ENABLED=1
SPX_BUILTINS=1
END;
--FILE--
<?php
function loop_a() {
    $sum = 0;
    for ($i = 0; $i < 100; $i++) { $sum += $i; }
    return $sum;
}

function loop_b() {
    return strlen(str_repeat('x', 100));
}

loop_a();
loop_b();
echo "ok\n";
?>
--EXPECTF--
ok
*** SPX Report ***
%a
