OBJS=main.o\
  boards.o\
  bstrlib.o\
  book.o\
  buckets.o\
  caches.o\
  classes.o\
  compat.o\
  dbase.o\
  dxp.o\
  endgame.o\
  hub.o\
  mcts.o\
  moves.o\
  my_malloc.o\
  my_mpi.o\
  my_printf.o\
  my_random.o\
  neural.o\
  profile.o\
  pdn.o\
  queues.o\
  records.o\
  score.o\
  search.o\
  states.o\
  threads.o\
  timers.o\
  utils.o\
  cJSON.o

ifndef ARCH
#ARCH=skylake
#ARCH=alderlake
#ARCH=nocona
ARCH=znver3
endif

ifdef OS
CC=clang
ifdef MAKE_OPENMPI
TARGET=a-$(ARCH)-mpi.exe
else
TARGET=a-$(ARCH).exe
endif
else
CC=clang
#CC=icx
TARGET=a.out
MAKE_VALGRIND=make_valgrind
MAKE_OPENMPI=make_openmpi
endif

ifdef MAKE_DEBUG
CFLAGS=-g -DDEBUG
else
CFLAGS=-O3 -march=$(ARCH)
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
COPTS+=-DUSE_AVX2_INTRINSICS -msse4.2 -mavx2 -mfma
#COPTS+=-msse4.2 -mavx2
endif
COPTS+=-Wall -pthread

CFLAGS+=$(COPTS)

LFLAGS=-L/usr/local/lib -lm -lzstd -lpthread -lsqlite3

ifdef MAKE_OPENMPI
LFLAGS+=-lmpi
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
  book.h\
  bstrlib.h\
  buckets.h\
  caches.h\
  cJSON.h\
  classes.h\
  compat.h\
  dbase.h\
  dxp.h\
  endgame.h\
  globals.h\
  hub.h\
  main.h\
  mcts.h\
  moves.h\
  my_malloc.h\
  my_printf.h\
  my_mpi.h\
  my_random.h\
  neural.h\
  pdn.h\
  profile.h\
  queues.h\
  records.h\
  score.h\
  search.h\
  states.h\
  threads.h\
  timers.h\
  utils.h

main.o: Makefile main.c $(HEADERS)

boards.o: Makefile boards.d $(HEADERS)
book.o: Makefile book.c $(HEADERS)
bstrlib.o: Makefile bstrlib.c $(HEADERS)
buckets.o: Makefile buckets.c $(HEADERS)
caches.o: Makefile caches.c $(HEADERS)
compat.o: Makefile compat.c $(HEADERS)
classes.o: Makefile classes.c $(HEADERS)
dbase.o: Makefile dbase.c $(HEADERS)
dxp.o: Makefile dxp.c $(HEADERS)
endgame.o: Makefile endgame.c $(HEADERS)
hub.o: Makefile hub.c $(HEADERS)
mcts.o: Makefile mcts.c mcts.d $(HEADERS)
moves.o: Makefile moves.c moves.d $(HEADERS)
my_mpi.o: Makefile my_mpi.c $(HEADERS)
my_printf.o: Makefile my_printf.c $(HEADERS)
my_malloc.o: Makefile my_malloc.c $(HEADERS)
my_random.o: Makefile my_random.c $(HEADERS)
neural.o: Makefile neural.c $(HEADERS)
profile.o: Makefile profile.c profile.h 
pdn.o: Makefile pdn.c pdn.d $(HEADERS)
queues.o: Makefile queues.c $(HEADERS)
records.o: Makefile records.c $(HEADERS)
score.o: Makefile score.c score.d $(HEADERS)
search.o: Makefile search.c search.d $(HEADERS)
states.o: Makefile states.c $(HEADERS)
threads.o: Makefile threads.c $(HEADERS)
timers.o: Makefile timers.c $(HEADERS)
utils.o: Makefile utils.c $(HEADERS)
cJSON.o: Makefile cJSON.c $(HEADERS)

check:
	cppcheck --enable=all .

clean:
	rm *.o logs/*

