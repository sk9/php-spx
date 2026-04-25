--TEST--
Invalid config values are rejected and reported (instead of crashing or silent default)
--INI--
log_errors=on
--ENV--
return <<<END
SPX_ENABLED=1
SPX_SAMPLING_PERIOD=garbage
SPX_FP_LIMIT=-5
END;
--FILE--
<?php
echo 'Normal output';
?>
--EXPECTF--
PHP Notice:  SPX: Invalid sampling_period value: garbage in Unknown on line 0

Notice: SPX: Invalid sampling_period value: garbage in Unknown on line 0
SPX Error [SPX_ERR_INVALID_INPUT]: Invalid characters in input: 'garbage' at position 0
  at %s/spx_security_validation.c:%d in spx_parse_long
PHP Notice:  SPX: Invalid fp_limit value: -5 in Unknown on line 0

Notice: SPX: Invalid fp_limit value: -5 in Unknown on line 0
SPX Error [SPX_ERR_OUT_OF_RANGE]: Value -5 out of range [0, 1000000]
  at %s/spx_security_validation.c:%d in spx_parse_long
Normal output
*** SPX Report ***
%a
