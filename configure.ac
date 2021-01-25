AC_INIT([Recorder], 2.2.0)

AC_PROG_CC([mpicc cc])
AC_PROG_CXX([mpicxx CC])
AC_CONFIG_MACRO_DIRS([m4])


AC_ARG_ENABLE([posix], 
    AS_HELP_STRING([--disable-posix], [Disable POSIX tracing]))

AC_ARG_ENABLE([mpi],
    AS_HELP_STRING([--disable-mpi], [Disable MPI tracing]))

AC_ARG_ENABLE([hdf5],
    AS_HELP_STRING([--disable-hdf5], [Disable HDF5 tracing]))

AC_ARG_ENABLE([fcntl],
    AS_HELP_STRING([--disable-fcntl], [Disable FCNTL tracing]))


AS_IF([test "x$enable_posix" == "xno"], [
  AC_SUBST(DISABLE_POSIX, "-DDISABLE_POSIX_TRACE")
])
AS_IF([test "x$enable_mpi" == "xno"], [
  AC_SUBST(DISABLE_MPI, "-DDISABLE_MPIO_TRACE")
])
AS_IF([test "x$enable_hdf5" == "xno"], [
  AC_SUBST(DISABLE_HDF5, "-DDISABLE_HDF5_TRACE")
])
AS_IF([test "x$enable_fcntl" == "xno"], [
  AC_SUBST(DISABLE_FCNTL, "-DDISABLE_FCNTL_TRACE")
])

AM_INIT_AUTOMAKE([foreign])
LT_INIT


AC_CONFIG_FILES([
    Makefile
    lib/Makefile
    tools/Makefile
    tools/reporter/Makefile
])

AC_OUTPUT
