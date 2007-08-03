AC_PREREQ([2.58])
AC_INIT(swfdec-mozilla,0.5.1)

[nano=$(echo $PACKAGE_VERSION | sed 's/[0-9]\.[0-9]\.[0-9][0-9]*\.*//')]
if test x"$nano" = x1 ; then
  SWFDEC_CVS="yes"
else
  SWFDEC_CVS="no"
fi

AM_INIT_AUTOMAKE(1.6)
dnl AC_CANONICAL_TARGET([])
AM_MAINTAINER_MODE

SWFDEC_MAJORMINOR=0.5
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

SWFDEC_REQUIRES=0.5.1
PKG_CHECK_MODULES(SWFDEC, swfdec-gtk-$SWFDEC_MAJORMINOR = $SWFDEC_REQUIRES, HAVE_SWFDEC=yes, HAVE_SWFDEC=no)
if test "$HAVE_SWFDEC" = "no"; then
  AC_MSG_ERROR([cannot find swfdec-$SWFDEC_MAJORMINOR $SWFDEC_REQUIRES, which is required for build])
fi
AC_SUBST(SWFDEC_LIBS)
AC_SUBST(SWFDEC_CFLAGS)

dnl setup Mozilla install directory
PLUGIN_DIR=${libdir}/mozilla/plugins
AC_ARG_WITH(plugin-dir, 
   AC_HELP_STRING([--with-plugin-dir],[install directory for plugin]),
   [PLUGIN_DIR=${withval}])
AC_SUBST(PLUGIN_DIR)

MOZILLA_CFLAGS="-I\$(top_srcdir)/mozilla-sucks"
AC_SUBST(MOZILLA_CFLAGS)

dnl define standard mozilla defines
dnl this needs updates if we want to compile on Windows or OS X
AC_DEFINE_UNQUOTED(XP_UNIX, 1, [Define the Mozilla environment])
AC_DEFINE_UNQUOTED(MOZ_X11, 1, [Define the Mozilla windowing])

AC_SUBST(GLOBAL_CFLAGS)

dnl #########################
dnl # Make the output files #
dnl #########################

AC_CONFIG_FILES(
Makefile
mozilla-sucks/Makefile
src/Makefile
)

AC_OUTPUT

