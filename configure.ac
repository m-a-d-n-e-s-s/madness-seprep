AC_PREREQ(2.61)
AC_INIT([madness], [0.9], [http://code.google.com/p/m-a-d-n-e-s-s])
AC_CONFIG_SRCDIR([configure.ac])

initial_CXXFLAGS="$CXXFLAGS"

# This is where autoconf automatically generated files go
AC_CONFIG_AUX_DIR([config])

# This is where local macros will be stored (by us)
AC_CONFIG_MACRO_DIR([config])

# Test for IBM/AIX ... must be before running C compiler (why?)
AC_AIX

# All source will be built with the MPI C++ and C compilers
ACX_MPI
AC_LANG_PUSH([C++])
ACX_MPI
AC_LANG_POP([C++])
CC="$MPICC"
CXX="$MPICXX"
LIBS="$MPILIBS $LIBS"

# If the user did not specify compiler flags attempt to select optimal ones here.
# Do this now so subsequent checks use/modify these flags.
if test x$initial_CXXFLAGS = x; then
  AC_OPTIMAL_CXXFLAGS
else
  echo "Using user specified C++ compiler flags '$CXXFLAGS'"
fi

# Default options for automake ??
AM_INIT_AUTOMAKE([-Wall -Werror nostdinc foreign])

# We want #defines output into a header file
AC_CONFIG_HEADERS([include/madness_config.h])

# Check for basic build functionality
AM_SANITY_CHECK
# Don't need to check again for C and C++, and the PROG_CC
# will destroy our setting of CC=MPICC
#AC_PROG_CC
#AC_PROG_CXX
AC_PROG_RANLIB
AC_PROG_INSTALL
AC_PROG_LN_S
AC_PROG_SED
AC_PROG_MAKE_SET
AM_PROG_AS

# System headers
AC_HEADER_TIME
AC_HEADER_STDC
AC_CHECK_HEADERS([limits.h stddef.h stdint.h stdlib.h string.h sys/param.h sys/time.h unistd.h])
AC_HEADER_STDBOOL
ACX_GNU_HASHMAP

# Types
AC_TYPE_PID_T
AC_C_RESTRICT
AC_TYPE_SIZE_T
AC_HEADER_TIME
AC_TYPE_UINT8_T
AC_TYPE_UINT16_T
AC_TYPE_UINT32_T
AC_TYPE_UINT64_T
AC_TYPE_INT64_T
AC_C_VOLATILE
AC_C_CONST
AC_C_INLINE
AC_CHECK_TYPES([ptrdiff_t])

# Library functions
AC_CHECK_FUNCS([sleep random execv perror gettimeofday memmove memset pow sqrt strchr strdup])
AC_FUNC_FORK
AC_FUNC_MALLOC
AC_FUNC_ERROR_AT_LINE
ACX_POSIX_MEMALIGN

# Checks for Fortran linking conventions and BLAS+LAPACK at the same time
ACX_FORTRAN_SYMBOLS

# Compiler quirks
ACX_UNQUALIFIED_STATIC_DECL
ACX_STD_ABS

# This sets host, host_cpu, host_vendor, host_os
AC_CANONICAL_HOST
echo "HOST INFORMATION:   $host    $host_cpu    $host_vendor    $host_os"
AC_DEFINE_UNQUOTED([HOST_CPU], ["$host_cpu"], [Defines the host cpu (x86, x86_64, ...).])
AC_DEFINE_UNQUOTED([HOST_SYSTEM], ["$host_os"], [Defines the host os (linux-gnu, ...).])

# Pick out CPUs for which we have special source files (assembler)
use_x86_64_asm=no
use_x86_32_asm=no
case $host_cpu in
     x86_64)   
         AC_DEFINE([X86_64], [1], [If set indicates we are using a 64-bit Intel/AMD cpu.]) 
         use_x86_64_asm=yes
         ;;
     i?86*) 
         AC_DEFINE([X86_32], [1], [If set indicates we are using a 32-bit Intel/AMD cpu.]) 
         use_x86_32_asm=yes
         ;;
esac

# Define AM conditional for use of aforementioned assembler
AM_CONDITIONAL([USE_X86_64_ASM], [test $use_x86_64_asm = yes])
AM_CONDITIONAL([USE_X86_32_ASM], [test $use_x86_32_asm = yes])

# Check for xterm
AC_CHECK_PROG(XTERM,xterm,xterm,no)
if test $XTERM = 'xterm'; then
  AC_DEFINE(HAVE_XTERM,[1],[If set indicates xterm is in path])
fi

# Check for gdb
AC_CHECK_PROG(GDB,gdb,gdb,no)
if test $GDB = 'gdb'; then
  AC_DEFINE(HAVE_GDB,[1],[If set indicates gdb is in path])
fi

# How to handle MADNESS exceptions.
AC_ARG_ENABLE(madex,
              [  --enable-madex=arg   Specifies behavior of MADNESS assertions: throw (default), abort, assert, disable],
              [madex=$enableval],
              [madex=throw])
case $madex in
  abort)
    echo MADNESS assertions will abort
    AC_DEFINE([MADNESS_ASSERTIONS_ABORT], [1], [Set if MADNESS assertions abort])
  ;;
  assert)
    echo MADNESS assertions will assert
    AC_DEFINE([MADNESS_ASSERTIONS_ASSERT], [1], [Set if MADNESS assertions assert])
  ;;
  disable)
    echo MADNESS assertions are disabled
    AC_DEFINE([MADNESS_ASSERTIONS_DISABLE], [1], [Set if MADNESS assertions disabled])
  ;;
  *)
    echo MADNESS assertions will throw
    AC_DEFINE([MADNESS_ASSERTIONS_THROW], [1], [Set if MADNESS assertions throw])
  ;;
esac

# Determine if we should use the instrumented new/delete
AC_ARG_ENABLE(memstats,
              [  --enable-memstats  If yes, gather memory statistics (expensive)],
              [memstats=$enableval],
              [memstats=no])
if test "$memstats" = "yes"; then
  echo 'MADNESS will gather memory statistics (expensive)'
  AC_DEFINE([WORLD_GATHER_MEM_STATS], [1], [Set if MADNESS gathers memory statistics])
fi

# Capture configuration info for user by compiled code
AC_DEFINE_UNQUOTED([MADNESS_CONFIGURATION_USER],["$USER"],[User that configured the code])
AC_DEFINE_UNQUOTED([MADNESS_CONFIGURATION_DATE],["`date`"],[Date of configuration])
AC_DEFINE_UNQUOTED([MADNESS_CONFIGURATION_CXX], ["$CXX"],[Configured C++ compiler])
AC_DEFINE_UNQUOTED([MADNESS_CONFIGURATION_CXXFLAGS], ["$CXXFLAGS"],[Configured C++ compiler flags])
AC_DEFINE_UNQUOTED([MADNESS_CONFIGURATION_HOST], ["$HOSTNAME"],[Configured on this machine])

# Not yet enabled the user to configure this stuff but probably should
AC_DEFINE([WORLD_WATCHDOG],[1],[Enables timeout for dead lock detection])
AC_DEFINE([WATCHDOG_BARK_INTERVAL],[300],[Interval for warning about possible deadlock])
AC_DEFINE([WATCHDOG_TIMEOUT],[1200],[Abort if idle for this long])
AC_DEFINE([TENSOR_INSTANCE_COUNT],[1],[Enable counting of allocated tensors ... useful for memory leak detection])
#AC_DEFINE([BOUNDS_CHECKING],[1],[Enable checking of bounds in tensors ... slow but useful for debugging])
AC_DEFINE(MADNESS_FORINT,[long],[Equivalent C type for Fortran integers ... should always be long?])


# Stuff that either does not work or is ill-advised
######AC_DEFINE([WORLD_TAU_TRACE],[1],[Enable tracing using tau])
#####AC_DEFINE([WORLD_SCHED_BACKOFF],[1],[Yield during busy spin])


# These are the files that must be generated/preprocessed ... as you add files
# please maintain the directory tree file order and indenting.
AC_CONFIG_FILES([
 Makefile
    src/Makefile
    src/lib/Makefile
       src/lib/world/Makefile
       src/lib/misc/Makefile
       src/lib/tinyxml/Makefile
       src/lib/tensor/Makefile
       src/lib/linalg/Makefile
       src/lib/mra/Makefile
    src/apps/Makefile
       src/apps/examples/Makefile
       src/apps/tdse/Makefile
       src/apps/moldft/Makefile
           src/apps/moldft/xc/Makefile
       src/apps/hf/Makefile
])

AC_OUTPUT