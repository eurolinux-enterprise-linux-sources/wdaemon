# Initialize Autoconf
AC_PREREQ([2.60])
AC_INIT([wdaemon], [0.17])
AC_CONFIG_HEADERS([config.h])

# Initialize Automake
AM_INIT_AUTOMAKE([foreign dist-bzip2])
AM_MAINTAINER_MODE

AC_ARG_WITH([uinput-device],
    [AS_HELP_STRING([--with-uinput-device=<path>],
        [Set kernel uinput device (default: /dev/uinput)])],
    [UINPUT_DEVICE="$withval"],
    [UINPUT_DEVICE='/dev/uinput'])

AC_SUBST([UINPUT_DEVICE])

AC_ARG_WITH([devicename-prefix],
    [AS_HELP_STRING([--with-devicename-prefix=<prefix>],
        [Set prefix for devices created by wdaemon (default: 'wdaemon')])],
    [WDAEMON_DEVNAME_PREFIX="$withval"],
    [WDAEMON_DEVNAME_PREFIX='wdaemon'])
AC_SUBST([WDAEMON_DEVNAME_PREFIX])

PKG_CHECK_MODULES(EVEMU, [utouch-evemu >= 1.0.6])
AC_SUBST([PACKAGE_VERSION])

PKG_CHECK_MODULES(UDEV, udev)
UDEVDIR=`$PKG_CONFIG --variable=udevdir udev`
AC_SUBST([UDEVDIR])

# Initialize libtool
AC_PROG_LIBTOOL

# Checks for programs.
AC_PROG_CC
AC_PROG_INSTALL

# man page generation
PKG_CHECK_MODULES(XML, libxml-2.0, [HAVE_LIBXML=yes], [HAVE_LIBXML=no])
if test x$HAVE_LIBXML = "xno"; then
    AC_MSG_WARN([libxml headers not available. Cannot build evtest-capture.])
fi
AM_CONDITIONAL(HAVE_LIBXML, [test "x$HAVE_LIBXML" = "xyes"])

AC_PATH_PROG(XSLTPROC, [xsltproc])
AM_CONDITIONAL([HAVE_XSLTPROC], [test "x$XSLTPROC" != "x"])
if test "x$XSLTPROC" = "x"; then
        AC_MSG_WARN([xsltproc not found, needed to generate devices.])
fi

AC_ARG_VAR([XMLTO], [Path to xmlto command])
AC_PATH_PROG([XMLTO], [xmlto])
AC_ARG_VAR([ASCIIDOC], [Path to asciidoc command])
AC_PATH_PROG([ASCIIDOC], [asciidoc])
AM_CONDITIONAL([HAVE_DOCTOOLS], [test "x$XMLTO" != "x" && test "x$ASCIIDOC" != "x"])
if test "x$XMLTO" = "x" || test "x$ASCIIDOC" = "x"; then
        AC_MSG_WARN([xmlto or asciidoc not found - cannot create man pages without it])
fi


AC_CONFIG_FILES([Makefile
                 conf/Makefile
                 scripts/Makefile
                 scripts/wdaemon
                 udev/Makefile
                 udev/61-uinput-stddev.rules
                 udev/61-uinput-wacom.rules
                 wdaemon.txt])
AC_OUTPUT
