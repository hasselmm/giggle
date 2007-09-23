dnl Turn on the additional warnings last, so -Werror doesn't affect other tests.                                                                                
AC_DEFUN([IMENDIO_COMPILE_WARNINGS],[
   if test -f $srcdir/autogen.sh; then
        default_compile_warnings="error"
    else
        default_compile_warnings="no"
    fi
                                                                                
    AC_ARG_WITH(compile-warnings, [  --with-compile-warnings=[no/yes/error] Compiler warnings ], [enable_compile_warnings="$withval"], [enable_compile_warnings="$default_compile_warnings"])
                                                                                
    warnCFLAGS=
    if test "x$GCC" != xyes; then
        enable_compile_warnings=no
    fi
                                                                                
    warning_flags=
    realsave_CFLAGS="$CFLAGS"
                                                                                
    case "$enable_compile_warnings" in
    no)
        warning_flags=
        ;;
    yes)
        warning_flags="-Wall -Wunused -Wmissing-prototypes -Wmissing-declarations"
        ;;
    maximum|error)
        warning_flags="-Wall -Wunused -Wchar-subscripts -Wmissing-declarations -Wmissing-prototypes -Wnested-externs -Wpointer-arith -Wcast-align -std=c99"
        CFLAGS="$warning_flags $CFLAGS"
        for option in -Wno-sign-compare -Wno-pointer-sign; do
                SAVE_CFLAGS="$CFLAGS"
                CFLAGS="$CFLAGS $option"
                AC_MSG_CHECKING([whether gcc understands $option])
                AC_TRY_COMPILE([], [],
                        has_option=yes,
                        has_option=no,)
                CFLAGS="$SAVE_CFLAGS"
                AC_MSG_RESULT($has_option)
                if test $has_option = yes; then
                  warning_flags="$warning_flags $option"
                fi
                unset has_option
                unset SAVE_CFLAGS
        done
        unset option
        if test "$enable_compile_warnings" = "error" ; then
            warning_flags="$warning_flags -Werror"
        fi
        ;;
    *)
        AC_MSG_ERROR(Unknown argument '$enable_compile_warnings' to --enable-compile-warnings)
        ;;
    esac
    CFLAGS="$realsave_CFLAGS"
    AC_MSG_CHECKING(what warning flags to pass to the C compiler)
    AC_MSG_RESULT($warning_flags)
                                                                                
    WARN_CFLAGS="$warning_flags"
    AC_SUBST(WARN_CFLAGS)
])

AC_DEFUN([IGE_PLATFORM_CHECK],[
    gdk_target=`$PKG_CONFIG --variable=target gtk+-2.0`

    if test "x$gdk_target" = "xquartz"; then
        AC_MSG_CHECKING([checking for Mac OS X support])
        carbon_ok=no
        AC_TRY_CPP([
        #include <Carbon/Carbon.h>
        #include <CoreServices/CoreServices.h>
        ], carbon_ok=yes)
        AC_MSG_RESULT($carbon_ok)
        if test $carbon_ok = yes; then
          IGE_PLATFORM=osx
          IGE_PLATFORM_NAME="GTK+ OS X"
          AC_DEFINE(HAVE_PLATFORM_OSX, 1, [whether GTK+ OS X is available])
        fi
    elif test "x$gdk_target" = "xx11"; then
        IGE_PLATFORM=x11
        IGE_PLATFORM_NAME="GTK+ X11"
        AC_DEFINE(HAVE_PLATFORM_X11, 1, [whether GTK+ X11 is available])
    else
        AC_MSG_ERROR([Could not detect the platform])
    fi

    AM_CONDITIONAL(HAVE_PLATFORM_OSX, test $IGE_PLATFORM = osx)
    AM_CONDITIONAL(HAVE_PLATFORM_X11, test $IGE_PLATFORM = x11)
])
