# -*- coding: utf-8 -*-

try:
    import gc
    import shall

    gc.enable()
    gc.set_debug(gc.DEBUG_LEAK)

    code = """\
#!/usr/bin/env php54

echo 'Hello world';
?><html><style>background: <?= $foo ?>;\
"""

    print shall.lexer_guess(code, { 'start_inline': True })
    print shall.lexer_by_name('php')

    lexer = shall.PHP()
    lexer.set_option('start_inline', True)
    print lexer.get_option('start_inline')

    print shall.PHP.name, lexer.name, lexer.get_name()
    print shall.PHP.aliases, lexer.aliases, lexer.get_aliases()
    print shall.PHP.mimetypes, lexer.mimetypes, lexer.get_mimetypes()

    print shall.highlight(code, [ lexer ], shall.TerminalFormatter())

    class MyFormatter(shall.BaseFormatter):
        def start_document(self):
            return '<document>'

        def end_document(self):
            return '</document>'

        def start_token(self, ttype):
            return '<token>'

        def end_token(self, ttype):
            return '</token>'

        def write_token(self, token):
            return token

    class MyInheritedFormatter(shall.HTMLFormatter):
        def __init__(self):
            #super(MyInheritedFormatter, self).__init__()
            pass

        def start_document(self):
            super(MyInheritedFormatter, self).start_document()
            return 'XXX'

    print shall.highlight(code, [ shall.PHP() ], MyFormatter()) # plante

    print '=' * 20

    php = shall.PHP({'start_inline': True})
    print php.set_option('secondary', shall.XML())
    print php.get_option('secondary')

    print shall.highlight(code, [ php ], shall.TerminalFormatter())

    print '=' * 20

    print shall.highlight(code, [ shall.PHP() ], MyInheritedFormatter()) # plante

    print '=' * 20

    print shall.highlight(code, [ shall.lexer_guess(code, { 'start_inline': True }) ], shall.TerminalFormatter())

    gc.collect()

    for x in gc.garbage:
        print "[GC] %s (%s)\n" % [ str(x), type(x) ]

except:
    import traceback
    var = traceback.format_exc()
    print var
