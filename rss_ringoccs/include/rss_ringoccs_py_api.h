#ifndef RSS_RINGOCCS_H
#define RSS_RINGOCCS_H

/*  To avoid compiler warnings about deprecated numpy stuff.                  */
#define PY_SSIZE_T_CLEAN
#define NPY_NO_DEPRECATED_API NPY_1_7_API_VERSION

#include <Python.h>
#include <structmember.h>

typedef struct {
    PyObject_HEAD
    PyObject   *B_rad_vals;
    PyObject   *D_km_vals;
    PyObject   *F_km_vals;
    PyObject   *f_sky_hz_vals;
    PyObject   *outfiles;
    PyObject   *p_norm_fwd_vals;
    PyObject   *p_norm_vals;
    PyObject   *phase_fwd_vals;
    PyObject   *phase_rad_vals;
    PyObject   *phase_vals;
    PyObject   *phi_rad_vals;
    PyObject   *phi_rl_rad_vals;
    PyObject   *power_vals;
    PyObject   *raw_tau_threshold_vals;
    PyObject   *rev_info;
    PyObject   *rho_corr_pole_km_vals;
    PyObject   *rho_corr_timing_km_vals;
    PyObject   *rho_dot_kms_vals;
    PyObject   *rho_km_vals;
    PyObject   *t_oet_spm_vals;
    PyObject   *t_ret_spm_vals;
    PyObject   *t_set_spm_vals;
    PyObject   *tau_threshold_vals;
    PyObject   *tau_vals;
    PyObject   *w_km_vals;
    PyObject   *dathist;
    PyObject   *history;
    PyObject   *perturb;
    PyObject   *rngreq;
    int         bfac;
    int         fwd;
    int         use_norm;
    int         use_fft;
    int         verbose;
    int         write_file;
    double      dx_km;
    double      ecc;
    double      input_res;
    double      norm_eq;
    double      peri;
    double      res;
    double      res_factor;
    double      sigma;
    double      range[2];
    long        n_used;
    long        finish;
    long        start;
    const char *psitype;
    const char *wtype;
    char        interp;
} PyDiffrecObj;

#endif