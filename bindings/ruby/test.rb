require 'shall'

class MyFormatter < Shall::Formatter::Base
    def start_document
        '<document>'
    end

    def end_document
        '</document>'
    end

    def start_token(type)
        '<token>'
    end

    def end_token(type)
        '</token>'
    end

    def write_token(token)
        token
    end
end

class MyInheritedFormatter < Shall::Formatter::HTML
    def start_document
        super
        'XXX'
    end
end

code = <<-EOS
<?php
echo 'Hello world', 42;
?>
<html>
EOS

puts Shall::highlight code, Shall::Lexer::PHP.new, MyFormatter.new

puts '=' * 20

php = Shall::Lexer::PHP.new
puts php.get_option(:secondary).inspect
php.set_option :secondary, Shall::Lexer::XML.new
puts php.get_option(:secondary).inspect

puts Shall::highlight code, php, Shall::Formatter::Terminal.new

puts '=' * 20

puts Shall::highlight code, Shall::Lexer::PHP.new, MyInheritedFormatter.new

puts '=' * 20

code = <<-EOS
#!/usr/bin/env php54

echo 'Hello world', 42;
EOS

puts Shall::highlight code, Shall::lexer_guess(code, { start_inline: true }), Shall::Formatter::Terminal.new

puts '=' * 20

# ruby -Ilib:ext -r shall test.rb
