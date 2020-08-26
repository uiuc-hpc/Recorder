AC_INIT([Recorder], 2.1.3)

AC_PROG_CC([mpicc])
AC_PROG_CXX([mpicxx])
AC_CONFIG_MACRO_DIRS([m4])


AC_ARG_ENABLE([posix], 
    AS_HELP_STRING([--disable-posix], [Disable POSIX tracing]))

AC_ARG_ENABLE([mpi],
     AS_HELP_STRING([--disable-mpi], [Disable MPI tracing]))

AC_ARG_ENABLE([hdf5],
      AS_HELP_STRING([--disable-hdf5], [Disable HDF5 tracing]))

AS_IF([test "x$enable_posix" == "xno"], [
  AC_SUBST(DISABLE_POSIX, "-DDISABLE_POSIX_TRACE")
])
AS_IF([test "x$enable_mpi" == "xno"], [
  AC_SUBST(DISABLE_MPI, "-DDISABLE_MPIO_TRACE")
])
AS_IF([test "x$enable_hdf5" == "xno"], [
  AC_SUBST(DISABLE_HDF5, "-DDISABLE_HDF5_TRACE")
])


AM_INIT_AUTOMAKE([foreign])
LT_INIT


AC_CONFIG_FILES([
    Makefile
    lib/Makefile
    tools/Makefile
])

AC_OUTPUT
