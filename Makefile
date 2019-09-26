CXX = g++
CFLAGS = -DNDEBUG -DMY_SEMAPHORES -flto -I myopt -I sysport
CFLAGSD = -g -DMY_SEMAPHORES -I myopt -I sysport
WARN = -Wwrite-strings -Wconversion -Wshadow -Wparentheses -Wlogical-op -Wunused -Wmissing-prototypes -Wmissing-declarations -Wdeclaration-after-statement -W -Wall -Wextra
OPT = -O3
LIBFLAGS = -lm -lpthread

EXE = ordo

SRC = myopt/myopt.cc sysport/sysport.cc mystr.cc proginfo.cc pgnget.cc randfast.cc gauss.cc groups.cc cegt.cc indiv.cc encount.cc ratingb.cc rating.cc xpect.cc csv.cc fit1d.cc mymem.cc relprior.cc report.cc relpman.cc plyrs.cc namehash.cc inidone.cc rtngcalc.cc ra.cc sim.cc summations.cc bitarray.cc strlist.cc justify.cc myhelp.cc mytimer.cc main.cc
DEPS = myopt/myopt.h sysport/sysport.h boolean.h  datatype.h  gauss.h  groups.h  mystr.h  mytypes.h  ordolim.h  pgnget.h  proginfo.h  progname.h  randfast.h  version.h cegt.h indiv.h encount.h xpect.h csv.h ratingb.h fit1d.h rating.h report.h relprior.h relpman.h mymem.h namehash.h inidone.h rtngcalc.h ra.h sim.h summations.h bitarray.h strlist.h plyrs.h justify.h mytimer.h myhelp.h
OBJ = myopt/myopt.o sysport/sysport.o mystr.o proginfo.o pgnget.o randfast.o gauss.o groups.o cegt.o indiv.o encount.o ratingb.o rating.o xpect.o csv.o fit1d.o mymem.o report.o relprior.o relpman.o plyrs.o namehash.o inidone.o rtngcalc.o ra.o sim.o summations.o bitarray.o strlist.o justify.o myhelp.o mytimer.o main.o 

%.o: %.c $(DEPS)
	$(CXX) -c -o $@ $< $(CFLAGS)

ordo: $(OBJ)
	$(CXX) -o $@ $^ $(CFLAGS) $(WARN) $(OPT) $(LIBFLAGS)

all:
	$(CXX) $(CFLAGS) $(WARN) $(OPT) -o $(EXE) $(SRC) $(LIBFLAGS)

debug:
	$(CXX) $(CFLAGSD) $(WARN) $(OPT) -o $(EXE) $(SRC) $(LIBFLAGS)

install:
	cp $(EXE) /usr/local/bin/$(EXE)

clean:
	rm -f *.o *~ myopt/*.o ordo-v*.tar.gz ordo-v*-win.zip *.out









