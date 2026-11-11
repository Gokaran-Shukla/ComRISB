# ComRISB modernization (October 2026)

Branch `modernize` of github.com/comscope/ComRISB, based on commit 687c8f5 (the
2021 Comscope VM version). The goal is to run the published code unchanged in
physics on a current software stack.

| | published (2021) | this update |
|---|---|---|
| Python | 3.7 / 3.8 era | **3.9 – 3.13** (tested on 3.13) |
| NumPy | ≤ 1.23 | **1.24 – 2.x** (tested 2.5) |
| SciPy / h5py / mpi4py | 1.5 / 2.10 / 3.0 | **1.18 / 3.16 / 4.1** |
| pymatgen | 2020–21 (monolithic) | **pymatgen-core 2026** |
| Fortran | gfortran ≤ 9 / ifort | **gfortran 13** (also gfortran ≥ 10) |
| MPI / HDF5 | — | OpenMPI 4.1, HDF5 1.10 (parallel) |
| Wannier90 | 2.1 (bundled) | **3.1.0 (bundled)** |
| Packaging | `setup.py develop` | **PEP 517/621 `pyproject.toml`** |

## Build

```bash
# Ubuntu 24.04 (or equivalent modules on a cluster)
sudo apt install gfortran libopenmpi-dev openmpi-bin libhdf5-openmpi-dev \
                 libopenblas-dev liblapack-dev
export COMRISB_BIN=$PWD/bin
make            # wannier90_3.1.0 (lib), gw, ComWann, cygutz, pygrisb
# or step by step:
(cd wannier90_3.1.0 && make lib); (cd gw && make); (cd ComWann && make)
(cd cygutz && make install); pip install -e pygrisb/pygrisb
```

`arch.mk -> arch.mk.gnu` (gfortran). For Intel compilers use `arch.mk.intel`
(the new flag is gfortran-only).

## Changes

### Python (pygrisb, ComBin, cygutz/pygtool)
1. **NumPy ≥ 1.24 / 2.x.** 105 uses of the removed aliases `np.complex`,
   `np.float`, `np.int` (and `numpy.…`) were replaced by the builtins they were
   aliases of; `np.complex_` was replaced by `np.complex128` (42 files). Results are unchanged.
2. **h5py ≥ 3.** In h5py 3, `h5py.File(name)` without a mode opens read-only (it was
   append before). The two calls that need it now pass an explicit mode
   (`mbody/h5coulomb_matrix.py` writes → `'r+'`; `estructure/bandstruct.py` → `'r'`).
3. **Current pymatgen.** Submodules are no longer imported implicitly:
   `ComBin/cif2matdelab/structure.py` and `gutz/atoms.py` now import
   `pymatgen.symmetry.analyzer` / `.bandstructure` explicitly. Without this,
   `cif2matdelab.py` (DFT input generation) failed.
4. **Python 2 leftovers.** `from future_builtins import zip` was removed (3 scripts), and
   print statements in `cygutz/pygtool/{misc/pychk_rhi.py, misc/plot_bands_1.py,
   gw_gutz/chk_dos.py}` were converted. Every `.py` file in the repository now compiles under Python 3.13.
5. **Import-time environment lookup.** `run/gwien.py` evaluated
   `os.environ['WIEN_GUTZ_ROOT']` as a default argument, so importing
   `pygrisb.run.gwien`, `pygrisb.dft.wien` or `pygrisb.dft.comrisb` raised
   `KeyError` on any machine without Wien2k. The lookup now happens at call time.
6. **Packaging.** Added `pyproject.toml` (setuptools ≥ 64, PEP 660 editable
   installs); `setup.py` is now a shim. Two undeclared dependencies are now declared: `spglib` and
   `tabulate`. Optional extras: `pygrisb[quantum]` (qiskit, openfermion) and
   `pygrisb[phonopy]`. Also fixed: the `MANIFEST.in` typo (`ecursive-include`), and the
   `pip install --user` in `pygrisb/Makefile`.

### Bug fixes (present in the published version)
7. **Nonlinear solver: finite-difference Jacobian** (`run/cygutz.py`,
   `gsolve_nleqns`). MINPACK's internal difference step is purely relative,
   h = √eps·|x|. Starting an adiabatic U scan from a converged U = 0
   solution leaves symmetry-zero parameters at ~1e-17, so their
   Jacobian columns are noise and hybr jumps to an unphysical point. With
   `iembeddiag = -3` (ED with S = 0) this produced NaN for every U > 0 in the
   published model scans (`model/test/scan_semicirc.py`). That was reproduced with the
   original code in a 2021 environment (Python 3.8, NumPy 1.19.5, SciPy 1.5.4).
   Now, when no analytic Jacobian is requested, an explicit forward-difference
   Jacobian with h_j = √eps·max(|x_j|, 1) is used. This is identical to the
   previous step for |x_j| ≥ 1 and keeps the same `eps` option.
8. **`cygutz/src/cygutz/GMPI.f90`, `dsum_master_mpi`.** The routine called
   `mpi_reduce(this, a, b, n, …)` with a spurious leading `this`, which shifts every
   MPI argument by one. The routine is not called anywhere in the current code,
   so published results are unaffected, but gfortran ≥ 10 refuses to compile it.

9a. **`gw/lda_dmft_charge_density.F`, `initialize_wan_density_matrix`: uninitialized
   HDF5 offsets (critical for parallel runs).** The hyperslab offset array `nv`
   passed to every `fread` of `wannier_den_matrix.dat` was never set. Built with
   gfortran 13, every MPI rank except the master failed to read `vw_matrix_new`,
   `n_matrix_new` and `include_band_new` (`ERROR: fread_zn: h5dread_f failed`).
   Those ranks kept zero matrices, so the G-RISB density correction was
   silently dropped on their k-points. One DFT step of PM Fe (U = 5, J = 0.8)
   gave Δρ = 1.897e-3 (np = 1, equal to the tutorial), 4.57e-4 (np = 2),
   1.69e-4 (np = 3) and 9.0e-5 (np = 4). After setting `nv = 0`, all rank counts
   give 1.897089e-3 and E = −2541.07106914 with no read errors. A U = 0 run
   cannot detect this, because the correction vanishes. An audit of all 23
   routines that use the HDF5 layer (gw + ComWann) found no other unset offsets.
9b. **`pygrisb/iface/wanniertb.py`, `k_path`: array compared with a string.**
   `if kpts == 'full'` received the k-point array from `plot_band_tf.py`. Old
   NumPy returned `False` with a FutureWarning; NumPy ≥ 2 compares elementwise
   and raises `ValueError: truth value of an array ... is ambiguous`. As a result, every
   G-RISB band-structure plot failed. The string cases are now tested only when
   `kpts` is a `str`. The PM and FM Fe band plots reproduce the tutorial figures.

### Fortran build
9. `arch.mk.gnu`: added `-fallow-argument-mismatch`. gfortran ≥ 10 turns the
   legacy implicit-interface MPI/BLAS calls (COMPLEX buffers passed where REAL is
   expected, etc.) into errors. Without it, cygutz, ComWann and gw do not build.

### Wannier90 2.1 → 3.1.0
10. `wannier90_3.1.0/` is the official v3.1.0 source, with a `make.inc` that reproduces
    the previous configuration (ComRISB compiler from `arch.mk`, `COMMS=mpi`).
    The library-mode interface used by ComWann (`wannier_setup`,
    `wannier_run`) has the same argument list and declarations in 2.1 and 3.1.0,
    so no interface code changed.
    **The library must be built with `COMMS=serial`.** ComWann calls the library on
    the master rank only. Wannier90 3.x (unlike 2.1) initializes its own MPI
    communicator in library mode and performs collectives over MPI_COMM_WORLD,
    so a `COMMS=mpi` build deadlocks ComWann (observed in testing). `GNUmakefile` and `ComWann/GNUmakefile` now
    point to the new directory. The gw/FlapwMBPT code does not link Wannier90.
    `wannier90_2.1/` is kept for reference and is no longer built.
    (Wannier90 4.x has a redesigned library API and would need a rewrite of
    `ComWann/wannier_interface.F`.)

## Known issues not changed (pre-existing in the published repository)
Seven modules import functions or modules that do not exist anywhere in the
repository, so they cannot be imported in either the old or the new environment:
`gutz/batch_init.py`, `gutz/embedh.py`, `gutz/trans_gparam.py`,
`mbody/generate_j2_basis.py`, `mbody/hs_ired_representation.py`,
`mbody/hs_rotation_representation.py`, `model/circauxi.py`. The shipped
`model/test/test_*_model/REF/test_tb.py` scripts also use the missing
`pygrisb.gutz.ginput` API. The current model drivers (`pygrisb.model.*` with
`run.cygutz.driver_cygutz`) are unaffected.

## Validation

Toolchain: Ubuntu 24.04, Python 3.13, NumPy 2.5.3, SciPy 1.18.1, h5py 3.16,
mpi4py 4.1.2, pymatgen-core 2026.10, gfortran 13.3, OpenMPI 4.1.6, HDF5 1.10.10,
OpenBLAS. Every Fortran component builds with 0 errors. All `.py` files compile;
the import sweep of `pygrisb` gives 85 modules OK. The remaining failures are the 7 pre-existing broken modules
listed above, 4 command-line scripts that exit on argument parsing, and the
optional `openfermion` solver.

The reference runs of the **original** code used a 2021 environment: Python 3.8.20,
NumPy 1.19.5, SciPy 1.5.4, h5py 2.10, mpi4py 3.0.3 and pymatgen 2020.12.31 (built from source).

**1. End-to-end DFT + Wannier90 + GRISB (`test/fe`, bcc Fe, U = J = 0).**
This test runs FlapwMBPT DFT, then ComWann with Wannier90 3.1.0, then CyGutz. It is compared with the reference
`GLog.h5` shipped by the authors:

| | reference | this update |
|---|---|---|
| test criterion (`compare_array`, chk_sum) | — | **pass** |
| Tr NC_PHY | 6.459975 | 6.459975 |
| eigenvalues (eg ×4, t2g ×6) | 0.5996, 0.6769 | 0.5996, 0.6769 |
| max eigenvalue difference | | 4.6e-9 |

**2. One-band semicircular model (`model/semicir`) vs the exact Brinkman–Rice
solution** (D = 1, half filling: Z = 1 − (U/U_c)², U_c = 32/(3π)). The scan is
adiabatic, U = 0 → 1 → 2 → 3, as in `model/test/scan_semicirc.py`:

| U | Z (exact) | Z, ED modes −1, −2, −3 | d (exact) | d |
|---|---|---|---|---|
| 1 | 0.913255 | 0.913256 | 0.176369 | 0.176369 |
| 2 | 0.653022 | 0.653023 | 0.102738 | 0.102738 |
| 3 | 0.219299 | 0.219282 | 0.029107 | 0.029104 |

(The residual at U = 3, close to U_c, comes from the 5000-point energy mesh.) Before
fix 7, mode −3 gave NaN for every U > 0, with both the original and the ported
code.

**3. Three-orbital Hubbard–Kanamori model (`model/mbandhubbardsemicir`, n = 2,
canonical ensemble), original code in the 2021 environment vs this update:**

| U, J | E (original) | E (updated) | Z (original / updated) |
|---|---|---|---|
| 2.0, 0.4 | −1.6049345467 | −1.6049345467 | 0.718236 / 0.718236 |
| 4.0, 0.8 | −2.5391528418 | −2.5391528418 | 0.414814 / 0.414814 |

Energies, Z and the local density matrix agree to all printed digits.

**4. Tutorials (github.com/comscope/comrisb_tutorial), re-run in full.**
The Bethe lattice, checkerboard lattice and Fe tutorials were re-run (DFT-LDA; DFT+G-RISB PM and FM
at U = 5, J = 0.8 eV with 2 MPI ranks). They reproduce the documented results: converged
PM Fe E = −2540.914219 Ry (doc −2540.914208), Z(t2g, eg) = 0.8799, 0.8207; FM
Fe E = −2540.938417 Ry (doc −2540.938418), moment 2.140 μB (doc 2.14). The
convergence trajectories follow the documented ones to ~1e-6 Ry per iteration,
and the band-structure figures are identical. See `VALIDATION_2026.md` in the
tutorial repository.
