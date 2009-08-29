AC_PREREQ(2.57)
AC_INIT(elock, 0.1, lunohod@openinkpot.org)
AM_INIT_AUTOMAKE(foreign)

AC_PROG_CC

PKG_PROG_PKG_CONFIG

PKG_CHECK_MODULES(EFL, [ecore ecore-con ecore-evas evas edje])
PKG_CHECK_MODULES(LIBKEYS, [libkeys])

AC_OUTPUT([Makefile src/Makefile themes/Makefile data/Makefile po/Makefile])
