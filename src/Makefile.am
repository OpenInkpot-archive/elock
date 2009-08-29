INCLUDES = @EFL_CFLAGS@ @LIBKEYS_CFLAGS@
AM_CFLAGS=--std=c99 -Wall -DDATADIR='"$(datadir)"'

bin_PROGRAMS = elock

elock_SOURCES = elock.c
elock_LDADD = @EFL_LIBS@ @LIBKEYS_LIBS@
