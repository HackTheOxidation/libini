include = Dir('include')
env = Environment(CPPPATH=include)
env.MergeFlags(env.ParseFlags("-std=c++20 -O3 -Wall -Wextra -Wshadow -Wconversion -Wpedantic -Werror"))
libini = env.SharedLibrary('libini', ['src/reader.cpp', 'src/lexer.cpp', 'src/parser.cpp'])

env.Install('/usr/lib', libini)
env.Alias('install', '/usr/lib')

Mkdir("/usr/include/libini")
env.Install('/usr/include/libini', ['include/lexer.hpp', 'include/libini.h', 'include/parser.hpp', 'include/tokens.hpp'])
env.Alias('install', '/usr/include/libini')
