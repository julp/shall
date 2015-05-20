require 'mkmf'

dir_config('shall')

if enable_config('debug')
    $defs.push('-DDEBUG') unless $defs.include? '-DDEBUG'
end

unless find_header('shall/shall.h')
    abort 'shall.h not found'
end

unless find_library('shall', 'lexer_implementation_by_name')
    abort 'libshall not found'
end

create_makefile('shall/shall')
