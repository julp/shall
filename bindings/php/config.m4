PHP_ARG_WITH(shall, for Shall support,
[  --with-shall=DIR       Include Shall support], no)

if test "$PHP_SHALL" != "no"; then
  PHP_NEW_EXTENSION(shall, php_shall.c, $ext_shared)
  PHP_SUBST(SHALL_SHARED_LIBADD)
  if test "$PHP_SHALL" != "yes" -a "$PHP_SHALL" != "no"; then
    if test -f $PHP_SHALL/include/shall/shall.h; then
      SHALL_DIR=$PHP_SHALL
      SHALL_INCDIR=$SHALL_DIR/include
    #elif test -f $PHP_SHALL/include/shall.h; then
      #SHALL_DIR=$PHP_SHALL
      #SHALL_INCDIR=$SHALL_DIR/include
    fi
  else 
    for i in /usr/local /usr $PHP_SHALL_DIR; do
      if test -f $i/include/shall/shall.h; then
        SHALL_DIR=$i
        SHALL_INCDIR=$i/include
      #elif test -f $i/include/shall.h; then
        #SHALL_DIR=$i
        #SHALL_INCDIR=$i/include
      fi
    done
  fi
  if test -z "$SHALL_DIR"; then
    AC_MSG_ERROR(Cannot find shall)
  fi
  #AC_DEFINE(HAVE_PHP_SHALL,1,[ ])
  PHP_ADD_LIBPATH($SHALL_DIR/$PHP_LIBDIR, SHALL_SHARED_LIBADD)
  PHP_SHALL_DIR=$SHALL_DIR
  PHP_ADD_LIBRARY(shall,, SHALL_SHARED_LIBADD)
  PHP_ADD_INCLUDE($SHALL_INCDIR)
fi
