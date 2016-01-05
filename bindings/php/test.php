<?php
$code = <<<'EOS'
echo 'Hello world';
EOS;

$code2 = <<<'EOS'
echo 'Hello world'; ?><p class="foo">
EOS;

$options = ['start_inline' => TRUE];

class MyFormatter extends Shall\Formatter\Base {
    public function start_document() {
        return '<document>';
    }

    public function end_document() {
        return '</document>';
    }

    public function start_token($type) {
        return '<token>';
    }

    public function end_token($type) {
        return '</token>';
    }

    public function write_token($token) {
        return $token;
    }
}

class MyInheritedFormatter extends Shall\Formatter\HTML {
    public function start_document() {
        call_user_func_array(array(get_parent_class(), __FUNCTION__), func_get_args());
        return 'XXX';
    }
}

$phplexer = new Shall\Lexer\PHP($options);
$htmlfmt = new Shall\Formatter\HTML(array('cssclass' => 'abc'));
$inhritedfmt = new MyInheritedFormatter(array('foo' => 'bar'));
$myfmt = new MyFormatter(array('foo' => 'bar'));

var_dump(
    Shall\lexer_guess('#!/usr/bin/env php54' . PHP_EOL . $code),
    get_class(Shall\lexer_by_name('php')),
    Shall\lexer_by_name('XXX'),
    Shall\highlight($code, Shall\lexer_by_name('php', $options), $htmlfmt),
    Shall\highlight($code, Shall\lexer_for_filename(__FILE__, $options), $htmlfmt),
    Shall\highlight($code, $phplexer, new Shall\Formatter\Terminal),
    str_repeat('=', 20),
    Shall\highlight($code, $phplexer, $myfmt),
    Shall\highlight($code, $phplexer, $inhritedfmt),
    str_repeat('=', 20),
    'start_inline = ',
    $phplexer->getOption('start_inline'),
    'secondary = ',
    $phplexer->getOption('secondary'),
    $phplexer->setOption('secondary', new Shall\Lexer\XML), # 2 LEAKS
    'secondary = ',
    $phplexer->getOption('secondary'),
# segfault si on commente la fin à partir d'ici
    Shall\highlight($code2, $phplexer, $htmlfmt),
//     $x = new Shall\Lexer\Varnish,
//     $x->getOption('version'),
//     $x->setOption('version', 6),
//     $x->getOption('version'),
//     new Shall\Lexer\Base,
    NULL
);

var_dump(
    $htmlfmt->getOption('cssclass'),
    $htmlfmt->setOption('cssclass', 'def'),
    $htmlfmt->getOption('cssclass'),
    str_repeat('=', 20),
    $inhritedfmt->getOption('cssclass'),
    $inhritedfmt->getOption('foo'),
    str_repeat('=', 20),
    $myfmt->getOption('foo'),
    NULL
);
