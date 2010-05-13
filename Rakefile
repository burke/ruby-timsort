require 'rubygems'
require 'rake'

task :gem => :build

begin
  require 'jeweler'
  Jeweler::Tasks.new do |gem|
    gem.name = ""
    gem.summary = ""
    gem.description = ""
    gem.email = "burke@burkelibbey.org"
    gem.homepage = "http://github.com/burke/#{gem.name}"
    gem.authors = ["Burke Libbey"]
    gem.files.include '{spec,lib}/**/*'
    gem.files.include 'ext/**/*.{c,h,rb}'
    gem.files.include 'ext/**/{Makefile,Rakefile}'
    gem.extensions = ["ext/extconf.rb"]
  end
rescue LoadError
  puts "Jeweler (or a dependency) not available. Install it with: sudo gem install jeweler"
end

begin
  require 'rcov/rcovtask'
  Rcov::RcovTask.new do |test|
    test.libs << 'spec'
    test.pattern = 'spec/**/*_spec.rb'
    test.verbose = true
  end
rescue LoadError
  task :rcov do
    abort "RCov is not available. In order to run rcov, you must: sudo gem install spicycode-rcov"
  end
end

