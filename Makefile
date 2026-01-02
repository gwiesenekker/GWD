OBJS=main.o\
  bstrlib.o\
  cJSON.o\
  xxhash.o\
  my_safec.o\
  boards.o\
  book.o\
  buckets.o\
  caches.o\
  classes.o\
  compat.o\
  compress.o\
  dxp.o\
  endgame.o\
  fbuffer.o\
  hub.o\
  mcts.o\
  moves.o\
  my_bstreams.o\
  my_cjson.o\
  my_malloc.o\
  my_mpi.o\
  my_printf.o\
  my_random.o\
  my_sqlite3.o\
  networks.o\
  patterns.o\
  profile.o\
  pdn.o\
  queues.o\
  records.o\
  score.o\
  search.o\
  states.o\
  stats.o\
  my_threads.o\
  timers.o\
  utils.o

ARCH := $(shell uname -m)

ifneq ($(ARCH), aarch64)
#ARCH=skylake
#ARCH=alderlake
#ARCH=nocona
#ARCH=znver3
ARCH=native
endif

MAKE_VALGRIND=make_valgrind
MAKE_SQLITE3=make_sqlite3
MAKE_OPENMPI=make_openmpi

TARGET=a.out

ifdef MAKE_DEBUG
CC=gcc
CFLAGS=-g -DDEBUG
else
CC=clang
CFLAGS=-O3 -g 
endif
LFLAGS=-lm -lzstd -lpthread

ifdef MAKE_VALGRIND
CFLAGS+=-DUSE_VALGRIND 
endif

ifdef MAKE_SQLITE3
CFLAGS+=-DUSE_SQLITE3
endif

ifdef MAKE_OPENMPI
CFLAGS+=-DUSE_OPENMPI
CFLAGS+=-I/usr/local/include -I/usr/lib/x86_64-linux-gnu/openmpi/include
LFLAGS+=-L/usr/local/lib -lmpi -lpmix
endif

ifdef MAKE_SQLITE3
LFLAGS+=-lsqlite3
endif

ifdef MAKE_PROFILE
CFLAGS+=-DPROFILE
endif

CFLAGS+=-D_GNU_SOURCE

ifeq ($(ARCH), aarch64)
CFLAGS+=-mcpu=neoverse-n1
#CFLAGS+=-march=armv8-a+simd+crc
else
CFLAGS+=-march=$(ARCH)
CFLAGS+=-DUSE_AVX2_INTRINSICS -DXXH_VECTOR=XXH_AVX2 
endif

ifeq ($(CC), gcc)
CFLAGS+=-fomit-frame-pointer
CFLAGS+=-Wno-stringop-truncation
CFLAGS+=-Wno-format-truncation
#CFLAGS+=-Wl,--no-insert-timestamp
CFLAGS+=-Wpointer-arith
CFLAGS+=-Wunused-macros
endif

ifeq ($(CC), clang)
CFLAGS+=-Wall
endif

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) -o $@ $(CFLAGS) $(OBJS) $(LFLAGS)
	REVISION=$$(./$(TARGET) --revision); echo $$REVISION;\
        mkdir -p /tmp8/gwies/dxp1/$$REVISION;\
        cp $(TARGET) gwd.json overrides.json networks.json ballot2.fen /tmp8/gwies/dxp1/$$REVISION;\
        mkdir -p hub_client/$$REVISION;\
        cp $(TARGET) gwd.json overrides.json networks.json hub_client/$$REVISION

HEADERS=boards.h\
  bstrlib.h\
  cJSON.h\
  xxhash.h\
  my_safec.h\
  book.h\
  buckets.h\
  caches.h\
  classes.h\
  compat.h\
  compress.h\
  dxp.h\
  endgame.h\
  fbuffer.h\
  globals.h\
  hub.h\
  main.h\
  mcts.h\
  moves.h\
  my_bstreams.h\
  my_cjson.h\
  my_malloc.h\
  my_printf.h\
  my_mpi.h\
  my_random.h\
  my_sqlite3.h\
  networks.h\
  patterns.h\
  pdn.h\
  profile.h\
  queues.h\
  records.h\
  score.h\
  search.h\
  states.h\
  stats.h\
  my_threads.h\
  timers.h\
  utils.h

main.o: Makefile main.c $(HEADERS)

bstrlib.o: Makefile bstrlib.c $(HEADERS)
cJSON.o: Makefile cJSON.c $(HEADERS)
xxhash.o: Makefile xxhash.c $(HEADERS)
my_safec.o: Makefile my_safec.c $(HEADERS)
boards.o: Makefile $(HEADERS)
book.o: Makefile book.c $(HEADERS)
my_bstreams.o: Makefile my_bstreams.c $(HEADERS)
buckets.o: Makefile buckets.c $(HEADERS)
caches.o: Makefile caches.c $(HEADERS)
compat.o: Makefile compat.c $(HEADERS)
compress.o: Makefile compress.c $(HEADERS)
classes.o: Makefile classes.c $(HEADERS)
dxp.o: Makefile dxp.c $(HEADERS)
endgame.o: Makefile endgame.c $(HEADERS)
fbuffer.o: Makefile fbuffer.c $(HEADERS)
hub.o: Makefile hub.c $(HEADERS)
mcts.o: Makefile mcts.c $(HEADERS)
moves.o: Makefile moves.c $(HEADERS)
my_cjson.o: Makefile my_cjson.c $(HEADERS)
my_mpi.o: Makefile my_mpi.c $(HEADERS)
my_malloc.o: Makefile my_malloc.c $(HEADERS)
my_printf.o: Makefile my_printf.c $(HEADERS)
my_random.o: Makefile my_random.c $(HEADERS)
my_sqlite3.o: Makefile my_sqlite3.c $(HEADERS)
networks.o: Makefile networks.c $(HEADERS)
patterns.o: Makefile patterns.c $(HEADERS)
profile.o: Makefile profile.c profile.h 
pdn.o: Makefile pdn.c $(HEADERS)
queues.o: Makefile queues.c $(HEADERS)
records.o: Makefile records.c $(HEADERS)
score.o: Makefile score.c $(HEADERS)
search.o: Makefile search.c $(HEADERS)
states.o: Makefile states.c $(HEADERS)
stats.o: Makefile stats.c $(HEADERS)
my_threads.o: Makefile my_threads.c $(HEADERS)
timers.o: Makefile timers.c $(HEADERS)
utils.o: Makefile utils.c $(HEADERS)

cppcheck:
	clear
	cppcheck --enable=all \
	--suppress=missingIncludeSystem \
	--suppress=constParameterPointer \
	--suppress=constVariablePointer \
	--suppress=constParameter \
	--suppress=constParameterCallback \
	--check-level=exhaustive \
	--inconclusive \
	--force -D__cppcheck__ -i bstrlib.c -i cJSON.c -i xxhash.c .

clean:
	rm *.o logs/*

