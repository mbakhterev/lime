echo = echo -e

cc = gcc -Wall -Werror -pedantic -c -pipe
cflags =
coptimization = -flto
cdebug = -g -flto
cstd = c1x
cppstd = c++0x
dep = $(cc)

lnk = gcc -pipe
lflags =
loptimization = -flto -Wl,-s -O3 -march=native -mtune=native
ldebug = -g -flto

ar = ar

lex = flex -Caer -8 --yylineno --bison-locations

yacc = bison -Wall -L C --locations

# Переменные для подстройки конкретных целей 

c99lexfix = -Wno-unused-but-set-variable
