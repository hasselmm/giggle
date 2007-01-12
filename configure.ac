dnl Process this file with autoconf to produce a configure script.
AC_PREREQ(2.52)
 
AC_INIT(giggle, 0.1)
AC_CONFIG_SRCDIR(src/giggle-main.c)
AM_INIT_AUTOMAKE(AC_PACKAGE_NAME, AC_PACKAGE_VERSION)

AM_MAINTAINER_MODE
AM_CONFIG_HEADER(config.h)

AM_DISABLE_STATIC
AM_PROG_LIBTOOL

AC_ISC_POSIX
AC_PROG_CC
AM_PROG_CC_STDC
AC_HEADER_STDC

AM_PATH_GLIB_2_0

dnl Defined in acinclude.m4
IMENDIO_COMPILE_WARNINGS

dnl  ------------------
dnl | Pkgconfig checks |---------------------------------------
dnl  ------------------
PKG_CHECK_MODULES(GIGGLE, [
        glib-2.0 >= 2.12
	gthread-2.0
	gtk+-2.0 >= 2.10
])
dnl -----------------------------------------------------------


dnl  ------------------
dnl | Language Support |---------------------------------------
dnl  ------------------
#GETTEXT_PACKAGE=giggle
#AC_SUBST(GETTEXT_PACKAGE)
#AC_DEFINE_UNQUOTED(GETTEXT_PACKAGE,"$GETTEXT_PACKAGE",[Gettext package name])

#AM_GLIB_GNU_GETTEXT
dnl -----------------------------------------------------------


dnl This will cause the automake generated makefiles to pass the
dnl correct flags to aclocal.
ACLOCAL_AMFLAGS="\${ACLOCAL_FLAGS}"
AC_SUBST(ACLOCAL_AMFLAGS)

AC_OUTPUT([
	Makefile 
	src/Makefile
])

echo
echo "Prefix                 : $prefix"
echo
