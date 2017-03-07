MRuby::Gem::Specification.new('mruby-zyre') do |spec|
  spec.license = 'Apache-2'
  spec.author  = 'Hendrik Beskow'
  spec.summary = 'zyre bindings for mruby'
  spec.add_dependency 'mruby-errno'
  spec.linker.libraries << 'zmq' << 'czmq' << 'zyre'
end
