MRuby::Gem::Specification.new('mruby-zyre') do |spec|
  spec.license = 'MPL-2'
  spec.author  = 'Hendrik Beskow'
  spec.summary = 'Zyre - an open-source framework for proximity-based peer-to-peer applications'
  spec.add_dependency 'mruby-errno'
  spec.linker.libraries << 'zmq' << 'czmq' << 'zyre'
end
