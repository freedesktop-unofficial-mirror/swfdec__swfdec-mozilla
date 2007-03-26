AC_PREREQ([2.58])
AC_INIT(swfdec-mozilla,0.4.3.1)

[nano=$(echo $PACKAGE_VERSION | sed 's/[0-9]\.[0-9]\.[0-9][0-9]*\.*//')]
if test x"$nano" = x1 ; then
  SWFDEC_CVS="yes"
else
  SWFDEC_CVS="no"
fi

AM_INIT_AUTOMAKE(1.6)
dnl AC_CANONICAL_TARGET([])
AM_MAINTAINER_MODE

SWFDEC_MAJORMINOR=0.4
AC_SUBST(SWFDEC_MAJORMINOR)

AM_CONFIG_HEADER(config.h)
AC_SUBST(ACLOCAL_AMFLAGS, "-I m4")

dnl decide on error flags
dnl if we support them, we set them unconditionally
AS_COMPILER_FLAG(-Wall, GLOBAL_CFLAGS="-Wall", GLOBAL_CFLAGS="")
dnl I want this but stupid headers don't let me
dnl AS_COMPILER_FLAG(-Wshadow, GLOBAL_CFLAGS="$GLOBAL_CFLAGS -Wshadow")
AS_COMPILER_FLAG(-Wextra -Wno-missing-field-initializers -Wno-unused-parameter, GLOBAL_CFLAGS="$GLOBAL_CFLAGS -Wextra -Wno-missing-field-initializers -Wno-unused-parameter")
dnl if we're in nano >= 1, add -Werror if supported
if test x$SWFDEC_CVS = xyes ; then
  AS_COMPILER_FLAG(-Werror, GLOBAL_CFLAGS="$GLOBAL_CFLAGS -Werror")
fi

SWFDEC_LIBVERSION="0:0:0"
AC_SUBST(SWFDEC_LIBVERSION)
AM_PROG_LIBTOOL

dnl Add parameters for aclocal
dnl (This must come after AM_INIT_AUTOMAKE, since it modifies ACLOCAL)
#ACLOCAL_AMFLAGS="-I m4 $ACLOCAL_AMFLAGS"

AC_PROG_CC
AM_PROG_CC_STDC

AC_HEADER_STDC([])

dnl ##############################
dnl # Do automated configuration #
dnl ##############################

dnl Check for tools:
dnl ================

dnl modify pkg-config path
AC_ARG_WITH(pkg-config-path, 
   AC_HELP_STRING([--with-pkg-config-path],[colon-separated list of pkg-config(1) dirs]),
   [export PKG_CONFIG_PATH=${withval}])

dnl Check for essential libraries first:
dnl ====================================

SWFDEC_REQUIRES=0.4.3.1
PKG_CHECK_MODULES(SWFDEC, swfdec-gtk-$SWFDEC_MAJORMINOR = $SWFDEC_REQUIRES, HAVE_SWFDEC=yes, HAVE_SWFDEC=no)
if test "$HAVE_SWFDEC" = "no"; then
  AC_MSG_ERROR([cannot find swfdec-$SWFDEC_MAJORMINOR $SWFDEC_REQUIRES, which is required for build])
fi
AC_SUBST(SWFDEC_LIBS)
AC_SUBST(SWFDEC_CFLAGS)

PKG_CHECK_MODULES(MOZILLA, mozilla-plugin, HAVE_MOZILLA=yes, HAVE_MOZILLA=no)
MOZILLA_DIR=mozilla
if test "$HAVE_MOZILLA" = no ; then
  PKG_CHECK_MODULES(MOZILLA, firefox-plugin, HAVE_MOZILLA=yes, HAVE_MOZILLA=no)
  MOZILLA_DIR=mozilla-firefox
fi
if test "$HAVE_MOZILLA" = no ; then
  PKG_CHECK_MODULES(MOZILLA, iceape-plugin, HAVE_MOZILLA=yes, HAVE_MOZILLA=no)
  MOZILLA_DIR=iceape
fi
AC_SUBST(MOZILLA_CFLAGS)
AC_SUBST(MOZILLA_LIBS)
AC_SUBST(MOZILLA_DIR)
AM_CONDITIONAL(HAVE_MOZILLA, [test $HAVE_MOZILLA = yes])
if test "x$HAVE_MOZILLA" = "xno"; then
  AC_MSG_ERROR([Mozilla not found. Mozilla/Netscape plugin will not be built.])
fi

dnl define standard mozilla defines
dnl this needs updates if we want to compile on Windows or OS X
AC_DEFINE_UNQUOTED(XP_UNIX, 1, [Define the Mozilla environment])
AC_DEFINE_UNQUOTED(MOZ_X11, 1, [Define the Mozilla windowing])

AC_SUBST(GLOBAL_CFLAGS)

if test "x${prefix}" = "xNONE"; then
  PACKAGE_PREFIX=${ac_default_prefix}
else
  PACKAGE_PREFIX=${prefix}
fi
AC_DEFINE_UNQUOTED(PACKAGE_PREFIX, "$PACKAGE_PREFIX", [Define the package prefix])
AC_SUBST(PACKAGE_PREFIX)

dnl #########################
dnl # Make the output files #
dnl #########################

AC_CONFIG_FILES(
Makefile
src/Makefile
)

AC_OUTPUT

