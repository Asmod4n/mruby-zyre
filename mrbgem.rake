MRuby::Gem::Specification.new('mruby-zyre') do |spec|
  spec.license = 'Apache-2'
  spec.author  = 'Hendrik Beskow'
  spec.summary = 'actor library for mruby'
  spec.add_dependency 'mruby-errno'
  spec.add_dependency 'mruby-czmq'
  spec.linker.libraries << 'zyre'
end
