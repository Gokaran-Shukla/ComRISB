# ComRISB build of the Wannier90 3.1.0 library (libwannier.a): compiler and
# LAPACK from ComRISB's arch.mk.
#
# COMMS must be serial: ComWann calls wannier_setup/wannier_run on the master
# rank only.  The 3.x library (unlike 2.1) sets up its own MPI communicator
# (comms_setup_vars) and performs collectives over MPI_COMM_WORLD when built
# with COMMS=mpi, which deadlocks against ComWann's own mpi_bcast calls.
-include ../arch.mk
-include ../../arch.mk
-include ../../../arch.mk

F90    = $(PF90)
MPIF90 = $(PF90)
COMMS  = serial
FCOPTS = -O2 -fallow-argument-mismatch
LDOPTS =
LIBS   = $(LAPACK_LIB)
