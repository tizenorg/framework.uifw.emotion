
dnl use: EMOTION_CHECK_DEP_XINE(want_static[, ACTION-IF-FOUND[, ACTION-IF-NOT-FOUND]])

AC_DEFUN([EMOTION_CHECK_DEP_XINE],
[

requirement=""

PKG_CHECK_MODULES([XINE],
   [libxine >= 1.1.1 evas >= 1.0.0],
   [
    have_dep="yes"
    requirement="libxine"
   ],
   [have_dep="no"])

if test "x$1" = "xstatic" ; then
   requirement_emotion="${requirement} ${requirement_emotion}"
fi

AS_IF([test "x$have_dep" = "xyes"], [$2], [$3])

])

dnl use: EMOTION_CHECK_DEP_GSTREAMER(want_static[, ACTION-IF-FOUND[, ACTION-IF-NOT-FOUND]])

AC_DEFUN([EMOTION_CHECK_DEP_GSTREAMER],
[

GST_REQS=0.10.2
GSTPLUG_REQS=0.10.1
GST_MAJORMINOR=0.10

requirement=""
PKG_CHECK_MODULES([GSTREAMER],
   [gstreamer-$GST_MAJORMINOR >= $GST_REQS gstreamer-plugins-base-$GST_MAJORMINOR >= $GSTPLUG_REQS gstreamer-video-$GST_MAJORMINOR >= $GSTPLUG_REQS evas >= 1.0.0 eina >= 1.1.99],
   [
    have_dep="yes"
    requirement="gstreamer-$GST_MAJORMINOR gstreamer-plugins-base-$GST_MAJORMINOR"
   ],
   [have_dep="no"])

if test "x$1" = "xstatic" ; then
   requirement_emotion="${requirement} ${requirement_emotion}"
fi

AS_IF([test "x$have_dep" = "xyes"], [$2], [$3])

])

dnl use: EMOTION_CHECK_DEP_GENERIC_VLC(want_static[, ACTION-IF-FOUND[, ACTION-IF-NOT-FOUND]])

AC_DEFUN([EMOTION_CHECK_DEP_GENERIC_VLC],
[

requirement=""

PKG_CHECK_MODULES([GENERIC_VLC],
   [libvlc >= 0.9 eina >= 1.1.99],
   [
    have_dep="yes"
    requirement="libvlc"
   ],
   [have_dep="no"])

if test "x$1" = "xstatic" ; then
   requirement_emotion="${requirement} ${requirement_emotion}"
fi

AS_IF([test "x$have_dep" = "xyes"], [$2], [$3])

])

dnl use: EMOTION_CHECK_DEP_GENERIC(want_static[, ACTION-IF-FOUND[, ACTION-IF-NOT-FOUND]])

AC_DEFUN([EMOTION_CHECK_DEP_GENERIC],
[

requirement=""

PKG_CHECK_MODULES([GENERIC],
   [evas >= 1.0.0],
   [
    have_dep="yes"
   ],
   [have_dep="no"])

if test "x$1" = "xstatic" ; then
   requirement_emotion="${requirement} ${requirement_emotion}"
fi

AS_IF([test "x$have_dep" = "xyes"], [$2], [$3])

])

dnl use: EMOTION_CHECK_MODULE(description, want_module[, ACTION-IF-FOUND[, ACTION-IF-NOT-FOUND]])
AC_DEFUN([EMOTION_CHECK_MODULE],
[
m4_pushdef([UP], m4_translit([$1], [-a-z], [_A-Z]))dnl
m4_pushdef([DOWN], m4_translit([$1], [-A-Z], [_a-z]))dnl

want_module="$2"

AC_ARG_ENABLE([DOWN],
   [AC_HELP_STRING([--enable-]DOWN, [enable build of $1 module @<:@default=yes@:>@])],
   [
    if test "x${enableval}" = "xyes" ; then
       enable_module="yes"
    else
       if test "x${enableval}" = "xstatic" ; then
          enable_module="static"
       else
          enable_module="no"
       fi
    fi
   ],
   [enable_module="yes"])

if test "x${enable_module}" = "xyes" || test "x${enable_module}" = "xstatic" ; then
   want_module="yes"
fi

have_module="no"
if test "x${want_module}" = "xyes" && (test "x${enable_module}" = "xyes" || test "x${enable_module}" = "xstatic") ; then
   m4_default([EMOTION_CHECK_DEP_]m4_defn([UP]))(${enable_module}, [have_module="yes"], [have_module="no"])
fi

AC_MSG_CHECKING([whether to enable $1 module built])
AC_MSG_RESULT([${have_module}])

static_module="no"
if test "x${have_module}" = "xyes" && test "x${enable_module}" = "xstatic" ; then
   static_module="yes"
fi

AM_CONDITIONAL(EMOTION_BUILD_[]UP, [test "x${have_module}" = "xyes"])
AM_CONDITIONAL(EMOTION_STATIC_BUILD_[]UP, [test "x${static_module}" = "xyes"])

if test "x${static_module}" = "xyes" ; then
   AC_DEFINE(EMOTION_STATIC_BUILD_[]UP, 1, [Set to 1 if $1 is statically built])
   have_static_module="yes"
fi

enable_[]DOWN="no"
if test "x${have_module}" = "xyes" ; then
   enable_[]DOWN=${enable_module}
   AC_DEFINE(EMOTION_BUILD_[]UP, 1, [Set to 1 if $1 is built])
fi

AS_IF([test "x$have_module" = "xyes"], [$3], [$4])

m4_popdef([UP])
m4_popdef([DOWN])
])

dnl use: EMOTION_CHECK_GENERIC_PLAYER(description, want_module[, ACTION-IF-FOUND[, ACTION-IF-NOT-FOUND]])
AC_DEFUN([EMOTION_CHECK_GENERIC_PLAYER],
[
m4_pushdef([UP], m4_translit([$1], [-a-z], [_A-Z]))dnl
m4_pushdef([DOWN], m4_translit([$1], [-A-Z], [_a-z]))dnl

want_module="$2"

AC_ARG_ENABLE(generic-[]DOWN,
   [AC_HELP_STRING([--enable-generic-]DOWN, [enable build of Generic Player $1 @<:@default=yes@:>@])],
   [
    if test "x${enableval}" = "xyes" ; then
       enable_module="yes"
    else
       enable_module="no"
    fi
   ],
   [enable_module="auto"])

# shm_open
EFL_CHECK_SHM_OPEN([have_shm_open="yes"], [have_shm_open="no"])

if test "x${have_shm_open}" != "xyes"; then
  enable_generic="no"
fi

if test "x${enable_generic}" != "xyes" && test "x${enable_generic}" != "xstatic"; then
   if test "x${enable_module}" = "xyes"; then
      AC_MSG_WARN([Generic module is disabled, force disable of Generic Player $1])
   fi
   enable_module="no"
   want_module="no"
fi

if test "x${enable_module}" = "xauto"; then
   enable_module="${want_module}"
elif test "x${enable_module}" = "xyes"; then
   want_module="yes"
fi

have_module="no"
if test "x${want_module}" = "xyes" && test "x${enable_module}" = "xyes"; then
   m4_default([EMOTION_CHECK_DEP_GENERIC_]m4_defn([UP]))(${enable_module}, [have_module="yes"], [have_module="no"])
fi

AC_MSG_CHECKING([Whether to enable Generic Player $1])
AC_MSG_RESULT([${have_module}])

AM_CONDITIONAL(EMOTION_BUILD_GENERIC_[]UP, [test "x${have_module}" = "xyes"])

enable_generic_[]DOWN="no"
if test "x${have_module}" = "xyes" ; then
   enable_generic_[]DOWN=${enable_module}
   AC_DEFINE(EMOTION_BUILD_GENERIC_[]UP, 1, [Set to 1 if $1 is built])
fi

AS_IF([test "x$have_module" = "xyes"], [$3], [$4])

m4_popdef([UP])
m4_popdef([DOWN])
])
