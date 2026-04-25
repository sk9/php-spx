--TEST--
Profiler runs cleanly when auto_prepend_file is set
--INI--
auto_prepend_file={PWD}/spx_auto_prepend_helper.inc
--ENV--
return <<<END
SPX_ENABLED=1
SPX_FP_LIMIT=10
END;
--FILE--
<?php
function main_function() {
    return prepended_helper() * 2;
}

echo "result=", main_function(), "\n";
?>
--EXPECTF--
result=84

*** SPX Report ***
%a
