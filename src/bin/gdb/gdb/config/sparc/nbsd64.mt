# Target: NetBSD/sparc64
TDEPFILES= sparc64-tdep.o sparc64nbsd-tdep.o \
	sparc-tdep.o sparcnbsd-tdep.o nbsd-tdep.o \
	corelow.o solib.o solib-svr4.o
DEPRECATED_TM_FILE= solib.h
