<?php
$re = new ReflectionExtension('shall');
foreach ($re->getConstants() as $c => $v) {
    if (preg_match('~^Shall\\\\Token\\\\~', $c)) {
        printf('const %s;' . PHP_EOL, str_replace('Shall\\Token\\', '', $c));
    }
}
