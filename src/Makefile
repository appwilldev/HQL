#
# make file for hql parser
#
# author : KDr2
#

OPENRESTY?=/usr/local/openresty
LUA_VERSION?=5.1

uname_S := $(shell sh -c 'uname -s 2>/dev/null || echo not')

ifeq ($(uname_S),Darwin)
  DYLIB_CC_OPT=-bundle -undefined dynamic_lookup
else
  DYLIB_CC_OPT=-shared
endif

CXXFLAGS += -Wall -fPIC -I../vendor/libjson -O2

LUA_EXTRA_FLAG := $(shell sh -c 'pkg-config lua$(LUA_VERSION) --cflags 2>/dev/null')
ifeq ($(LUA_EXTRA_FLAG)X,X)
  LUA_EXTRA_FLAG := -I$(OPENRESTY)/luajit/include/luajit-2.0 -L$(OPENRESTY)/luajit/lib -lluajit-5.1
endif

PY_EXTRA_FLAG := $(shell sh -c 'pkg-config python-2.7 --cflags 2>/dev/null')


all: parser

clean:
	-rm parser.cpp parser.hpp tokens.cpp
	-rm ../parser
	-rm *.o *.so ../*.so

distclean: clean
	-rm ../vendor/*.a
	cd ../vendor/libjson; make clean

# parser
parser.cpp: parser.y
	-bison -v -d -o $@ $^

parser.hpp: parser.cpp

tokens.cpp: tokens.l parser.hpp
	-flex -o $@ $^

# libraries
../vendor/libjson.a:
	cd ../vendor/libjson;make && cp libjson.a ..

# objs
parser.o: cmd.hpp cmn_util.hpp concrete_ast.hpp parser.hpp parser.cpp
tokens.o: cmd.hpp ast.hpp parser.hpp tokens.cpp

type_config.o: type_config.hpp type_config.cpp

ast.o: parser.hpp cmn_util.hpp ast_util.hpp ast.hpp ast.cpp
ast_basic.o: ast.hpp concrete_ast.hpp ast_basic.cpp model.hpp
ast_sl.o: ast.hpp concrete_ast.hpp ast_sl.cpp model.hpp
ast_slfk.o: ast.hpp concrete_ast.hpp ast_slfk.cpp model.hpp
ast_rl.o: ast.hpp concrete_ast.hpp ast_rl.cpp model.hpp
ast_logic.o: ast.hpp ast_util.hpp concrete_ast.hpp model.hpp ast_logic.cpp
ast_misc.o: cmn_util.hpp ast.hpp concrete_ast.hpp model.hpp ast_misc.cpp
ast_util.o: ast.hpp ast_util.hpp ast_util.cpp
trollers.o: cmn_util.hpp ast_util.hpp ast.hpp type_config.hpp trollers.hpp model.hpp trollers.cpp
hql_xpc.o: ast_util.hpp trollers.hpp hql_xpc.hpp hql_xpc.cpp
cmd.o: cmd.hpp cmd.cpp
test.o: parser.hpp ast.hpp test.cpp type_config.hpp cmn_util.hpp
model.o: model.hpp model.cpp

lua_api.o: parser.hpp cmn_util.hpp type_config.hpp ast.hpp lua_api.cpp
lua_api.o: CXXFLAGS += $(LUA_EXTRA_FLAG)

python_api.o: parser.hpp cmn_util.hpp type_config.hpp ast.hpp python_api.cpp
python_api.o: CXXFLAGS += $(PY_EXTRA_FLAG)

OBJS = parser.o tokens.o ast.o ast_util.o cmd.o  \
       ast_basic.o ast_sl.o ast_slfk.o ast_rl.o ast_logic.o ast_misc.o \
       model.o trollers.o type_config.o hql_xpc.o

parser: $(OBJS) test.o ../vendor/libjson.a
	g++ -Wall -o ../$@ $^

luamod: $(OBJS) lua_api.o ../vendor/libjson.a
	@echo "building lua module: hql"
	g++ $(DYLIB_CC_OPT) -o ../hql.so $^

pymod: $(OBJS) python_api.o ../vendor/libjson.a
	@echo "building python module: hql"
	g++ $(DYLIB_CC_OPT) -o ../hql.so $^
