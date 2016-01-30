# coding: utf-8

version = File.read(File.expand_path('../../../CMakeLists.txt', __FILE__)).scan(/set\(SHALL_VERSION_[[:upper:]]+ (\d+)\)/).join('.')

Gem::Specification.new do |s|
    s.name = 'shall'
    s.version = version
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
