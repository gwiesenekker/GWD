OBJS=main.o\
  bstrlib.o\
  cJSON.o\
  xxhash.o\
  boards.o\
  book.o\
  buckets.o\
  caches.o\
  classes.o\
  compat.o\
  dbase.o\
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
  threads.o\
  timers.o\
  utils.o

ifndef ARCH
#ARCH=skylake
#ARCH=alderlake
#ARCH=nocona
ARCH=znver3
endif

CC=clang
TARGET=a.out
MAKE_VALGRIND=make_valgrind
MAKE_OPENMPI=make_openmpi

ifdef MAKE_DEBUG
CC=gcc
CFLAGS=-g -DDEBUG
else
CFLAGS=-O3 -march=$(ARCH) -g
endif

ifdef MAKE_PROFILE
CFLAGS+=-DPROFILE
endif

ifdef MAKE_VALGRIND
CFLAGS+=-DUSE_VALGRIND 
endif

ifdef MAKE_OPENMPI
CFLAGS+=-DUSE_OPENMPI -I/usr/lib/x86_64-linux-gnu/openmpi/include
endif

ifneq ($(CC), icx)
CFLAGS+=-Wno-char-subscripts -Wshadow 
endif

ifeq ($(CC), gcc)
CFLAGS+=-fomit-frame-pointer
CFLAGS+=-Wno-stringop-truncation
CFLAGS+=-Wno-format-truncation
#CFLAGS+=-Wl,--no-insert-timestamp
CFLAGS+=-Wpointer-arith
CFLAGS+=-Wunused-macros
endif

COPTS+=-D_GNU_SOURCE -mrdrnd
ifeq ($(ARCH), nocona)
COPTS+=-msse4.2
else
COPTS+=-DUSE_AVX2_INTRINSICS -DXXH_VECTOR=XXH_AVX2 -msse4.2 -mavx2 -mfma
#COPTS+=-msse4.2 -mavx2
endif
COPTS+=-Wall -pthread

CFLAGS+=$(COPTS)

LFLAGS=-lm -lzstd -lpthread -lsqlite3

ifdef MAKE_OPENMPI
LFLAGS+=-L/usr/lib/x86_64-linux-gnu/openmpi/lib -lmpi
endif

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) -o $@ $(CFLAGS) $(OBJS) $(LFLAGS)
	REVISION=$$(./$(TARGET) --revision); echo $$REVISION;\
        mkdir -p dxp_server/$$REVISION;\
        cp $(TARGET) gwd.json overrides.json networks.json dxp_server/$$REVISION;\
        mkdir -p hub_client/$$REVISION;\
        cp $(TARGET) gwd.json overrides.json networks.json hub_client/$$REVISION

HEADERS=boards.h\
  bstrlib.h\
  cJSON.h\
  xxhash.h\
  book.h\
  buckets.h\
  caches.h\
  classes.h\
  compat.h\
  dbase.h\
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
  threads.h\
  timers.h\
  utils.h

main.o: Makefile main.c $(HEADERS)

bstrlib.o: Makefile bstrlib.c $(HEADERS)
cJSON.o: Makefile cJSON.c $(HEADERS)
xxhash.o: Makefile xxhash.c $(HEADERS)
boards.o: Makefile boards.d $(HEADERS)
book.o: Makefile book.c $(HEADERS)
my_bstreams.o: Makefile my_bstreams.c $(HEADERS)
buckets.o: Makefile buckets.c $(HEADERS)
caches.o: Makefile caches.c $(HEADERS)
compat.o: Makefile compat.c $(HEADERS)
classes.o: Makefile classes.c $(HEADERS)
dbase.o: Makefile dbase.c $(HEADERS)
dxp.o: Makefile dxp.c $(HEADERS)
endgame.o: Makefile endgame.c $(HEADERS)
fbuffer.o: Makefile fbuffer.c $(HEADERS)
hub.o: Makefile hub.c $(HEADERS)
mcts.o: Makefile mcts.c mcts.d $(HEADERS)
moves.o: Makefile moves.c moves.d $(HEADERS)
my_cjson.o: Makefile my_cjson.c $(HEADERS)
my_mpi.o: Makefile my_mpi.c $(HEADERS)
my_malloc.o: Makefile my_malloc.c $(HEADERS)
my_printf.o: Makefile my_printf.c $(HEADERS)
my_random.o: Makefile my_random.c $(HEADERS)
networks.o: Makefile networks.c $(HEADERS)
patterns.o: Makefile patterns.c $(HEADERS)
profile.o: Makefile profile.c profile.h 
pdn.o: Makefile pdn.c $(HEADERS)
queues.o: Makefile queues.c $(HEADERS)
records.o: Makefile records.c $(HEADERS)
score.o: Makefile score.c score.d $(HEADERS)
search.o: Makefile search.c search.d $(HEADERS)
states.o: Makefile states.c $(HEADERS)
stats.o: Makefile stats.c $(HEADERS)
threads.o: Makefile threads.c $(HEADERS)
timers.o: Makefile timers.c $(HEADERS)
utils.o: Makefile utils.c $(HEADERS)

check:
	cppcheck --enable=all .

clean:
	rm *.o logs/*

