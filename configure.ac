AC_PREREQ(2.59)
AC_INIT([eagle-eye], [0.0.1],
        [http://www.livemocha.com],
        [eagle-eye])

AM_INIT_AUTOMAKE(no-define)

# Checks for programs.
AC_PROG_CC
AC_PROG_INSTALL
AC_PROG_LN_S
AC_PROG_MAKE_SET

# Checks for libraries.
PKG_CHECK_MODULES(webkit, [webkit-1.0],,
                  [AC_MSG_FAILURE([$webkit_PKG_ERRORS])])
PKG_CHECK_MODULES(libsoup, [libsoup-2.4],,
                  [AC_MSG_FAILURE([$libsoup_PKG_ERRORS])])
PKG_CHECK_MODULES(gtk, [gtk+-2.0],,
                  [AC_MSG_FAILURE([$gtk_PKG_ERRORS])])
PKG_CHECK_MODULES(glib, [glib-2.0 gthread-2.0 gmodule-2.0],,
                  [AC_MSG_FAILURE([$gthread_PKG_ERRORS])])

AC_CONFIG_FILES([Makefile src/Makefile eagle-eye.desktop])
AC_OUTPUT
