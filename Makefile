.PHONY: cppcheck clang-tidy clang-format clean

COMPATDIR=/tmp2/gwies/compat
ENDGAME7DIR=/tmp2/gwies/endgame7

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

COMPATOBJS = compat.o

ENDGAME7OBJS = gwdegtb.o\
  wdl.o\
  egtb.o\
  progress.o\
  material.o\
  endgame_index.o

ARCH := $(shell uname -m)

ifneq ($(ARCH),arm64)
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
CC=clang
CFLAGS=-g -O0 -DDEBUG
#CFLAGS=-g 
else
CC=clang
CFLAGS=-g -O3
#CFLAGS=-g -O3 -fsanitize=shift
endif

CFLAGS+=-I$(COMPATDIR) -I$(ENDGAME7DIR)

LFLAGS=-lm -lzstd -lpthread 

ifdef MAKE_AVX2
CFLAGS+=-DUSE_AVX2_INTRINSICS
endif

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

CFLAGS+=-std=c11 -D_GNU_SOURCE

ifeq ($(ARCH), arm64)
CFLAGS+=-march=native
#CFLAGS+=-mcpu=neoverse-n1
#CFLAGS+=-march=armv8-a+simd+crc
else
CFLAGS+=-march=$(ARCH)
CFLAGS+=-DXXH_VECTOR=XXH_AVX2 
#CFLAGS+=-DXXH_VECTOR=XXH_AVX2 
endif

ifeq ($(CC), gcc)
CFLAGS+=-fomit-frame-pointer
CFLAGS+=-Wno-stringop-truncation
CFLAGS+=-Wno-format-truncation
#CFLAGS+=-Wl,--no-insert-timestamp
CFLAGS+=-Wpointer-arith
CFLAGS+=-Wunused-macros
CFLAGS+=-Wshadow
endif

ifeq ($(CC), clang)
CFLAGS+=-Wall -Wshadow
endif

xxhash.o: CFLAGS += -O3
#score.o: CFLAGS += -Wall -Wextra -Wconversion -Wfloat-conversion 
#search.o: CFLAGS += -Wall -Wextra -Wconversion -Wfloat-conversion 
all: $(TARGET)

$(TARGET): $(OBJS) $(COMPATOBJS) $(ENDGAME7OBJS)
	$(CC) -o $@ $(CFLAGS) $(OBJS) $(COMPATOBJS) $(ENDGAME7OBJS) $(LFLAGS)
	REVISION=$$(./$(TARGET) --revision); echo $$REVISION;\
        mkdir -p /tmp8/gwies/dxp1/$$REVISION;\
        cp $(TARGET) gwd.json overrides.json network0.json ballot2.fen /tmp8/gwies/dxp1/$$REVISION;\

HEADERS=boards.h\
  bstrlib.h\
  cJSON.h\
  xxhash.h\
  my_safec.h\
  book.h\
  buckets.h\
  caches.h\
  classes.h\
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

COMPATHEADERS = $(COMPATDIR)/compat.h

ENDGAME7HEADERS = $(ENDGAME7DIR)/gwdegtb.h\
  $(ENDGAME7DIR)/wdl.h\
  $(ENDGAME7DIR)/egtb.h\
  $(ENDGAME7DIR)/progress.h\
  $(ENDGAME7DIR)/material.h\
  $(ENDGAME7DIR)/endgame_index.h\
  $(ENDGAME7DIR)/movegen.h\
  $(ENDGAME7DIR)/crc32c.h\
  $(ENDGAME7DIR)/cache_modulo.h

$(OBJS): %.o: %.c Makefile $(HEADERS) $(COMPATHEADERS) $(ENDGAME7HEADERS)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

$(COMPATOBJS): %.o: $(COMPATDIR)/%.c Makefile $(COMPATHEADERS)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

$(ENDGAME7OBJS): %.o: $(ENDGAME7DIR)/%.c Makefile $(ENDGAME7HEADERS) $(COMPATHEADERS)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

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
	--force -D__cppcheck__ -i bstrlib.c -i cJSON.c -i xxhash.c *.c

clang-tidy:
	clang-tidy *.c -- -I./include

clang-format:
	clang-format -i *.c *.h

clean:
	rm *.o logs/*

.PHONY: test-movegen test-movegen-sanitize
test-movegen:
	bash tests/run-movegen.sh

test-movegen-sanitize:
	bash tests/run-movegen.sh sanitize

.PHONY: test-pv
test-pv:
	bash tests/run-pv.sh

.PHONY: test-dense-16x8 test-dense-16x8-sanitize
test-dense-16x8:
	bash tests/run-dense-16x8.sh

test-dense-16x8-sanitize:
	bash tests/run-dense-16x8.sh sanitize

.PHONY: test-compat test-compat-sanitize
test-compat:
	COMPAT_DIR="$(COMPATDIR)" CC="$(CC)" bash tests/run-compat.sh

test-compat-sanitize:
	COMPAT_DIR="$(COMPATDIR)" CC="$(CC)" bash tests/run-compat.sh sanitize

.PHONY: test-no-mpi test-no-mpi-sanitize
test-no-mpi:
	COMPAT_DIR="$(COMPATDIR)" CC="$(CC)" bash tests/run-no-mpi.sh

test-no-mpi-sanitize:
	COMPAT_DIR="$(COMPATDIR)" CC="$(CC)" bash tests/run-no-mpi.sh sanitize

.PHONY: test-thread-names test-thread-names-sanitize test-thread-names-tsan
test-thread-names:
	COMPAT_DIR="$(COMPATDIR)" CC="$(CC)" bash tests/run-thread-names.sh

test-thread-names-sanitize:
	COMPAT_DIR="$(COMPATDIR)" CC="$(CC)" bash tests/run-thread-names.sh sanitize

test-thread-names-tsan:
	COMPAT_DIR="$(COMPATDIR)" CC="$(CC)" bash tests/run-thread-names.sh tsan

.PHONY: test-atomic-caches test-atomic-caches-sanitize test-atomic-caches-tsan
test-atomic-caches:
	COMPAT_DIR="$(COMPATDIR)" CC="$(CC)" bash tests/run-atomic-caches.sh

test-atomic-caches-sanitize:
	COMPAT_DIR="$(COMPATDIR)" CC="$(CC)" bash tests/run-atomic-caches.sh sanitize

test-atomic-caches-tsan:
	COMPAT_DIR="$(COMPATDIR)" CC="$(CC)" bash tests/run-atomic-caches.sh tsan
