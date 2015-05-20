# coding: utf-8
# lib = File.expand_path('../lib', __FILE__)
# $LOAD_PATH.unshift(lib) unless $LOAD_PATH.include?(lib)
require File.expand_path('../lib/shall/version', __FILE__)

Gem::Specification.new do |s|
    s.name = 'shall'
    s.version = Shall::VERSION
    s.homepage = 'https://github.com/julp/shall'
    s.summary = 'Ruby bindings for Shall, syntax highlighter'
    s.authors = ['julp']
    s.platform = Gem::Platform::RUBY
    s.bindir = 'bin'
    s.extensions = %w[ext/shall/extconf.rb]
    s.files = Dir.glob ['Rakefile', 'Gemfile', 'shall.gemspec', 'lib/**/*.rb', 'ext/**/*.[ch]']
    s.required_ruby_version = '>= 2.0.0'
    s.license = 'BSD'
#     s.require_paths = %w[lib ext]
    s.add_development_dependency 'rake-compiler'
    s.add_development_dependency 'rake'
end
