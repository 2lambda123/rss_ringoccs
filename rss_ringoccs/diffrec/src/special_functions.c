/*  To avoid compiler warnings about deprecated numpy stuff.                  */
#define NPY_NO_DEPRECATED_API NPY_1_7_API_VERSION

/*  Various header files required for the C-Python API to work.               */
#include <Python.h>
#include <numpy/ndarraytypes.h>
#include <numpy/ufuncobject.h>
#include "special_functions.h"

/*  compute_norm_eq, max and min found here. math.h included here as well.    */
#include "_diffraction_functions.h"
#include "_fresnel_kernel.h"

/*---------------------------DEFINE PYTHON FUNCTIONS--------------------------*
 *  This contains the Numpy-C and Python-C API parts that allow for the above *
 *  functions to be called in Python. Numpy arrays, as well as floating point *
 *  and integer valued arguments may then be passed into these functions for  *
 *  improvement in performance, as opposed to the routines written purely in  *
 *  Python. Successful compiling requires the Numpy and Python header files.  *
 *----------------------------------------------------------------------------*/

/*  Make sure these names are available.                                      */
#ifdef _array_from_one
#undef _array_from_one
#endif

/*  Make sure these names are available.                                      */
#ifdef _array_from_two
#undef _array_from_two
#endif

#ifdef _get_one_real_from_three_real
#undef _get_one_real_from_three_real
#endif

#ifdef _get_complex_from_four_real
#undef _get_complex_from_four_real
#endif

#ifdef _get_complex_from_three_real
#undef _get_complex_from_three_real
#endif

#ifdef OneVarFunctionForNumpy
#undef OneVarFunctionForNumpy
#endif

#ifdef WindowFunctionForNumpy
#undef WindowFunctionForNumpy
#endif

#ifdef VarToString
#undef VarToString
#endif

#define VarToString(Var) (#Var)

/*  To avoid repeating the same code over and over again, define these macros *
 *  to be used for looping over functions.                                    */
#define _array_from_one(x, y, dim, f) ({                                    \
    /*  Declare necessary variables.                                      */\
    long i;                                                                 \
    for (i=0; i<dim; ++i)   y[i] = (*f)(x[i]);                              \
})

#define _array_from_two(x1, x2, y, dim, f) ({                               \
    /*  Declare necessary variables.                                      */\
    long i;                                                                 \
    for (i=0; i<dim; ++i)   y[i] = (*f)(x1[i], x2);                         \
})

#define _get_one_real_from_three_real(x1, x2, x3, y, dim, f) ({             \
    /*  Declare necessary variables.                                      */\
    long i;                                                                 \
    for (i=0; i<dim; ++i)   y[i] = (*f)(x1[i], x2, x3);                     \
})

#define _get_complex_from_three_real(x, a, F, y, dim, f) ({                 \
    /*  Declare necessary variables.                                      */\
    long i;                                                                 \
    for (i=0; i<dim; ++i)   y[i] = (*f)(x[i], a, F);                        \
})

#define _get_complex_from_four_real(x, a, b, F, y, dim, f) ({\
    /*  Declare necessary variables.                                         */\
    long i;\
    \
    for (i=0; i<dim; ++i){\
        y[i] = (*f)(x[i], a, b, F);\
    }\
})

/*  Again, to avoid repetition, the code used in the API for numpy is arrays  *
 *  is identical for all functions with the exception of the function name.   *
 *  This preprocessor function saves thousands of lines of code, all of which *
 *  would be just copy/paste otherwise.                                       */
#define OneVarFunctionForNumpy(FuncName, CName)\
static PyObject * FuncName(PyObject *self, PyObject *args)\
{\
    /*  Declare necessary variables.                                         */\
    PyObject *output, *capsule;\
    PyArrayObject *x;\
    char typenum;\
    long dim;\
    void *data;\
    \
    /*  Parse the data from Python and try to convert it to a usable format. */\
    if (!PyArg_ParseTuple(args, "O!", &PyArray_Type, &x)){\
        PyErr_Format(PyExc_TypeError,\
                     "\n\rError Encountered: rss_ringoccs\n"\
                     "\r\tdiffrec.special_functions.%s\n\n"\
                     "\rCould not parse inputs. Legal inputs are:\n"\
                     "\r\tx: Numpy Array of real numbers (Floats)\n\rNotes:\n"\
                     "\r\tx must be a non-empty one dimensional numpy array.",\
                     VarToString(FuncName));\
        return NULL;\
    }\
    \
    /*  Grab useful information about the data.                              */\
    typenum = (char)PyArray_TYPE(x);\
    dim     = PyArray_DIMS(x)[0];\
    data    = PyArray_DATA(x);\
    \
    /*  Check the inputs to make sure they're valid.                         */\
    if (PyArray_NDIM(x) != 1){\
        PyErr_Format(PyExc_TypeError, "\n\rError Encountered: rss_ringoccs\n"\
                                      "\r\tdiffrec.special_functions.%s\n"\
                                      "\n\rInput is not 1-dimensional.\n",\
                                      VarToString(FuncName));\
        return NULL;\
    }\
    else if (dim == 0){\
        PyErr_Format(PyExc_TypeError, "\n\rError Encountered: rss_ringoccs\n"\
                                      "\r\tdiffrec.special_functions.%s"\
                                      "\n\n\rInput numpy array is empty.\n",\
                                      VarToString(FuncName));\
    }\
    \
    if (typenum == NPY_FLOAT){                                              \
        float *y;                                                           \
        y = (float *)malloc(dim*sizeof(float));                             \
        _array_from_one(((float *)data), y, dim, CName##_Float);            \
        output  = PyArray_SimpleNewFromData(1, &dim, NPY_FLOAT, (void *)y); \
        capsule = PyCapsule_New(y, NULL, capsule_cleanup);                  \
    }                                                                       \
    else if (typenum == NPY_DOUBLE){                                        \
        double *y;                                                          \
        y = (double *)malloc(dim*sizeof(double));                           \
        _array_from_one(((double *)data), y, dim, CName##_Double);          \
        output  = PyArray_SimpleNewFromData(1, &dim, NPY_DOUBLE, (void *)y);\
        capsule = PyCapsule_New(y, NULL, capsule_cleanup);                  \
    }                                                                       \
    else if (typenum == NPY_LONGDOUBLE){                                    \
        long double *y;                                                     \
        y = (long double *)malloc(dim*sizeof(long double));                 \
        _array_from_one(((long double *)data), y, dim,CName##_Long_Double); \
        output = PyArray_SimpleNewFromData(1, &dim, NPY_LONGDOUBLE, (void *)y);\
        capsule = PyCapsule_New(y, NULL, capsule_cleanup);                  \
    }                                                                       \
    else {                                                                  \
        double *y;                                                          \
        y = (double *)malloc(dim*sizeof(double));                           \
        if (typenum == NPY_BYTE)                                            \
            _array_from_one(((char *)data), y, dim, CName##_Char);          \
        else if (typenum == NPY_UBYTE)                                      \
            _array_from_one(((unsigned char *)data), y, dim, CName##_UChar);\
        else if (typenum == NPY_SHORT)                                      \
            _array_from_one(((short *)data), y, dim, CName##_Short);        \
        else if (typenum == NPY_USHORT)                                     \
            _array_from_one(((unsigned short *)data), y, dim, CName##_UShort);\
        else if (typenum == NPY_INT)                                        \
            _array_from_one(((int *)data), y, dim, CName##_Int);            \
        else if (typenum == NPY_UINT)                                       \
            _array_from_one(((unsigned int *)data), y, dim, CName##_UInt);  \
        else if (typenum == NPY_LONG)                                       \
            _array_from_one(((long *)data), y, dim, CName##_Long);          \
        else if (typenum == NPY_ULONG)                                      \
            _array_from_one(((unsigned long *)data), y, dim, CName##_ULong);\
        else if (typenum == NPY_LONGLONG)                                   \
            _array_from_one(((long long *)data), y, dim, CName##_Long_Long);\
        else if (typenum == NPY_ULONG)                                      \
            _array_from_one(((unsigned long long *)data), y, dim,           \
                            CName##_Long_Long);                             \
        else {                                                              \
            PyErr_Format(PyExc_TypeError,                                   \
                         "\n\rError Encountered: rss_ringoccs\n"            \
                         "\r\tdiffrec.special_functions.%s\n\n"             \
                         "\rInvalid data type for input array. Input should be"\
                         "\n\ra 1-dimensional array of real numbers.\n",    \
                         VarToString(FuncName));                            \
            return NULL;                                                    \
        }                                                                   \
                                                                            \
        output  = PyArray_SimpleNewFromData(1, &dim, NPY_DOUBLE, (void *)y);\
        capsule = PyCapsule_New(y, NULL, capsule_cleanup);                  \
    }                                                                       \
                                                                            \
    /*  This frees the variable at the Python level once it's destroyed.  */\
    PyArray_SetBaseObject((PyArrayObject *)output, capsule);\
                                                                            \
    /*  Return the results to Python.                                     */\
    return Py_BuildValue("N", output);                                      \
}

#define WindowFunctionForNumpy(FuncName, CName)\
static PyObject * FuncName(PyObject *self, PyObject *args)\
{\
    PyObject *output, *capsule;\
    PyArrayObject *x;\
    double dx;\
    char typenum;\
    long dim;\
    void *data;\
    \
    if (!PyArg_ParseTuple(args, "O!d", &PyArray_Type, &x, &dx))\
    {\
        PyErr_Format(PyExc_TypeError,\
                     "\n\rError Encountered: rss_ringoccs\n"\
                     "\r\tdiffrec.special_functions.%s\n\n"\
                     "\rCould not parse inputs. Legal inputs are:\n"\
                     "\r\tx:     Numpy Array of real numbers (Floats)\n"\
                     "\r\tdx:    Positive real number (Float)\n\rNotes:\n"\
                     "\r\tx must be a non-empty one dimensional numpy array.",\
                     VarToString(FuncName));\
        return NULL;\
    }\
    \
    /*  Useful information about the data.                                   */\
    typenum = (char)PyArray_TYPE(x);\
    dim     = PyArray_DIMS(x)[0];\
    data    = PyArray_DATA(x);\
    \
    /*  Check the inputs to make sure they're valid.                         */\
    if (PyArray_NDIM(x) != 1)\
    {\
        PyErr_Format(PyExc_TypeError, "\n\rError Encountered: rss_ringoccs\n"\
                                      "\r\tdiffrec.special_functions.%s\n\n"\
                                      "\rInput array is not 1-dimensional.\n",\
                                      VarToString(FuncName));\
        return NULL;\
    }\
    else if (dim == 0)\
    {\
        PyErr_Format(PyExc_TypeError, "\n\rError Encountered: rss_ringoccs\n"\
                                      "\r\tdiffrec.special_functions.%s\n\n"\
                                      "\rInput numpy array is empty.\n",\
                                      VarToString(FuncName));\
        return NULL;\
    }\
    \
    /*  Check that dx is positive.                                           */\
    if (dx <= 0)\
    {\
        PyErr_Format(PyExc_ValueError, "\n\rError Encountered: rss_ringoccs\n"\
                                       "\r\tdiffrec.special_functions.%s\n\n"\
                                       "\rdx must be a positive number.\n",\
                                       VarToString(FuncName));\
        return NULL;\
    }\
    \
    if (typenum == NPY_FLOAT)                                               \
    {                                                                       \
        float *y;                                                           \
        y = (float *)malloc(dim*sizeof(float));                             \
        _array_from_two(((float *)data), dx, y, dim, CName##_Float);        \
        output = PyArray_SimpleNewFromData(1, &dim, NPY_FLOAT, (void *)y);  \
        capsule = PyCapsule_New(y, NULL, capsule_cleanup);                  \
    }                                                                       \
    else if (typenum == NPY_LONGDOUBLE)                                     \
    {                                                                       \
        long double *y;                                                     \
        y = (long double *)malloc(dim*sizeof(long double));                 \
        _array_from_two(((long double *)data), dx, y, dim,                  \
                        CName##_Long_Double);                               \
        output = PyArray_SimpleNewFromData(1, &dim, NPY_LONGDOUBLE, (void *)y);\
        capsule = PyCapsule_New(y, NULL, capsule_cleanup);                  \
    }                                                                       \
    else                                                                    \
    {                                                                       \
        double *y;                                                          \
        y = (double *)malloc(dim*sizeof(double));                           \
                                                                            \
        if (typenum == NPY_DOUBLE)                                          \
            _array_from_two(((double *)data), dx, y, dim, CName##_Double);  \
        else if (typenum == NPY_BYTE)                                       \
            _array_from_two(((char *)data), dx, y, dim, CName##_Char);      \
        else if (typenum == NPY_UBYTE)                                      \
            _array_from_two(((unsigned char *)data), dx, y, dim,            \
                            CName##_UChar);                                 \
        else if (typenum == NPY_SHORT)                                      \
            _array_from_two(((short *)data), dx, y, dim, CName##_Short);    \
        else if (typenum == NPY_USHORT)                                     \
            _array_from_two(((unsigned short *)data), dx, y, dim,           \
                            CName##_UShort);                                \
        else if (typenum == NPY_INT)                                        \
            _array_from_two(((int *)data), dx, y, dim, CName##_Int);        \
        else if (typenum == NPY_UINT)                                       \
            _array_from_two(((unsigned int *)data), dx, y, dim, CName##_UInt);\
        else if (typenum == NPY_LONG)                                       \
            _array_from_two(((long *)data), dx, y, dim, CName##_Long);      \
        else if (typenum == NPY_ULONG)                                      \
            _array_from_two(((unsigned long *)data), dx, y, dim,            \
                            CName##_ULong);                                 \
        else if (typenum == NPY_LONGLONG)                                   \
            _array_from_two(((long long *)data), dx, y, dim,                \
                            CName##_Long_Long);                             \
        else if (typenum == NPY_ULONG)                                      \
            _array_from_two(((unsigned long long *)data), dx, y,            \
                            dim, CName##_ULong_Long);                       \
        else                                                                \
        {                                                                   \
            PyErr_Format(PyExc_TypeError,                                   \
                         "\n\rError Encountered: rss_ringoccs\n"            \
                         "\r\tdiffrec.special_functions.%s\n\n"             \
                         "\rInvalid data type for input array. Input should\n"\
                         "\rbe a 1-dimensional numpy array of real numbers.\n",\
                         VarToString(FuncName));                            \
            return NULL;                                                    \
        }                                                                   \
                                                                            \
        output = PyArray_SimpleNewFromData(1, &dim, NPY_DOUBLE, (void *)y); \
        capsule = PyCapsule_New(y, NULL, capsule_cleanup);                  \
    }                                                                       \
                                                                            \
    /*  This frees the variable at the Python level once it's destroyed.  */\
    PyArray_SetBaseObject((PyArrayObject *)output, capsule);                \
                                                                            \
    /*  Return the results to Python.                                     */\
    return Py_BuildValue("N", output);                                      \
}



/*  This function frees the memory allocated to a pointer by malloc when the  *
 *  corresponding variable is destroyed at the Python level.                  */
static void capsule_cleanup(PyObject *capsule)
{
    void *memory = PyCapsule_GetPointer(capsule, NULL);
    free(memory);
}

/*  Generate the code for the functions. These preprocessor functions are     *
 *  defined in special_functions.h.                                           */
OneVarFunctionForNumpy(besselJ0, BesselJ0);
OneVarFunctionForNumpy(besselI0, BesselI0);
OneVarFunctionForNumpy(sinc, Sinc);
OneVarFunctionForNumpy(fresnel_sin, Fresnel_Sin);
OneVarFunctionForNumpy(fresnel_cos, Fresnel_Cos);
OneVarFunctionForNumpy(lambertw, LambertW);
OneVarFunctionForNumpy(wavelength_to_wavenumber, Wavelength_To_Wavenumber);
OneVarFunctionForNumpy(frequency_to_wavelength, Frequency_To_Wavelength);
OneVarFunctionForNumpy(resolution_inverse, Resolution_Inverse);
WindowFunctionForNumpy(rect, Rect_Window);
WindowFunctionForNumpy(coss, Coss_Window);
WindowFunctionForNumpy(kb20, Kaiser_Bessel_2_0);
WindowFunctionForNumpy(kb25, Kaiser_Bessel_2_5);
WindowFunctionForNumpy(kb35, Kaiser_Bessel_3_5);
WindowFunctionForNumpy(kbmd20, Modified_Kaiser_Bessel_2_0);
WindowFunctionForNumpy(kbmd25, Modified_Kaiser_Bessel_2_5);
WindowFunctionForNumpy(kbmd35, Modified_Kaiser_Bessel_3_5);

static PyObject *compute_norm_eq(PyObject *self, PyObject *args)
{
    PyArrayObject *arr;
    PyObject *tuple = PyTuple_GetItem(args, 0);

    if (PyLong_Check(tuple)){
        long normeq;
        if (PyArg_ParseTuple(args, "l", &normeq)){
            return PyLong_FromLong(normeq);
        }
        else {
            PyErr_Format(
                PyExc_TypeError,
                "\n\r\trss_ringoccs.diffrec.math_functions.compute_norm_eq\n"
                "\r\t\tCould not parse int type input.");
            return NULL;
        }
    }
    else if (PyFloat_Check(tuple)){
        double normeq;
        if (PyArg_ParseTuple(args, "d", &normeq)){
            return PyFloat_FromDouble(normeq);
        }
        else {
            PyErr_Format(
                PyExc_TypeError,
                "\n\r\trss_ringoccs.diffrec.math_functions.compute_norm_eq\n"
                "\r\t\tCould not parse float type input.");
            return NULL;
        }
    }
    else if (PyArg_ParseTuple(args, "O!", &PyArray_Type, &arr)){
        npy_int typenum, dim;
        void *data;

        // Check to make sure input isn't zero dimensional!
        if (PyArray_NDIM(arr) != 1){
            PyErr_Format(
                PyExc_TypeError,
                "\n\trss_ringoccs.diffrec.special_functions.compute_norm_eq\n"
                "\r\t\tInput must be a one-dimensional array."
            );
            return NULL;
        }

        // Useful information about the data.
        typenum = PyArray_TYPE(arr);
        dim     = PyArray_DIMS(arr)[0];
        data    = PyArray_DATA(arr);
        if (dim == 0){
            PyErr_Format(
                PyExc_TypeError,
                "\n\r\trss_ringoccs.diffrec.math_functions.compute_norm_eq\n"
                "\r\t\tInput is zero dimensional."
            );
            return NULL;
        }
        if (typenum == NPY_FLOAT){
            return PyFloat_FromDouble(
                Normeq_Float((float *)data, dim)
            );
        }
        else if (typenum == NPY_DOUBLE){
            return PyFloat_FromDouble(
                Normeq_Double((double *)data, dim)
            );
        }
        else if (typenum == NPY_LONGDOUBLE){
            return PyFloat_FromDouble(
                Normeq_Long_Double((long double *)data, dim)
            );
        }
        else if (typenum == NPY_SHORT){
            return PyFloat_FromDouble(
                Normeq_Short((short *)data, dim)
            );
        }
        else if (typenum == NPY_INT){
            return PyFloat_FromDouble(
                Normeq_Int((int *)data, dim)
            );
        }
        else if (typenum == NPY_LONG){
            return PyFloat_FromDouble(
                Normeq_Long((long *)data, dim)
            );
        }
        else if (typenum == NPY_LONGLONG){
            return PyFloat_FromDouble(
                Normeq_Long_Long((long long *)data, dim)
            );
        }
        else {
            PyErr_Format(
                PyExc_TypeError,
                "\n\r\trss_ringoccs.diffrec.math_functions.compute_norm_eq\n"
                "\r\t\tInput should be a numpy array of numbers."
            );
            return NULL;
        }
    }
    else {
        PyErr_Format(
            PyExc_TypeError,
            "\n\r\trss_ringoccs.diffrec.math_functions.compute_norm_eq\n"
            "\r\t\tInput should be a numpy array of numbers."
        );
        return NULL;
    }
}

static PyObject *fresnel_transform(PyObject *self, PyObject *args)
{
    PyObject *output, *capsule, *perturb_list, *next, *iter;
    PyArrayObject *T_in,    *rho_km_vals, *F_km_vals, *phi_rad_vals;
    PyArrayObject *kd_vals, *B_rad_vals,  *D_km_vals, *w_km_vals;
    long start, n_used;
    int i;
    unsigned char wtype, use_norm, use_fft, use_fwd, order, interp;
    double ecc, peri;


    if (!(PyArg_ParseTuple(args, "O!O!O!O!O!O!O!O!Ollbbbbbbdd",
                           &PyArray_Type, &T_in,
                           &PyArray_Type, &rho_km_vals,
                           &PyArray_Type, &F_km_vals,
                           &PyArray_Type, &phi_rad_vals,
                           &PyArray_Type, &kd_vals,
                           &PyArray_Type, &B_rad_vals,
                           &PyArray_Type, &D_km_vals,
                           &PyArray_Type, &w_km_vals,
                           &perturb_list, &start, &n_used,
                           &wtype, &use_norm, &use_fwd, &use_fft,
                           &order, &interp, &ecc, &peri))){ 
        PyErr_Format(
            PyExc_TypeError,
            "\n\rError Encountered: rss_ringoccs\n"
            "\r\tdiffrec.special_functions.fresnel_transform\n\n"
            "\rCould not parse input variables. Inputs should be one\n"
            "\rdimensional numpy arrays and real numbers:\n"
            "\r\tT_in:        \t1-D complex numpy array\n"
            "\r\trho_km_vals: \t1-D real numpy array\n"
            "\r\tF_km_vals:   \t1-D real numpy array\n"
            "\r\tphi_rad_vals:\t1-D real numpy array\n"
            "\r\tkd_vals:     \t1-D real numpy array\n"
            "\r\tB_rad_vals:  \t1-D real numpy array\n"
            "\r\tD_km_vals:   \t1-D real numpy array\n"
            "\r\tw_km_vals:   \t1-D real numpy array\n"
            "\r\tstart:       \tPositive integer\n"
            "\r\tn_used:      \tPositive integer\n"
            "\r\twtype:       \tPositive integer\n"
            "\r\tuse_norm:    \tPositive integer\n"
            "\r\tuse_fwd:     \tPositive integer\n"
            "\r\torder:       \tPositive integer\n"
            "\r\tecc:         \tReal number\n"
            "\r\tperi:        \tReal number\n\n"
            "\rNOTE:\n"
            "\r\tOnly one dimensional numpy arrays are allowed. Only\n"
            "\r\tdouble types are supported. No current support for long\n"
            "\r\tdouble or float. Set this in Python with\n"
            "\r\tastype(numpy.float) or astype(numpy.float64).\n\n"
            "\r\tAlso note, order should be between 0 and 256."

        );
        return NULL;
    }

    DLPObj dlp;

    dlp.start    = start;
    dlp.n_used   = n_used;
    dlp.wtype    = wtype;
    dlp.use_norm = use_norm;
    dlp.use_fwd  = use_fwd;
    dlp.order    = order;
    dlp.ecc      = ecc;
    dlp.peri     = peri;
    dlp.interp   = interp;

    iter = PyObject_GetIter(perturb_list);
    if (!iter) {
        PyErr_Format(
            PyExc_TypeError,
            "\n\rError Encountered: rss_ringoccs\n"
            "\r\tdiffrec.special_functions.fresnel_transform\n\n"
            "\rperturb should be a python list of five floats or ints.\n"
        );
        return NULL;
    }

    for (i = 0; i < 5; ++i){
        next = PyIter_Next(iter);
        if (!next) {
            PyErr_Format(
                PyExc_TypeError,
                "\n\rError Encountered: rss_ringoccs\n"
                "\r\tdiffrec.special_functions.fresnel_transform\n\n"
                "\rperturb should be a python list of five floats or ints.\n"
                "\rSize of your list: %d", i
            );
        }

        if (PyLong_Check(next)) dlp.perturb[i] = (double)PyLong_AsLong(next);
        else if (PyFloat_Check(next)) dlp.perturb[i] = PyFloat_AsDouble(next);
        else {
            PyErr_Format(
                PyExc_TypeError,
                "\n\rError Encountered: rss_ringoccs\n"
                "\r\tdiffrec.special_functions.fresnel_transform\n\n"
                "\rperturb should be a python list of five floats or ints.\n"
                "\rYour list contains objects that are not real numbers.\n"
            );
        }
    }


    dlp.arr_size = PyArray_DIMS(T_in)[0];

    /*  Error checks on T_in.                                                 */
    if (PyArray_TYPE(T_in) != NPY_CDOUBLE){
        PyErr_Format(
            PyExc_TypeError,
            "\n\rError Encountered: rss_ringoccs\n"
            "\r\tdiffrec.special_functions.fresnel_transform\n\n"
            "\rFirst argument (T_in) must be a complex numpy array.\n"
        );
        return NULL;
    }
    else if (PyArray_NDIM(T_in) != 1){
        PyErr_Format(
            PyExc_IndexError,
            "\n\rError Encountered: rss_ringoccs\n"
            "\r\tdiffrec.special_functions.fresnel_transform\n\n"
            "\rFirst argument (T_in) must be one dimensional.\n"
            "\r\tNumber of dimensions: %ld", PyArray_NDIM(T_in)
        );
        return NULL;
    }
    else dlp.T_in = (complex double *)PyArray_DATA(T_in);

    /*  Error checks on rho_km_vals                                           */
    if (PyArray_TYPE(rho_km_vals) != NPY_DOUBLE){
        PyErr_Format(
            PyExc_TypeError,
            "\n\rError Encountered: rss_ringoccs\n"
            "\r\tdiffrec.special_functions.fresnel_transform\n\n"
            "\rSecond argument (rho_km_vals) must be a real numpy array.\n"
        );
        return NULL;
    }
    else if (PyArray_NDIM(rho_km_vals) != 1){
        PyErr_Format(
            PyExc_IndexError,
            "\n\rError Encountered: rss_ringoccs\n"
            "\r\tdiffrec.special_functions.fresnel_transform\n\n"
            "\rSecond argument (rho_km_vals) must be one dimensional.\n"
        );
        return NULL;
    }
    else if (PyArray_DIMS(rho_km_vals)[0] != dlp.arr_size){
        PyErr_Format(
            PyExc_IndexError,
            "\n\rError Encountered: rss_ringoccs\n"
            "\r\tdiffrec.special_functions.fresnel_transform\n\n"
            "\rrho_km_vals and T_in have a different number of elements.\n"
        );
        return NULL;
    }
    else dlp.rho_km_vals = (double *)PyArray_DATA(rho_km_vals);

    /*  Error checks on F_km_vals.                                            */
    if (PyArray_TYPE(F_km_vals) != NPY_DOUBLE){
        PyErr_Format(
            PyExc_TypeError,
            "\n\rError Encountered: rss_ringoccs\n"
            "\r\tdiffrec.special_functions.fresnel_transform\n\n"
            "\rThird argument (F_km_vals) must be a real numpy array.\n"
        );
        return NULL;
    }
    else if (PyArray_NDIM(F_km_vals) != 1){
        PyErr_Format(
            PyExc_IndexError,
            "\n\rError Encountered: rss_ringoccs\n"
            "\r\tdiffrec.special_functions.fresnel_transform\n\n"
            "\rThird argument (F_km_vals) must be one dimensional.\n"
        );
        return NULL;
    }
    else if (PyArray_DIMS(F_km_vals)[0] != dlp.arr_size){
        PyErr_Format(
            PyExc_IndexError,
            "\n\rError Encountered: rss_ringoccs\n"
            "\r\tdiffrec.special_functions.fresnel_transform\n\n"
            "\rF_km_vals and T_in have a different number of elements.\n"
        );
        return NULL;
    }
    else dlp.F_km_vals = (double *)PyArray_DATA(F_km_vals);

    /*  Error checks on phi_rad_vals.                                         */
    if (PyArray_TYPE(phi_rad_vals) != NPY_DOUBLE){
        PyErr_Format(
            PyExc_TypeError,
            "\n\rError Encountered: rss_ringoccs\n"
            "\r\tdiffrec.special_functions.fresnel_transform\n\n"
            "\rFourth argument (phi_rad_vals) must be a real numpy array.\n"
        );
        return NULL;
    }
    else if (PyArray_NDIM(phi_rad_vals) != 1){
        PyErr_Format(
            PyExc_IndexError,
            "\n\rError Encountered: rss_ringoccs\n"
            "\r\tdiffrec.special_functions.fresnel_transform\n\n"
            "\rFourth argument (phi_rad_vals) must be one dimensional.\n"
        );
        return NULL;
    }
    else if (PyArray_DIMS(phi_rad_vals)[0] != dlp.arr_size){
        PyErr_Format(
            PyExc_IndexError,
            "\n\rError Encountered: rss_ringoccs\n"
            "\r\tdiffrec.special_functions.fresnel_transform\n\n"
            "\rphi_rad_vals and T_in have a different number of elements.\n"
        );
        return NULL;
    }
    else dlp.phi_rad_vals = (double *)PyArray_DATA(phi_rad_vals);

    /*  Error checks on kd_vals.                                              */
    if (PyArray_TYPE(kd_vals) != NPY_DOUBLE){
        PyErr_Format(
            PyExc_TypeError,
            "\n\rError Encountered: rss_ringoccs\n"
            "\r\tdiffrec.special_functions.fresnel_transform\n\n"
            "\rFifth argument (kd_vals) must be a real numpy array.\n"
        );
        return NULL;
    }
    else if (PyArray_NDIM(kd_vals) != 1){
        PyErr_Format(
            PyExc_IndexError,
            "\n\rError Encountered: rss_ringoccs\n"
            "\r\tdiffrec.special_functions.fresnel_transform\n\n"
            "\rFifth argument (kd_vals) must be one dimensional.\n"
        );
        return NULL;
    }
    else if (PyArray_DIMS(kd_vals)[0] != dlp.arr_size){
        PyErr_Format(
            PyExc_IndexError,
            "\n\rError Encountered: rss_ringoccs\n"
            "\r\tdiffrec.special_functions.fresnel_transform\n\n"
            "\rkd_vals and T_in have a different number of elements.\n"
        );
        return NULL;
    }
    else dlp.kd_vals = (double *)PyArray_DATA(kd_vals);

    /*  Error checks on B_rad_vals.                                           */
    if (PyArray_TYPE(B_rad_vals) != NPY_DOUBLE){
        PyErr_Format(
            PyExc_TypeError,
            "\n\rError Encountered: rss_ringoccs\n"
            "\r\tdiffrec.special_functions.fresnel_transform\n\n"
            "\rSixth argument (B_rad_vals) must be a real numpy array.\n"
        );
        return NULL;
    }
    else if (PyArray_NDIM(B_rad_vals) != 1){
        PyErr_Format(
            PyExc_IndexError,
            "\n\rError Encountered: rss_ringoccs\n"
            "\r\tdiffrec.special_functions.fresnel_transform\n\n"
            "\rSixth argument (B_rad_vals) must be one dimensional.\n"
        );
        return NULL;
    }
    else if (PyArray_DIMS(B_rad_vals)[0] != dlp.arr_size){
        PyErr_Format(
            PyExc_IndexError,
            "\n\rError Encountered: rss_ringoccs\n"
            "\r\tdiffrec.special_functions.fresnel_transform\n\n"
            "\rB_rad_vals and T_in have a different number of elements.\n"
        );
        return NULL;
    }
    else dlp.B_rad_vals = (double *)PyArray_DATA(B_rad_vals);

    /*  Error checks on D_km_vals.                                            */
    if (PyArray_TYPE(D_km_vals) != NPY_DOUBLE){
        PyErr_Format(
            PyExc_TypeError,
            "\n\rError Encountered: rss_ringoccs\n"
            "\r\tdiffrec.special_functions.fresnel_transform\n\n"
            "\rSeventh argument (D_km_vals) must be a real numpy array.\n"
        );
        return NULL;
    }
    else if (PyArray_NDIM(D_km_vals) != 1){
        PyErr_Format(
            PyExc_IndexError,
            "\n\rError Encountered: rss_ringoccs\n"
            "\r\tdiffrec.special_functions.fresnel_transform\n\n"
            "\rSeventh argument (D_km_vals) must be one dimensional.\n"
        );
        return NULL;
    }
    else if (PyArray_DIMS(B_rad_vals)[0] != dlp.arr_size){
        PyErr_Format(
            PyExc_IndexError,
            "\n\rError Encountered: rss_ringoccs\n"
            "\r\tdiffrec.special_functions.fresnel_transform\n\n"
            "\rD_km_vals and T_in have a different number of elements.\n"
        );
        return NULL;
    }
    else dlp.D_km_vals = (double *)PyArray_DATA(D_km_vals);

    /*  Error checks on w_km_vals.                                            */
    if (PyArray_TYPE(w_km_vals) != NPY_DOUBLE){
        PyErr_Format(
            PyExc_TypeError,
            "\n\rError Encountered: rss_ringoccs\n"
            "\r\tdiffrec.special_functions.fresnel_transform\n\n"
            "\rEigth argument (w_km_vals) must be a real numpy array.\n"
        );
        return NULL;
    }
    else if (PyArray_NDIM(w_km_vals) != 1){
        PyErr_Format(
            PyExc_IndexError,
            "\n\rError Encountered: rss_ringoccs\n"
            "\r\tdiffrec.special_functions.fresnel_transform\n\n"
            "\rEigth argument (w_km_vals) must be one dimensional.\n"
        );
        return NULL;
    }
    else if (PyArray_DIMS(w_km_vals)[0] != dlp.arr_size){
        PyErr_Format(
            PyExc_IndexError,
            "\n\rError Encountered: rss_ringoccs\n"
            "\r\tdiffrec.special_functions.fresnel_transform\n\n"
            "\rw_km_vals and T_in have a different number of elements.\n"
        );
        return NULL;
    }
    else dlp.w_km_vals = (double *)PyArray_DATA(w_km_vals);

    if (start > dlp.arr_size){
        PyErr_Format(
            PyExc_IndexError,
            "\n\rError Encountered: rss_ringoccs\n"
            "\r\tdiffrec.special_functions.fresnel_transform\n\n"
            "\rStarting index (start) is greater than the size of the array.\n"
        );
        return NULL;
    }
    else if (start+n_used > dlp.arr_size){
        PyErr_Format(
            PyExc_IndexError,
            "\n\rError Encountered: rss_ringoccs\n"
            "\r\tdiffrec.special_functions.fresnel_transform\n\n"
            "\rFinal index (start+n_used) is greater than size of array.\n"
        );
        return NULL;
    }

    dlp.T_out = (complex double *)malloc((dlp.n_used+1)*sizeof(complex double));

    if (use_fft) DiffractionCorrectionSimpleFFT(&dlp);
    else {
        if (dlp.order == 0){
            if ((dlp.ecc == 0.0) && (dlp.peri == 0.0))
                if ((dlp.perturb[0] == 0) && (dlp.perturb[1] == 0) &&
                    (dlp.perturb[2] == 0) && (dlp.perturb[3] == 0) &&
                    (dlp.perturb[4] == 0)) {
                    DiffractionCorrectionNewton(&dlp);
                }
                else DiffractionCorrectionPerturbedNewton(&dlp);
            else DiffractionCorrectionEllipse(&dlp);
        }
        else if (dlp.order == 1) DiffractionCorrectionFresnel(&dlp);
        else DiffractionCorrectionLegendre(&dlp);
    }
    if (dlp.status == 0){

        /*  Create a Numpy array object to be passed back to Python.          */
        dlp.n_used += 1;
        output = PyArray_SimpleNewFromData(1, &dlp.n_used,
                                           NPY_CDOUBLE, (void *)dlp.T_out);

        /*  This frees the variable at the Python level once it's destroyed.  */
        capsule = PyCapsule_New(dlp.T_out, NULL, capsule_cleanup);
        PyArray_SetBaseObject((PyArrayObject *)output, capsule);

        /*  Return the results to Python.                                     */
        return Py_BuildValue("N", output);
    }
    else if (dlp.status == 1){
        PyErr_Format(
            PyExc_TypeError,
            "\n\rError Encountered: rss_ringoccs\n"
            "\r\tdiffrec.special_functions.fresnel_transform\n\n"
            "\rCould not extract data from inputs.\n"
        );
        return NULL;
    }
    else if (dlp.status == 2){
        PyErr_Format(
            PyExc_IndexError,
            "\n\rError Encountered: rss_ringoccs\n"
            "\r\tdiffrec.special_functions.fresnel_transform\n\n"
            "\r\tRequired window width goes beyond the available data range.\n"
            "\r\t\tBad Point (Index):         \t%ld\n"
            "\r\t\tNumber of Points in Window:\t%ld\n"
            "\r\t\tDifference:                \t%ld\n"
            "\r\t\tSum:                       \t%ld\n"
            "\r\t\tArray Size:                \t%ld\n"
            "\r\tDifference must be positive and sum must\n"
            "\r\tbe less than array size.\n",
            dlp.start, dlp.n_used, dlp.start-dlp.n_used, dlp.start+dlp.n_used,
            dlp.arr_size
        );
        return NULL;
    }
    else if (dlp.status == 3){
        PyErr_Format(
            PyExc_MemoryError,
            "\rError Encountered: rss_ringoccs"
            "\r\tspecial_functions.fresnel_transform\n\n"
            "\rMalloc failed to create new variables.\n"
            "\rYou are most likely out of memory.\n"
        );
        return NULL;
    }
    else if (dlp.status == 4){
        PyErr_Format(
            PyExc_MemoryError,
            "\rError Encountered: rss_ringoccs"
            "\r\tspecial_functions.fresnel_transform\n\n"
            "\rInterp should be either 0, 2, 3, or 4."
        );
        return NULL;
    }
    else {
        PyErr_Format(
            PyExc_TypeError,
            "\n\rError Encountered: rss_ringoccs\n"
            "\r\tdiffrec.special_functions.fresnel_transform\n\n"
            "\rInput should be one dimensional numpy arrays and\n"
            "\rreal valued numbers. The following are expected:\n"
            "\r\tT_in:        \t1-D complex numpy array\n"
            "\r\trho_km_vals: \t1-D real numpy array\n"
            "\r\tF_km_vals:   \t1-D real numpy array\n"
            "\r\tphi_rad_vals:\t1-D real numpy array\n"
            "\r\tkd_vals:     \t1-D real numpy array\n"
            "\r\tB_rad_vals:  \t1-D real numpy array\n"
            "\r\tD_km_vals:   \t1-D real numpy array\n"
            "\r\tw_km_vals:   \t1-D real numpy array\n"
            "\r\tstart:       \tPositive integer\n"
            "\r\tn_used:      \tPositive integer\n"
            "\r\twtype:       \tPositive integer\n"
            "\r\tuse_norm:    \tPositive integer\n"
            "\r\tuse_fwd:     \tPositive integer\n"
            "\r\torder:       \tPositive integer\n"
            "\r\tecc:         \tReal number\n"
            "\r\tperi:        \tReal number\n\n"
            "\rNOTE:\n"
            "\r\tOnly one dimensional numpy arrays are allowed. Only\n"
            "\r\tdouble types are supported. No current support for long\n"
            "\r\tdouble or float. Set this in Python with\n"
            "\r\tastype(numpy.float) or astype(numpy.float64).\n"
        );
        return NULL;
    }
}

static PyObject *gap_diffraction(PyObject *self, PyObject *args)
{
    PyObject *output, *capsule;
    PyArrayObject *rho;
    double a, b, F;
    char typenum;
    long dim;
    void *data;

    if (!PyArg_ParseTuple(args, "O!ddd", &PyArray_Type, &rho, &a, &b, &F)){
        PyErr_Format(
            PyExc_TypeError,
            "\n\rError Encountered: rss_ringoccs\n"
            "\r\tdiffrec.special_functions.gap_diffraction\n\n"
            "\rCould not parse inputs. Legal inputs are:\n"
            "\r\trho:   Numpy Array of positive real numbers (Floats)\n"
            "\r\ta:     Positive constant (Float)\n"
            "\r\tb:     Positive constant (Float) greater than a\n"
            "\r\tF      Positive constant (Float)\n\n"
            "\rNotes:\n"
            "\r\trho must be a non-empty one dimensional numpy array."
        );
        return NULL;
    }

    /*  Useful information about the data.                                */
    typenum = (char)PyArray_TYPE(rho);
    dim     = PyArray_DIMS(rho)[0];
    data    = PyArray_DATA(rho);

    /*  Check the inputs to make sure they're valid.                          */
    if (PyArray_NDIM(rho) != 1){
        PyErr_Format(
            PyExc_TypeError,
            "\n\rError Encountered: rss_ringoccs\n"
            "\r\tdiffrec.special_functions.gap_diffraction\n\n"
            "\rInput numpy array is not one-dimensional.\n"
        );
        return NULL;
    }
    else if (a >= b){
        PyErr_Format(
            PyExc_TypeError,
            "\n\rError Encountered: rss_ringoccs\n"
            "\r\tdiffrec.special_functions.gap_diffraction\n\n"
            "\rInner radius is not less than outer radius (i.e. a >= b).\n"
        );
        return NULL;
    }
    else if (a <= 0.0){
        PyErr_Format(
            PyExc_TypeError,
            "\n\rError Encountered: rss_ringoccs\n"
            "\r\tdiffrec.special_functions.gap_diffraction\n\n"
            "\rInner radius is negative. (i.e. a<0)\n"
        );
        return NULL;
    }
    else if (F < 0.0){
        PyErr_Format(
            PyExc_TypeError,
            "\n\rError Encountered: rss_ringoccs\n"
            "\r\tdiffrec.special_functions.gap_diffraction\n\n"
            "\rFresnel scale is negative (i.e. F<0).\n"
        );
        return NULL;
    }
    else if (dim == 0){
        PyErr_Format(
            PyExc_TypeError,
            "\n\rError Encountered: rss_ringoccs\n"
            "\r\tdiffrec.special_functions.gap_diffraction\n\n"
            "\rInput numpy array is empty.\n"
        );
    }

    if (typenum == NPY_FLOAT){
        complex float *T_hat;
        T_hat = (complex float *)malloc(dim*sizeof(complex float));
        _get_complex_from_four_real(((float *)data), a, b, F, T_hat,
                                     dim, Gap_Diffraction_Float);
        output = PyArray_SimpleNewFromData(1, &dim, NPY_CFLOAT, (void *)T_hat);
        capsule = PyCapsule_New(T_hat, NULL, capsule_cleanup);
    }
    else if (typenum == NPY_DOUBLE){
        complex double *T_hat;
        T_hat = (complex double *)malloc(dim*sizeof(complex double));
        _get_complex_from_four_real(((double *)data), a, b, F, T_hat, dim,
                                     Gap_Diffraction_Double);
        output = PyArray_SimpleNewFromData(1, &dim, NPY_CDOUBLE, (void *)T_hat);
        capsule = PyCapsule_New(T_hat, NULL, capsule_cleanup);
    }
    else if (typenum == NPY_LONGDOUBLE){
        complex long double *T_hat;
        T_hat = (complex long double *)malloc(dim*sizeof(complex long double));
        _get_complex_from_four_real(((long double *)data), a, b, F, T_hat,
                                     dim, Gap_Diffraction_Long_Double);
        output = PyArray_SimpleNewFromData(1, &dim, NPY_CLONGDOUBLE,
                                           (void *)T_hat);
        capsule = PyCapsule_New(T_hat, NULL, capsule_cleanup);
    }
    else {
        complex double *T_hat;
        T_hat = (complex double *)malloc(dim*sizeof(complex double));

        if (typenum == NPY_BYTE){
            _get_complex_from_four_real(((char *)data), a, b, F, T_hat, dim,
                                         Gap_Diffraction_Char);
        }
        else if (typenum == NPY_UBYTE){
            _get_complex_from_four_real(((unsigned char *)data), a, b, F,
                                         T_hat, dim, Gap_Diffraction_UChar);
        }
        else if (typenum == NPY_SHORT){
            _get_complex_from_four_real(((short *)data), a, b, F, T_hat, dim,
                                         Gap_Diffraction_Short);
        }
        else if (typenum == NPY_USHORT){
            _get_complex_from_four_real(((unsigned short *)data), a, b, F,
                                         T_hat, dim, Gap_Diffraction_UShort);
        }
        else if (typenum == NPY_INT){
            _get_complex_from_four_real(((int *)data), a, b, F, T_hat, dim,
                                         Gap_Diffraction_Int);
        }
        else if (typenum == NPY_UINT){
            _get_complex_from_four_real(((unsigned int *)data), a, b, F, T_hat,
                                         dim, Gap_Diffraction_UInt);
        }
        else if (typenum == NPY_LONG){
            _get_complex_from_four_real(((long *)data), a, b, F, T_hat, dim,
                                         Gap_Diffraction_Long);
        }
        else if (typenum == NPY_ULONG){
            _get_complex_from_four_real(((unsigned long *)data), a, b, F,
                                         T_hat, dim, Gap_Diffraction_ULong);
        }
        else if (typenum == NPY_LONGLONG){
            _get_complex_from_four_real(((long long *)data), a, b, F, T_hat,
                                         dim, Gap_Diffraction_Long_Long);
        }
        else if (typenum == NPY_ULONG){
            _get_complex_from_four_real(((unsigned long long *)data), a, b, F,
                                         T_hat, dim,
                                         Gap_Diffraction_ULong_Long);
        }
        else {
            PyErr_Format(
                PyExc_TypeError,
                "\n\rError Encountered: rss_ringoccs\n"
                "\r\tdiffrec.special_functions.gap_diffraction\n\n"
                "\rInvalid data type for input numpy array. Input should be\n"
                "\ra one dimensional numpy array of positive (floating point)\n"
                "\rreal numbers."
            );
            return NULL;
        }

        output = PyArray_SimpleNewFromData(1, &dim, NPY_CDOUBLE, (void *)T_hat);
        capsule = PyCapsule_New(T_hat, NULL, capsule_cleanup);
    }

    /*  This frees the variable at the Python level once it's destroyed.      */
    PyArray_SetBaseObject((PyArrayObject *)output, capsule);

    /*  Return the results to Python.                                         */
    return Py_BuildValue("N", output);
}

static PyObject *ringlet_diffraction(PyObject *self, PyObject *args)
{
    PyObject *output, *capsule;
    PyArrayObject *rho;
    double a, b, F;
    char typenum;
    long dim;
    void *data;

    if (!PyArg_ParseTuple(args, "O!ddd", &PyArray_Type, &rho, &a, &b, &F)){
        PyErr_Format(
            PyExc_TypeError,
            "\n\rError Encountered: rss_ringoccs\n"
            "\r\tdiffrec.special_functions.ringlet_diffraction\n\n"
            "\rCould not parse inputs. Legal inputs are:\n"
            "\r\trho:   Numpy Array of positive real numbers (Floats)\n"
            "\r\ta:     Positive constant (Float)\n"
            "\r\tb:     Positive constant (Float) greater than a\n"
            "\r\tF      Positive constant (Float)\n\n"
            "\rNotes:\n"
            "\r\trho must be a non-empty one dimensional numpy array."
        );
        return NULL;
    }

    /*  Useful information about the data.                                */
    typenum = (char)PyArray_TYPE(rho);
    dim     = PyArray_DIMS(rho)[0];
    data    = PyArray_DATA(rho);

    /*  Check the inputs to make sure they're valid.                          */
    if (PyArray_NDIM(rho) != 1){
        PyErr_Format(
            PyExc_TypeError,
            "\n\rError Encountered: rss_ringoccs\n"
            "\r\tdiffrec.special_functions.ringlet_diffraction\n\n"
            "\rInput numpy array is not one-dimensional.\n"
        );
        return NULL;
    }
    else if (a >= b){
        PyErr_Format(
            PyExc_TypeError,
            "\n\rError Encountered: rss_ringoccs\n"
            "\r\tdiffrec.special_functions.ringlet_diffraction\n\n"
            "\rInner radius is not less than outer radius (i.e. a >= b).\n"
        );
        return NULL;
    }
    else if (a <= 0.0){
        PyErr_Format(
            PyExc_TypeError,
            "\n\rError Encountered: rss_ringoccs\n"
            "\r\tdiffrec.special_functions.ringlet_diffraction\n\n"
            "\rInner radius is negative. (i.e. a<0)\n"
        );
        return NULL;
    }
    else if (F < 0.0){
        PyErr_Format(
            PyExc_TypeError,
            "\n\rError Encountered: rss_ringoccs\n"
            "\r\tdiffrec.special_functions.ringlet_diffraction\n\n"
            "\rFresnel scale is negative (i.e. F<0).\n"
        );
        return NULL;
    }
    else if (dim == 0){
        PyErr_Format(
            PyExc_TypeError,
            "\n\rError Encountered: rss_ringoccs\n"
            "\r\tdiffrec.special_functions.ringlet_diffraction\n\n"
            "\rInput numpy array is empty.\n"
        );
    }

    if (typenum == NPY_FLOAT){
        complex float *T_hat;
        T_hat = (complex float *)malloc(dim*sizeof(complex float));

        _get_complex_from_four_real(
            ((float *)data), a, b, F, T_hat, dim,
            Ringlet_Diffraction_Float
        );

        output = PyArray_SimpleNewFromData(1, &dim, NPY_CFLOAT, (void *)T_hat);
        capsule = PyCapsule_New(T_hat, NULL, capsule_cleanup);
    }
    else if (typenum == NPY_LONGDOUBLE){
        complex long double *T_hat;
        T_hat = (complex long double *)malloc(dim*sizeof(complex long double));

        _get_complex_from_four_real(
            ((long double *)data), a, b, F, T_hat, dim,
            Ringlet_Diffraction_Long_Double
        );

        output = PyArray_SimpleNewFromData(1, &dim, NPY_CLONGDOUBLE,
                                           (void *)T_hat);
        capsule = PyCapsule_New(T_hat, NULL, capsule_cleanup);
    }
    else {
        complex double *T_hat;
        T_hat = (complex double *)malloc(dim*sizeof(complex double));

        if (typenum == NPY_DOUBLE){
            _get_complex_from_four_real(
                ((double *)data), a, b, F, T_hat, dim,
                Ringlet_Diffraction_Double
            );
        }
        else if (typenum == NPY_BYTE){
            _get_complex_from_four_real(((char *)data), a, b, F, T_hat, dim,
                                         Ringlet_Diffraction_Char);
        }
        else if (typenum == NPY_UBYTE){
            _get_complex_from_four_real(((unsigned char *)data), a, b, F,
                                         T_hat, dim, Ringlet_Diffraction_UChar);
        }
        else if (typenum == NPY_SHORT){
            _get_complex_from_four_real(((short *)data), a, b, F, T_hat, dim,
                                         Ringlet_Diffraction_Short);
        }
        else if (typenum == NPY_USHORT){
            _get_complex_from_four_real(((unsigned short *)data), a, b, F,
                                         T_hat, dim,
                                         Ringlet_Diffraction_UShort);
        }
        else if (typenum == NPY_INT){
            _get_complex_from_four_real(((int *)data), a, b, F, T_hat, dim,
                                         Ringlet_Diffraction_Int);
        }
        else if (typenum == NPY_UINT){
            _get_complex_from_four_real(((unsigned int *)data), a, b, F, T_hat,
                                         dim, Ringlet_Diffraction_UInt);
        }
        else if (typenum == NPY_LONG){
            _get_complex_from_four_real(((long *)data), a, b, F, T_hat, dim,
                                         Ringlet_Diffraction_Long);
        }
        else if (typenum == NPY_ULONG){
            _get_complex_from_four_real(((unsigned long *)data), a, b, F,
                                         T_hat, dim, Ringlet_Diffraction_ULong);
        }
        else if (typenum == NPY_LONGLONG){
            _get_complex_from_four_real(((long long *)data), a, b, F, T_hat,
                                         dim, Ringlet_Diffraction_Long_Long);
        }
        else if (typenum == NPY_ULONG){
            _get_complex_from_four_real(((unsigned long long *)data), a, b, F,
                                         T_hat, dim,
                                         Ringlet_Diffraction_ULong_Long);
        }
        else {
            PyErr_Format(
                PyExc_TypeError,
                "\n\rError Encountered: rss_ringoccs\n"
                "\r\tdiffrec.special_functions.ringlet_diffraction\n\n"
                "\rInvalid data type for input numpy array. Input should be\n"
                "\ra one dimensional numpy array of positive (floating point)\n"
                "\rreal numbers."
            );
            return NULL;
        }

        output = PyArray_SimpleNewFromData(1, &dim, NPY_CDOUBLE, (void *)T_hat);
        capsule = PyCapsule_New(T_hat, NULL, capsule_cleanup);
    }

    /*  This frees the variable at the Python level once it's destroyed.      */
    PyArray_SetBaseObject((PyArrayObject *)output, capsule);

    /*  Return the results to Python.                                         */
    return Py_BuildValue("N", output);
}

static PyObject *right_straightedge(PyObject *self, PyObject *args)
{
    PyObject *output, *capsule;
    PyArrayObject *rho;
    double a, F;
    char typenum;
    long dim;
    void *data;

    if (!PyArg_ParseTuple(args, "O!dd", &PyArray_Type, &rho, &a, &F)){
        PyErr_Format(
            PyExc_TypeError,
            "\n\rError Encountered: rss_ringoccs\n"
            "\r\tdiffrec.special_functions.right_straightedge\n\n"
            "\rCould not parse inputs. Legal inputs are:\n"
            "\r\trho:   Numpy Array of positive real numbers (Floats)\n"
            "\r\ta:     Positive constant (Float)\n"
            "\r\tF      Positive constant (Float)\n\n"
            "\rNotes:\n"
            "\r\trho must be a non-empty one dimensional numpy array."
        );
        return NULL;
    }

    /*  Useful information about the data.                                */
    typenum = (char)PyArray_TYPE(rho);
    dim     = PyArray_DIMS(rho)[0];
    data    = PyArray_DATA(rho);

    /*  Check the inputs to make sure they're valid.                          */
    if (PyArray_NDIM(rho) != 1){
        PyErr_Format(
            PyExc_TypeError,
            "\n\rError Encountered: rss_ringoccs\n"
            "\r\tdiffrec.special_functions.right_straightedge\n\n"
            "\rInput numpy array is not one-dimensional.\n"
        );
        return NULL;
    }
    else if (a <= 0.0){
        PyErr_Format(
            PyExc_TypeError,
            "\n\rError Encountered: rss_ringoccs\n"
            "\r\tdiffrec.special_functions.right_straightedge\n\n"
            "\rInner radius is negative. (i.e. a<0)\n"
        );
        return NULL;
    }
    else if (F < 0.0){
        PyErr_Format(
            PyExc_TypeError,
            "\n\rError Encountered: rss_ringoccs\n"
            "\r\tdiffrec.special_functions.right_straightedge\n\n"
            "\rFresnel scale is negative (i.e. F<0).\n"
        );
        return NULL;
    }
    else if (dim == 0){
        PyErr_Format(
            PyExc_TypeError,
            "\n\rError Encountered: rss_ringoccs\n"
            "\r\tdiffrec.special_functions.right_straightedge\n\n"
            "\rInput numpy array is empty.\n"
        );
    }

    if (typenum == NPY_FLOAT){
        complex float *T_hat;
        T_hat = (complex float *)malloc(dim*sizeof(complex float));

        _get_complex_from_three_real(
            ((float *)data), a, F, T_hat, dim,
            Right_Straightedge_Diffraction_Float
        );

        output = PyArray_SimpleNewFromData(1, &dim, NPY_CFLOAT, (void *)T_hat);
        capsule = PyCapsule_New(T_hat, NULL, capsule_cleanup);
    }
    else if (typenum == NPY_LONGDOUBLE){
        complex long double *T_hat;
        T_hat = (complex long double *)malloc(dim*sizeof(complex long double));

        _get_complex_from_three_real(
            ((long double *)data), a, F, T_hat, dim,
            Right_Straightedge_Diffraction_Long_Double
        );

        output = PyArray_SimpleNewFromData(1, &dim, NPY_CLONGDOUBLE,
                                           (void *)T_hat);
        capsule = PyCapsule_New(T_hat, NULL, capsule_cleanup);
    }
    else {
        complex double *T_hat;
        T_hat = (complex double *)malloc(dim*sizeof(complex double));

        if (typenum == NPY_DOUBLE){
            _get_complex_from_three_real(
                ((double *)data), a, F, T_hat, dim,
                Right_Straightedge_Diffraction_Double
            );
        }
        else if (typenum == NPY_BYTE){
            _get_complex_from_three_real(
                ((char *)data), a, F, T_hat, dim,
                Right_Straightedge_Diffraction_Char
            );
        }
        else if (typenum == NPY_UBYTE){
            _get_complex_from_three_real(
                ((unsigned char *)data), a, F, T_hat, dim,
                Right_Straightedge_Diffraction_UChar
            );
        }
        else if (typenum == NPY_SHORT){
            _get_complex_from_three_real(
                ((short *)data), a, F, T_hat, dim,
                Right_Straightedge_Diffraction_Short
            );
        }
        else if (typenum == NPY_USHORT){
            _get_complex_from_three_real(
                ((unsigned short *)data), a, F, T_hat, dim,
                Right_Straightedge_Diffraction_UShort
            );
        }
        else if (typenum == NPY_INT){
            _get_complex_from_three_real(
                ((int *)data), a, F, T_hat, dim,
                Right_Straightedge_Diffraction_Int
            );
        }
        else if (typenum == NPY_UINT){
            _get_complex_from_three_real(
                ((unsigned int *)data), a, F, T_hat, dim,
                Right_Straightedge_Diffraction_UInt);
        }
        else if (typenum == NPY_LONG){
            _get_complex_from_three_real(
                ((long *)data), a, F, T_hat, dim,
                Right_Straightedge_Diffraction_Long
            );
        }
        else if (typenum == NPY_ULONG){
            _get_complex_from_three_real(
                ((unsigned long *)data), a, F, T_hat, dim,
                Right_Straightedge_Diffraction_ULong
            );
        }
        else if (typenum == NPY_LONGLONG){
            _get_complex_from_three_real(
                ((long long *)data), a, F, T_hat, dim,
                Right_Straightedge_Diffraction_Long_Long);
        }
        else if (typenum == NPY_ULONG){
            _get_complex_from_three_real(
                ((unsigned long long *)data), a, F, T_hat, dim,
                Right_Straightedge_Diffraction_ULong_Long);
        }
        else {
            PyErr_Format(
                PyExc_TypeError,
                "\n\rError Encountered: rss_ringoccs\n"
                "\r\tdiffrec.special_functions.right_straightedge\n\n"
                "\rInvalid data type for input numpy array. Input should be\n"
                "\ra one dimensional numpy array of positive (floating point)\n"
                "\rreal numbers."
            );
            return NULL;
        }

        output = PyArray_SimpleNewFromData(1, &dim, NPY_CDOUBLE, (void *)T_hat);
        capsule = PyCapsule_New(T_hat, NULL, capsule_cleanup);
    }

    /*  This frees the variable at the Python level once it's destroyed.      */
    PyArray_SetBaseObject((PyArrayObject *)output, capsule);

    /*  Return the results to Python.                                         */
    return Py_BuildValue("N", output);
}

static PyObject *left_straightedge(PyObject *self, PyObject *args)
{
    PyObject *output, *capsule;
    PyArrayObject *rho;
    double a, F;
    char typenum;
    long dim;
    void *data;

    if (!PyArg_ParseTuple(args, "O!dd", &PyArray_Type, &rho, &a, &F)){
        PyErr_Format(
            PyExc_TypeError,
            "\n\rError Encountered: rss_ringoccs\n"
            "\r\tdiffrec.special_functions.left_straightedge\n\n"
            "\rCould not parse inputs. Legal inputs are:\n"
            "\r\trho:   Numpy Array of positive real numbers (Floats)\n"
            "\r\ta:     Positive constant (Float)\n"
            "\r\tF      Positive constant (Float)\n\n"
            "\rNotes:\n"
            "\r\trho must be a non-empty one dimensional numpy array."
        );
        return NULL;
    }

    /*  Useful information about the data.                                */
    typenum = (char)PyArray_TYPE(rho);
    dim     = PyArray_DIMS(rho)[0];
    data    = PyArray_DATA(rho);

    /*  Check the inputs to make sure they're valid.                          */
    if (PyArray_NDIM(rho) != 1){
        PyErr_Format(
            PyExc_TypeError,
            "\n\rError Encountered: rss_ringoccs\n"
            "\r\tdiffrec.special_functions.left_straightedge\n\n"
            "\rInput numpy array is not one-dimensional.\n"
        );
        return NULL;
    }
    else if (a <= 0.0){
        PyErr_Format(
            PyExc_TypeError,
            "\n\rError Encountered: rss_ringoccs\n"
            "\r\tdiffrec.special_functions.left_straightedge\n\n"
            "\rInner radius is negative. (i.e. a<0)\n"
        );
        return NULL;
    }
    else if (F < 0.0){
        PyErr_Format(
            PyExc_TypeError,
            "\n\rError Encountered: rss_ringoccs\n"
            "\r\tdiffrec.special_functions.left_straightedge\n\n"
            "\rFresnel scale is negative (i.e. F<0).\n"
        );
        return NULL;
    }
    else if (dim == 0){
        PyErr_Format(
            PyExc_TypeError,
            "\n\rError Encountered: rss_ringoccs\n"
            "\r\tdiffrec.special_functions.left_straightedge\n\n"
            "\rInput numpy array is empty.\n"
        );
    }

    if (typenum == NPY_FLOAT){
        complex float *T_hat;
        T_hat = (complex float *)malloc(dim*sizeof(complex float));

        _get_complex_from_three_real(
            ((float *)data), a, F, T_hat, dim,
            Left_Straightedge_Diffraction_Float
        );

        output = PyArray_SimpleNewFromData(1, &dim, NPY_CFLOAT, (void *)T_hat);
        capsule = PyCapsule_New(T_hat, NULL, capsule_cleanup);
    }
    else if (typenum == NPY_LONGDOUBLE){
        complex long double *T_hat;
        T_hat = (complex long double *)malloc(dim*sizeof(complex long double));

        _get_complex_from_three_real(
            ((long double *)data), a, F, T_hat, dim,
            Left_Straightedge_Diffraction_Long_Double
        );

        output = PyArray_SimpleNewFromData(1, &dim, NPY_CLONGDOUBLE,
                                           (void *)T_hat);
        capsule = PyCapsule_New(T_hat, NULL, capsule_cleanup);
    }
    else {
        complex double *T_hat;
        T_hat = (complex double *)malloc(dim*sizeof(complex double));

        if (typenum == NPY_DOUBLE){
            _get_complex_from_three_real(
                ((double *)data), a, F, T_hat, dim,
                Left_Straightedge_Diffraction_Double
            );
        }
        else if (typenum == NPY_BYTE){
            _get_complex_from_three_real(
                ((char *)data), a, F, T_hat, dim,
                Left_Straightedge_Diffraction_Char
            );
        }
        else if (typenum == NPY_UBYTE){
            _get_complex_from_three_real(
                ((unsigned char *)data), a, F, T_hat, dim,
                Left_Straightedge_Diffraction_UChar
            );
        }
        else if (typenum == NPY_SHORT){
            _get_complex_from_three_real(
                ((short *)data), a, F, T_hat, dim,
                Left_Straightedge_Diffraction_Short
            );
        }
        else if (typenum == NPY_USHORT){
            _get_complex_from_three_real(
                ((unsigned short *)data), a, F, T_hat, dim,
                Left_Straightedge_Diffraction_UShort
            );
        }
        else if (typenum == NPY_INT){
            _get_complex_from_three_real(
                ((int *)data), a, F, T_hat, dim,
                Left_Straightedge_Diffraction_Int
            );
        }
        else if (typenum == NPY_UINT){
            _get_complex_from_three_real(
                ((unsigned int *)data), a, F, T_hat, dim,
                Left_Straightedge_Diffraction_UInt);
        }
        else if (typenum == NPY_LONG){
            _get_complex_from_three_real(
                ((long *)data), a, F, T_hat, dim,
                Left_Straightedge_Diffraction_Long
            );
        }
        else if (typenum == NPY_ULONG){
            _get_complex_from_three_real(
                ((unsigned long *)data), a, F, T_hat, dim,
                Left_Straightedge_Diffraction_ULong
            );
        }
        else if (typenum == NPY_LONGLONG){
            _get_complex_from_three_real(
                ((long long *)data), a, F, T_hat, dim,
                Left_Straightedge_Diffraction_Long_Long);
        }
        else if (typenum == NPY_ULONG){
            _get_complex_from_three_real(
                ((unsigned long long *)data), a, F, T_hat, dim,
                Left_Straightedge_Diffraction_ULong_Long);
        }
        else {
            PyErr_Format(
                PyExc_TypeError,
                "\n\rError Encountered: rss_ringoccs\n"
                "\r\tdiffrec.special_functions.left_straightedge\n\n"
                "\rInvalid data type for input numpy array. Input should be\n"
                "\ra one dimensional numpy array of positive (floating point)\n"
                "\rreal numbers."
            );
            return NULL;
        }

        output = PyArray_SimpleNewFromData(1, &dim, NPY_CDOUBLE, (void *)T_hat);
        capsule = PyCapsule_New(T_hat, NULL, capsule_cleanup);
    }

    /*  This frees the variable at the Python level once it's destroyed.      */
    PyArray_SetBaseObject((PyArrayObject *)output, capsule);

    /*  Return the results to Python.                                         */
    return Py_BuildValue("N", output);
}

static PyObject *square_wave_diffraction(PyObject *self, PyObject *args)
{
    PyObject *output, *capsule;
    PyArrayObject *x_arr;
    double W, F;
    long N;
    char typenum;
    long dim;
    void *data;
    complex double *T_hat;

    if (!PyArg_ParseTuple(args, "O!ddK", &PyArray_Type, &x_arr, &W, &F, &N)){
        PyErr_Format(
            PyExc_TypeError,
            "\n\rError Encountered: rss_ringoccs\n"
            "\r\tdiffrec.special_functions.square_wave_diffraction\n\n"
            "\rCould not parse inputs. Legal inputs are:\n"
            "\r\tx:     Numpy Array of positive real numbers (Floats)\n"
            "\r\tW:     Positive constant (Float)\n"
            "\r\tF:     Positive constant (Float)\n"
            "\r\tN:     Positive Integer (Int)\n\n"
            "\rNotes:\n"
            "\r\trho must be a non-empty one dimensional numpy array."
        );
        return NULL;
    }

    /*  Useful information about the data.                                */
    typenum = (char)PyArray_TYPE(x_arr);
    dim     = PyArray_DIMS(x_arr)[0];
    data    = PyArray_DATA(x_arr);

    /*  Check the inputs to make sure they're valid.                          */
    if (PyArray_NDIM(x_arr) != 1){
        PyErr_Format(
            PyExc_TypeError,
            "\n\rError Encountered: rss_ringoccs\n"
            "\r\tdiffrec.special_functions.square_wave_diffraction\n\n"
            "\rInput numpy array is not one-dimensional.\n"
        );
        return NULL;
    }
    else if (W <= 0.0){
        PyErr_Format(
            PyExc_TypeError,
            "\n\rError Encountered: rss_ringoccs\n"
            "\r\tdiffrec.special_functions.square_wave_diffraction\n\n"
            "\rWidth of wave is non-positive. (i.e. W<=0)\n"
        );
        return NULL;
    }
    else if (F <= 0.0){
        PyErr_Format(
            PyExc_TypeError,
            "\n\rError Encountered: rss_ringoccs\n"
            "\r\tdiffrec.special_functions.square_wave_diffraction\n\n"
            "\rFresnel scale is non-positive (i.e. F<=0).\n"
        );
        return NULL;
    }
    else if (dim == 0){
        PyErr_Format(
            PyExc_TypeError,
            "\n\rError Encountered: rss_ringoccs\n"
            "\r\tdiffrec.special_functions.square_wave_diffraction\n\n"
            "\rInput numpy array is empty.\n"
        );
    }

    T_hat = (complex double *)malloc(dim*sizeof(complex double));
    if (typenum == NPY_DOUBLE){
            _get_complex_from_four_real(
                ((double *)data), W, F, N, T_hat, dim,
                Square_Wave_Diffraction_Double
            );
        }
    else {
        PyErr_Format(
            PyExc_TypeError,
            "\n\rError Encountered: rss_ringoccs\n"
            "\r\tdiffrec.special_functions.square_wave_diffraction\n\n"
            "\rInvalid data type for input numpy array. Input should be\n"
            "\ra one dimensional numpy array of positive (floating point)\n"
            "\rreal numbers."
        );
        return NULL;
    }

    output = PyArray_SimpleNewFromData(1, &dim, NPY_CDOUBLE, (void *)T_hat);
    capsule = PyCapsule_New(T_hat, NULL, capsule_cleanup);

    /*  This frees the variable at the Python level once it's destroyed.      */
    PyArray_SetBaseObject((PyArrayObject *)output, capsule);

    /*  Return the results to Python.                                         */
    return Py_BuildValue("N", output);
}

/******************************************************************************
 *  Function:                                                                 *
 *      where_greater                                                         *
 *  Purpose:                                                                  *
 *      Given a numpy array arr, and a real number threshold, returns an      *
 *      array of alll indices n such that arr[n] > threshold.                 *
 *  Arguments:                                                                *
 *      arr (Numpy Array):                                                    *
 *          An arbitrary numpy array of real numbers.                         *
 *      threshold (double):                                                   *
 *          A real valued number. This is the value such that, for all n such *
 *          that arr[n] > threshold, the index n will be appended to the      *
 *          returning array.                                                  *
 *  Output:                                                                   *
 *      where_arr:                                                            *
 *          A numpy array of integer corresponding to the indices n such that *
 *          arr[n] > threshold.                                               *
 *  NOTES:                                                                    *
 *      1.) Values that are equal to threshold will not be included.          *
 *      2.) The input array MUST be one-dimensional.                          *
 *      3.) integer and floating point types are allowed, but complex are not.*
 ******************************************************************************/
static PyObject *where_greater(PyObject *self, PyObject *args)
{
    /*  Declare necessary variables.                                          */
    PyObject *output, *capsule;
    PyArrayObject *arr;
    double threshold;
    long **where;

    /*  The input is a numpy array, proceed. Otherwise spit out an error.     */
    if (PyArg_ParseTuple(args, "O!d", &PyArray_Type, &arr, &threshold)){
        
        /*  Declare some more necessary variables.                            */
        npy_int typenum, dim;
        void *data;

        /*  Check to make sure input is one dimensional.                      */
        if (PyArray_NDIM(arr) != 1){
            PyErr_Format(
                PyExc_TypeError,
                "rss_ringoccs.diffrec.special_functions.where_greater\n"
                "\r\tInput must be a one-dimensional array and a real number."
            );
            return NULL;
        }

        /*   Useful information about the data.                               */
        typenum = PyArray_TYPE(arr);
        dim     = PyArray_DIMS(arr)[0];
        data    = PyArray_DATA(arr);

        /*  Compute the index array corresponding to the indices n such that  *
         *  arr[n] > threshold. The returned pointer is created using malloc  *
         *  so we must remember to free it later to avoid memory leaks.       */
        switch(typenum)
        {
            case NPY_BYTE:
                where = Where_Greater_Char((char *)data, dim, threshold);
                break;
            case NPY_UBYTE:
                where =
                Where_Greater_UChar((unsigned char *)data, dim, threshold);
                break;
            case NPY_SHORT:
                where = Where_Greater_Short((short *)data, dim, threshold);
                break;
            case NPY_USHORT:
                where =
                Where_Greater_UShort((unsigned short *)data, dim, threshold);
                break;
            case NPY_INT:
                where = Where_Greater_Int((int *)data, dim, threshold);
                break;
            case NPY_UINT:
                where =
                Where_Greater_UInt((unsigned int *)data, dim, threshold);
                break;
            case NPY_LONG:
                where = Where_Greater_Long((long *)data, dim, threshold);
                break;
            case NPY_ULONG:
                where =
                Where_Greater_ULong((unsigned long *)data, dim, threshold);
                break;
            case NPY_LONGLONG:
                where =
                Where_Greater_Long_Long((long long *)data, dim, threshold);
                break;
            case NPY_ULONGLONG:
                where = Where_Greater_ULong_Long((unsigned long long *)data,
                                                 dim, threshold);
                break;
            case NPY_FLOAT:
                where = Where_Greater_Float((float *)data, dim, threshold);
                break;
            case NPY_DOUBLE:
                where = Where_Greater_Double((double *)data, dim, threshold);
                break;
            case NPY_LONGDOUBLE:
                where =
                Where_Greater_Long_Double((long double *)data, dim, threshold);
                break;
            default:
                PyErr_Format(
                    PyExc_TypeError,
                    "\n\r\trss_ringoccs.diffrec.math_functions.where_greater\n"
                    "\r\t\tInput numpy array should be real valued."
                );
                return NULL;
        }

        /*  The first element of where is the array of indices.               */
        long *where_arr = where[0];

        /*  The second element is the number of points in the index array.    */
        long where_dim = *where[1];

        /*  Create a Numpy array object to be passed back to Python.          */
        output =
        PyArray_SimpleNewFromData(1, &where_dim, NPY_LONG, (void *)where_arr);

        /*  This frees the variable at the Python level once it's destroyed.  */
        capsule = PyCapsule_New(where_arr, NULL, capsule_cleanup);
        PyArray_SetBaseObject((PyArrayObject *)output, capsule);

        /*  Where is created using malloc, so make sure to free it.           */
        free(where);

        /*  Return the results to Python.                                     */
        return Py_BuildValue("N", output);
    }
    else {
        /*  If he input is not a numpy array, return to Python with an error. */
        PyErr_Format(
            PyExc_TypeError,
            "\n\r\trss_ringoccs.diffrec.special_functions.where_greater\n"
            "\r\t\tInput should be a real numpy array and a real number."
        );
        return NULL;
    }
}

/******************************************************************************
 *  Function:                                                                 *
 *      where_lesser                                                          *
 *  Purpose:                                                                  *
 *      Given a numpy array arr, and a real number threshold, returns an      *
 *      array of all indices n such that arr[n] < threshold.                  *
 *  Arguments:                                                                *
 *      arr (Numpy Array):                                                    *
 *          An arbitrary numpy array of real numbers.                         *
 *      threshold (double):                                                   *
 *          A real valued number. This is the value such that, for all n such *
 *          that arr[n] < threshold, the index n will be appended to the      *
 *          returning array.                                                  *
 *  Output:                                                                   *
 *      where_arr:                                                            *
 *          A numpy array of integer corresponding to the indices n such that *
 *          arr[n] < threshold.                                               *
 *  NOTES:                                                                    *
 *      1.) Values that are equal to threshold will not be included.          *
 *      2.) The input array MUST be one-dimensional.                          *
 *      3.) integer and floating point types are allowed, but complex are not.*
 ******************************************************************************/
static PyObject *where_lesser(PyObject *self, PyObject *args)
{
    /*  Declare necessary variables.                                          */
    PyObject *output, *capsule;
    PyArrayObject *arr;
    double threshold;
    long **where;

    /*  The input is a numpy array, proceed. Otherwise spit out an error.     */
    if (PyArg_ParseTuple(args, "O!d", &PyArray_Type, &arr, &threshold)){

        /*  Check to make sure input is one dimensional.                      */
        if (PyArray_NDIM(arr) != 1){
            PyErr_Format(
                PyExc_TypeError,
                "rss_ringoccs.diffrec.special_functions.where_greater\n"
                "\r\tInput must be a one-dimensional array and a real number."
            );
            return NULL;
        }

        /*   Useful information about the data.                               */
        npy_int typenum = PyArray_TYPE(arr);
        npy_int dim     = PyArray_DIMS(arr)[0];

        /*  Compute the index array corresponding to the indices n such that  *
         *  arr[n] > threshold. The returned pointer is created using malloc  *
         *  so we must remember to free it later to avoid memory leaks.       */
        if (typenum == NPY_BYTE){
            char *data = (char *)PyArray_DATA(arr);
            where = Where_Lesser_Char(data, dim, threshold);
        }
        else if (typenum == NPY_UBYTE){
            unsigned char *data = (unsigned char *)PyArray_DATA(arr);
            where = Where_Lesser_UChar(data, dim, threshold);
        }
        else if (typenum == NPY_SHORT){
            short *data = (short *)PyArray_DATA(arr);
            where = Where_Lesser_Short(data, dim, threshold);
        }
        else if (typenum == NPY_USHORT){
            unsigned short *data = (unsigned short *)PyArray_DATA(arr);
            where = Where_Lesser_UShort(data, dim, threshold);
        }
        else if (typenum == NPY_INT){
            int *data = (int *)PyArray_DATA(arr);
            where = Where_Lesser_Int(data, dim, threshold);
        }
        else if (typenum == NPY_UINT){
            unsigned int *data = (unsigned int *)PyArray_DATA(arr);
            where = Where_Lesser_UInt(data, dim, threshold);
        }
        else if (typenum == NPY_LONG){
            long *data = (long *)PyArray_DATA(arr);
            where = Where_Lesser_Long(data, dim, threshold);
        }
        else if (typenum == NPY_ULONG){
            unsigned long *data = (unsigned long *)PyArray_DATA(arr);
            where = Where_Lesser_ULong(data, dim, threshold);
        }
        else if (typenum == NPY_LONGLONG){
            long long *data = (long long *)PyArray_DATA(arr);
            where = Where_Lesser_Long_Long(data, dim, threshold);
        }
        else if (typenum == NPY_ULONG){
            unsigned long long *data = (unsigned long long *)PyArray_DATA(arr);
            where = Where_Lesser_ULong_Long(data, dim, threshold);
        }
        else if (typenum == NPY_FLOAT){
            float *data = (float *)PyArray_DATA(arr);
            where = Where_Lesser_Float(data, dim, threshold);
        }
        else if (typenum == NPY_DOUBLE){
            double *data = (double *)PyArray_DATA(arr);
            where = Where_Lesser_Double(data, dim, threshold);
        }
        else if (typenum == NPY_LONGDOUBLE){
            long double *data = (long double *)PyArray_DATA(arr);
            where = Where_Lesser_Long_Double(data, dim, threshold);
        }
        else {
            /*  If he input is not a numpy array, return to Python with an error. */
            PyErr_Format(
                PyExc_TypeError,
                "\n\r\trss_ringoccs.diffrec.math_functions.where_greater\n"
                "\r\t\tInput numpy array should be real valued."
            );
            return NULL;
        }

        /*  The first element of where is the array of indices.               */
        long *where_arr = where[0];

        /*  The second element is the number of points in the index array.    */
        long where_dim = *where[1];

        /*  Create a Numpy array object to be passed back to Python.          */
        output  = PyArray_SimpleNewFromData(1, &where_dim, NPY_LONG,
                                            (void *)where_arr);

        /*  This frees the variable at the Python level once it's destroyed.  */
        capsule = PyCapsule_New(where_arr, NULL, capsule_cleanup);
        PyArray_SetBaseObject((PyArrayObject *)output, capsule);

        /*  Where is created using malloc, so make sure to free it.           */
        free(where);

        /*  Return the results to Python.                                     */
        return Py_BuildValue("N", output);
    }
    else {
        /*  If he input is not a numpy array, return to Python with an error. */
        PyErr_Format(
            PyExc_TypeError,
            "\n\r\trss_ringoccs.diffrec.math_functions.where_greater\n"
            "\r\t\tInput should be a numpy array of numbers and a real number."
        );
        return NULL;
    }
}

static PyObject *kbal(PyObject *self, PyObject *args)
{
    PyObject *output, *capsule;
    PyArrayObject *x;
    double dx, alpha;
    char typenum;
    long dim;
    void *data;

    if (!PyArg_ParseTuple(args, "O!dd", &PyArray_Type, &x, &dx, &alpha))
    {
        PyErr_Format(
            PyExc_TypeError,
            "\n\rError Encountered: rss_ringoccs\n"
            "\r\tdiffrec.special_functions.kbal\n\n"
            "\rCould not parse inputs. Legal inputs are:\n"
            "\r\tx:     Numpy Array of real numbers (Floats)\n"
            "\r\tdx:    Positive real number (Float)\n"
            "\rNotes:\n"
            "\r\tx must be a non-empty one dimensional numpy array."
        );
        return NULL;
    }

    /*  Useful information about the data.                                */
    typenum = (char)PyArray_TYPE(x);
    dim     = PyArray_DIMS(x)[0];
    data    = PyArray_DATA(x);

    /*  Check the inputs to make sure they're valid.                          */
    if (PyArray_NDIM(x) != 1)
    {
        PyErr_Format(
            PyExc_TypeError,
            "\n\rError Encountered: rss_ringoccs\n"
            "\r\tdiffrec.special_functions.kbal\n\n"
            "\rInput numpy array is not one-dimensional.\n"
        );
        return NULL;
    }
    else if (dim == 0)
    {
        PyErr_Format(
            PyExc_TypeError,
            "\n\rError Encountered: rss_ringoccs\n"
            "\r\tdiffrec.special_functions.kbal\n\n"
            "\rInput numpy array is empty.\n"
        );
        return NULL;
    }

    /*  Check that dx is positive.                                            */
    if (dx <= 0)
    {
        PyErr_Format(
            PyExc_ValueError,
            "\n\rError Encountered: rss_ringoccs\n"
            "\r\tdiffrec.special_functions.kbal\n\n"
            "\rdx must be a positive number.\n"
        );
        return NULL;
    }
    else if (alpha < 0)
    {
        PyErr_Format(
            PyExc_ValueError,
            "\n\rError Encountered: rss_ringoccs\n"
            "\r\tdiffrec.special_functions.kbal\n\n"
            "\ralpha must be a non-negative number (float).\n"
        );
        return NULL;
    }

    if (typenum == NPY_FLOAT)
    {
        float *y;
        y = (float *)malloc(dim*sizeof(float));
        _get_one_real_from_three_real(((float *)data), dx, alpha, y, dim,
                                     Kaiser_Bessel_Al_Float);
        output = PyArray_SimpleNewFromData(1, &dim, NPY_FLOAT, (void *)y);
        capsule = PyCapsule_New(y, NULL, capsule_cleanup);
    }
    else if (typenum == NPY_LONGDOUBLE)
    {
        long double *y;
        y = (long double *)malloc(dim*sizeof(long double));
        _get_one_real_from_three_real(((long double *)data), dx, alpha, y, dim,
                                     Kaiser_Bessel_Al_Long_Double);
        output = PyArray_SimpleNewFromData(1, &dim, NPY_LONGDOUBLE, (void *)y);
        capsule = PyCapsule_New(y, NULL, capsule_cleanup);
    }
    else
    {
        double *y;
        y = (double *)malloc(dim*sizeof(double));

        if (typenum == NPY_DOUBLE)
        {
            _get_one_real_from_three_real(((double *)data), dx, alpha, y, dim,
                                           Kaiser_Bessel_Al_Double);
        }
        else if (typenum == NPY_BYTE)
        {
            _get_one_real_from_three_real(((char *)data), dx, alpha, y, dim,
                                           Kaiser_Bessel_Al_Char);
        }
        else if (typenum == NPY_UBYTE)
        {
            _get_one_real_from_three_real(((unsigned char *)data), dx, alpha,
                                           y, dim, Kaiser_Bessel_Al_UChar);
        }
        else if (typenum == NPY_SHORT)
        {
            _get_one_real_from_three_real(((short *)data), dx, alpha, y, dim,
                                           Kaiser_Bessel_Al_Short);
        }
        else if (typenum == NPY_USHORT)
        {
            _get_one_real_from_three_real(((unsigned short *)data), dx, alpha, y,
                                           dim, Kaiser_Bessel_Al_UShort);
        }
        else if (typenum == NPY_INT)
        {
            _get_one_real_from_three_real(((int *)data), dx, alpha, y, dim,
                                           Kaiser_Bessel_Al_Int);
        }
        else if (typenum == NPY_UINT)
        {
            _get_one_real_from_three_real(((unsigned int *)data), dx, alpha, y,
                                           dim, Kaiser_Bessel_Al_UInt);
        }
        else if (typenum == NPY_LONG)
        {
            _get_one_real_from_three_real(((long *)data), dx, alpha, y, dim,
                                           Kaiser_Bessel_Al_Long);
        }
        else if (typenum == NPY_ULONG)
        {
            _get_one_real_from_three_real(((unsigned long *)data), dx, alpha, y,
                                           dim, Kaiser_Bessel_Al_ULong);
        }
        else if (typenum == NPY_LONGLONG)
        {
            _get_one_real_from_three_real(((long long *)data), dx, alpha, y, dim,
                                           Kaiser_Bessel_Al_Long_Long);
        }
        else if (typenum == NPY_ULONG)
        {
            _get_one_real_from_three_real(((unsigned long long *)data), dx,
                                           alpha, y, dim,
                                           Kaiser_Bessel_Al_ULong_Long);
        }
        else
        {
            PyErr_Format(
                PyExc_TypeError,
                "\n\rError Encountered: rss_ringoccs\n"
                "\r\tdiffrec.special_functions.kbal\n\n"
                "\rInvalid data type for input numpy array. Input should be\n"
                "\ra one dimensional numpy array of real numbers (float).\n"
            );
            return NULL;
        }

        output = PyArray_SimpleNewFromData(1, &dim, NPY_DOUBLE, (void *)y);
        capsule = PyCapsule_New(y, NULL, capsule_cleanup);
    }

    /*  This frees the variable at the Python level once it's destroyed.      */
    PyArray_SetBaseObject((PyArrayObject *)output, capsule);

    /*  Return the results to Python.                                         */
    return Py_BuildValue("N", output);
}

static PyObject *kbmdal(PyObject *self, PyObject *args)
{
    PyObject *output, *capsule;
    PyArrayObject *x;
    double dx, alpha;
    char typenum;
    long dim;
    void *data;

    if (!PyArg_ParseTuple(args, "O!dd", &PyArray_Type, &x, &dx, &alpha))
    {
        PyErr_Format(
            PyExc_TypeError,
            "\n\rError Encountered: rss_ringoccs\n"
            "\r\tdiffrec.special_functions.kbmdal\n\n"
            "\rCould not parse inputs. Legal inputs are:\n"
            "\r\tx:     Numpy Array of real numbers (Floats)\n"
            "\r\tdx:    Positive real number (Float)\n"
            "\rNotes:\n"
            "\r\tx must be a non-empty one dimensional numpy array."
        );
        return NULL;
    }

    /*  Useful information about the data.                                */
    typenum = (char)PyArray_TYPE(x);
    dim     = PyArray_DIMS(x)[0];
    data    = PyArray_DATA(x);

    /*  Check the inputs to make sure they're valid.                          */
    if (PyArray_NDIM(x) != 1)
    {
        PyErr_Format(
            PyExc_TypeError,
            "\n\rError Encountered: rss_ringoccs\n"
            "\r\tdiffrec.special_functions.kbmdal\n\n"
            "\rInput numpy array is not one-dimensional.\n"
        );
        return NULL;
    }
    else if (dim == 0)
    {
        PyErr_Format(
            PyExc_TypeError,
            "\n\rError Encountered: rss_ringoccs\n"
            "\r\tdiffrec.special_functions.kbmdal\n\n"
            "\rInput numpy array is empty.\n"
        );
        return NULL;
    }

    /*  Check that dx is positive.                                            */
    if (dx <= 0)
    {
        PyErr_Format(
            PyExc_ValueError,
            "\n\rError Encountered: rss_ringoccs\n"
            "\r\tdiffrec.special_functions.kbmdal\n\n"
            "\rdx must be a positive number.\n"
        );
        return NULL;
    }
    else if (alpha < 0)
    {
        PyErr_Format(
            PyExc_ValueError,
            "\n\rError Encountered: rss_ringoccs\n"
            "\r\tdiffrec.special_functions.kbmdal\n\n"
            "\ralpha must be a non-negative number (float).\n"
        );
        return NULL;
    }

    if (typenum == NPY_FLOAT)
    {
        float *y;
        y = (float *)malloc(dim*sizeof(float));
        _get_one_real_from_three_real(((float *)data), dx, alpha, y, dim,
                                     Modified_Kaiser_Bessel_Al_Float);
        output = PyArray_SimpleNewFromData(1, &dim, NPY_FLOAT, (void *)y);
        capsule = PyCapsule_New(y, NULL, capsule_cleanup);
    }
    else if (typenum == NPY_LONGDOUBLE)
    {
        long double *y;
        y = (long double *)malloc(dim*sizeof(long double));
        _get_one_real_from_three_real(((long double *)data), dx, alpha, y, dim,
                                     Modified_Kaiser_Bessel_Al_Long_Double);
        output = PyArray_SimpleNewFromData(1, &dim, NPY_LONGDOUBLE, (void *)y);
        capsule = PyCapsule_New(y, NULL, capsule_cleanup);
    }
    else
    {
        double *y;
        y = (double *)malloc(dim*sizeof(double));

        if (typenum == NPY_DOUBLE)
        {
            _get_one_real_from_three_real(((double *)data), dx, alpha, y, dim,
                                           Modified_Kaiser_Bessel_Al_Double);
        }
        else if (typenum == NPY_BYTE)
        {
            _get_one_real_from_three_real(((char *)data), dx, alpha, y, dim,
                                           Modified_Kaiser_Bessel_Al_Char);
        }
        else if (typenum == NPY_UBYTE)
        {
            _get_one_real_from_three_real(((unsigned char *)data), dx,
                                           alpha, y, dim,
                                           Modified_Kaiser_Bessel_Al_UChar);
        }
        else if (typenum == NPY_SHORT)
        {
            _get_one_real_from_three_real(((short *)data), dx, alpha, y, dim,
                                           Modified_Kaiser_Bessel_Al_Short);
        }
        else if (typenum == NPY_USHORT)
        {
            _get_one_real_from_three_real(((unsigned short *)data), dx,
                                           alpha, y, dim,
                                           Modified_Kaiser_Bessel_Al_UShort);
        }
        else if (typenum == NPY_INT)
        {
            _get_one_real_from_three_real(((int *)data), dx, alpha, y, dim,
                                           Modified_Kaiser_Bessel_Al_Int);
        }
        else if (typenum == NPY_UINT)
        {
            _get_one_real_from_three_real(((unsigned int *)data), dx, alpha, y,
                                           dim, Modified_Kaiser_Bessel_Al_UInt);
        }
        else if (typenum == NPY_LONG)
        {
            _get_one_real_from_three_real(((long *)data), dx, alpha, y, dim,
                                           Modified_Kaiser_Bessel_Al_Long);
        }
        else if (typenum == NPY_ULONG)
        {
            _get_one_real_from_three_real(((unsigned long *)data), dx,
                                           alpha, y, dim,
                                           Modified_Kaiser_Bessel_Al_ULong);
        }
        else if (typenum == NPY_LONGLONG)
        {
            _get_one_real_from_three_real(((long long *)data), dx, alpha, y, dim,
                                           Modified_Kaiser_Bessel_Al_Long_Long);
        }
        else if (typenum == NPY_ULONG)
        {
            _get_one_real_from_three_real(((unsigned long long *)data), dx,
                                           alpha, y, dim,
                                           Modified_Kaiser_Bessel_Al_ULong_Long);
        }
        else
        {
            PyErr_Format(
                PyExc_TypeError,
                "\n\rError Encountered: rss_ringoccs\n"
                "\r\tdiffrec.special_functions.kbmdal\n\n"
                "\rInvalid data type for input numpy array. Input should be\n"
                "\ra one dimensional numpy array of real numbers (float).\n"
            );
            return NULL;
        }

        output = PyArray_SimpleNewFromData(1, &dim, NPY_DOUBLE, (void *)y);
        capsule = PyCapsule_New(y, NULL, capsule_cleanup);
    }

    /*  This frees the variable at the Python level once it's destroyed.      */
    PyArray_SetBaseObject((PyArrayObject *)output, capsule);

    /*  Return the results to Python.                                         */
    return Py_BuildValue("N", output);
}

static PyObject *max(PyObject *self, PyObject *args)
{
    PyArrayObject *arr;
    PyObject *tuple = PyTuple_GetItem(args, 0);

    if (PyLong_Check(tuple)){
        long max;
        if (PyArg_ParseTuple(args, "l", &max)){
            return PyLong_FromLong(max);
        }
        else {
            PyErr_Format(PyExc_TypeError,
                         "\n\r\trss_ringoccs.diffrec.special_functions.max\n"
                         "\r\t\tCould not parse int type input.");
            return NULL;
        }
    }
    else if (PyFloat_Check(tuple)){
        double max;
        if(PyArg_ParseTuple(args, "d", &max)){
            return PyFloat_FromDouble(max);
        }
        else {
            PyErr_Format(PyExc_TypeError,
                         "\n\r\trss_ringoccs.diffrec.special_functions.max\n"
                         "\r\t\tCould not parse float type input.");
            return NULL;
        }
    }
    else if (PyArg_ParseTuple(args, "O!", &PyArray_Type, &arr)){
        npy_int typenum, dim;
        void *data;

        // Check to make sure input isn't zero dimensional!
        if (PyArray_NDIM(arr) != 1) goto FAIL;

        // Useful information about the data.
        typenum = PyArray_TYPE(arr);
        dim     = PyArray_DIMS(arr)[0];
        data    = PyArray_DATA(arr);

        if (dim == 0) goto FAIL;

        switch(typenum)
        {
            case NPY_FLOAT:
                return PyFloat_FromDouble(Max_Float((float *)data, dim));
            case NPY_DOUBLE:
                return PyFloat_FromDouble(Max_Double((double *)data, dim));
            case NPY_LONGDOUBLE:
                return
                PyFloat_FromDouble(Max_Long_Double((long double *)data, dim));
            case NPY_SHORT:
                return PyLong_FromLong(Max_Short((short *)data, dim));
            case NPY_INT:
                return PyLong_FromLong(Max_Int((int *)data, dim));
            case NPY_LONG:
                return PyLong_FromLong(Max_Long((long *)data, dim));
            case NPY_LONGLONG:
                return PyLong_FromLong(Max_Long_Long((long long *)data, dim));
            default:
                goto FAIL;
        }
    }
    else goto FAIL;
    FAIL: {
        PyErr_Format(PyExc_TypeError,
                     "\n\r\trss_ringoccs.diffrec.special_functions.max\n"
                     "\r\t\tInput should be a one dimensional numpy array of\n"
                     "\r\t\treal numbers, or a float/int number.\n"
                     "\r\t\tExample:\n"
                     "\r\t\t\t>>> import numpy\n"
                     "\r\t\t\t>>> import _special_functions as sf\n"
                     "\r\t\t\t>>> x = numpy.random.rand(100)\n"
                     "\r\t\t\t>>> y = sf.max(x)\n\n"
                     "\r\t\tNOTE:\n"
                     "\r\t\t\tOnly one dimensional numpy arrays are allowed.\n"
                     "\r\t\t\tComplex numbers are not allowed. If the input\n"
                     "\r\t\t\tis a single floating point or integer number,\n"
                     "\r\t\t\tthe output will simply be that number.");
        return NULL;
    };
}

static PyObject *min(PyObject *self, PyObject *args)
{
    PyArrayObject *arr;
    PyObject *tuple = PyTuple_GetItem(args, 0);

    if (PyLong_Check(tuple)){
        long min;
        if (PyArg_ParseTuple(args, "l", &min)){
            return PyLong_FromLong(min);
        }
        else {
            PyErr_Format(PyExc_TypeError,
                         "\n\r\trss_ringoccs.diffrec.math_functions.min\n"
                         "\r\t\tCould not parse int type input.");
            return NULL;
        }
    }
    else if (PyFloat_Check(tuple)){
        double min;
        if(PyArg_ParseTuple(args, "d", &min)){
            return PyFloat_FromDouble(min);
        }
        else {
            PyErr_Format(PyExc_TypeError,
                         "\n\r\trss_ringoccs.diffrec.math_functions.min\n"
                         "\r\t\tCould not parse float type input.");
            return NULL;
        }
    }
    else if (PyArg_ParseTuple(args, "O!", &PyArray_Type, &arr)){
        npy_int typenum, dim;
        void *data;

        // Check to make sure input isn't zero dimensional!
        if (PyArray_NDIM(arr) != 1){
            PyErr_Format(PyExc_TypeError,
                         "\n\r\trss_ringoccs.diffrec.math_functions.min\n"
                         "\r\t\tInput must be one dimensional.");
            return NULL;
        }

        // Useful information about the data.
        typenum = PyArray_TYPE(arr);
        dim     = PyArray_DIMS(arr)[0];
        data    = PyArray_DATA(arr);

        if (dim == 0){
            PyErr_Format(PyExc_TypeError,
                         "\n\r\trss_ringoccs.diffrec.math_functions.min\n"
                         "\r\t\tInput is zero dimensional.");
            return NULL;
        }

        if (typenum == NPY_FLOAT){
            return PyFloat_FromDouble(
                Min_Float((float *)data, dim)
            );
        }
        else if (typenum == NPY_DOUBLE){
            return PyFloat_FromDouble(
                Min_Double((double *)data, dim)
            );
        }
        else if (typenum == NPY_LONGDOUBLE){
            return PyFloat_FromDouble(
                Min_Long_Double((long double *)data, dim)
            );
        }
        else if (typenum == NPY_SHORT){
            return PyLong_FromLong(
                Min_Short((short *)data, dim)
            );
        }
        else if (typenum == NPY_INT){
            return PyLong_FromLong(
                Min_Int((int *)data, dim)
            );
        }
        else if (typenum == NPY_LONG){
            return PyLong_FromLong(
                Min_Long((long *)data, dim)
            );
        }
        else if (typenum == NPY_LONGLONG){
            return PyLong_FromLong(
                Min_Long_Long((long long *)data, dim)
            );
        }
        else {
            PyErr_Format(PyExc_TypeError,
                         "\n\r\trss_ringoccs.diffrec.math_functions.min\n"
                         "\r\t\tInput should be a numpy array of numbers.");
            return NULL;
        }
    }
    else {
        PyErr_Format(PyExc_TypeError,
                     "rss_ringoccs.diffrec.math_functions.min\n"
                     "\n\r\trss_ringoccs.diffrec.math_functions.min\n"
                     "\r\t\tInput should be a numpy array of numbers.");
        return NULL;
    }
}

static PyObject *window_norm(PyObject *self, PyObject *args){
    PyArrayObject *arr;
    PyObject *tuple = PyTuple_GetItem(args, 0);
    double dx, f_scale;

    if (PyLong_Check(tuple)){
        long ker;
        PyArg_ParseTuple(args, "ldd", &ker, &dx, &f_scale);
        return PyFloat_FromDouble(
            Window_Normalization_Long(&ker, 1, dx, f_scale)
        );
    }
    else if (PyFloat_Check(tuple)){
        double ker;
        PyArg_ParseTuple(args, "ddd", &ker, &dx, &f_scale);
        return PyFloat_FromDouble(
            Window_Normalization_Double(&ker, 1, dx, f_scale)
        );
    }
    else if (PyArg_ParseTuple(args, "O!dd", &PyArray_Type,
                              &arr, &dx, &f_scale)){
        npy_int typenum, dim;
        void *data;

        // Check to make sure input isn't zero dimensional!
        if (PyArray_NDIM(arr) != 1){
            PyErr_Format(PyExc_TypeError,
                         "rss_ringoccs.diffrec.math_functions.min\n"
                         "\rInput must be a one-dimensional array.");
            return NULL;
        }

        // Useful information about the data.
        typenum = PyArray_TYPE(arr);
        dim     = PyArray_DIMS(arr)[0];
        data    = PyArray_DATA(arr);

        if (typenum == NPY_CFLOAT){
            return PyFloat_FromDouble(
                Window_Normalization_Complex_Float(
                    (complex float *)data, dim, dx, f_scale
                )
            );
        }
        else if (typenum == NPY_CDOUBLE){
            return PyFloat_FromDouble(
                Window_Normalization_Complex_Double(
                    (complex double *)data, dim, dx, f_scale
                )
            );
        }
        else if (typenum == NPY_CLONGDOUBLE){
            return PyFloat_FromDouble(
                Window_Normalization_Complex_Long_Double(
                    (complex long double *)data, dim, dx, f_scale
                )
            );
        }
        else if (typenum == NPY_FLOAT){
            return PyFloat_FromDouble(
                Window_Normalization_Float((float *)data, dim, dx, f_scale)
            );
        }
        else if (typenum == NPY_DOUBLE){
            return PyFloat_FromDouble(
                Window_Normalization_Double((double *)data, dim, dx, f_scale)
            );
        }
        else if (typenum == NPY_LONGDOUBLE){
            return PyFloat_FromDouble(
                Window_Normalization_Long_Double((long double *)data,
                                                 dim, dx, f_scale)
            );
        }
        else if (typenum == NPY_INT){
            return PyLong_FromLong(
                Window_Normalization_Int((int *)data, dim, dx, f_scale)
            );
        }
        else if (typenum == NPY_LONG){
            return PyLong_FromLong(
                Window_Normalization_Long((long *)data, dim, dx, f_scale)
            );
        }
        else {
            PyErr_Format(PyExc_TypeError,
                        "rss_ringoccs.diffrec.math_functions.min\n"
                        "\rInput should be a numpy array of real numbers"
                        "or a floating point/integer value.");
            return NULL;
        }
    }
    else {
        PyErr_Format(PyExc_TypeError,
                     "rss_ringoccs.diffrec.math_functions.min\n"
                     "\rInput should be a numpy array of numbers,"
                     "or a floating point/integer value.");
        return NULL;
    }  
}

static PyMethodDef _special_functions_methods[] =
{
    {
        "coss",
        coss,
        METH_VARARGS,
        "\r\t"
        "Function:\n\r\t\t"
        "special_functions.coss\n\r\t"
        "Purpose:\n\r\t\t"
        "Compute the squared cosine window function.\n\r\t"
        "Arguments\n\r\t\t"
        "x (numpy.ndarray):\n\r\t\t\t"
        "A numpy array of real numbers. Independent variable for coss(x)."
        "\n\r\t\tW (float):\n\r\t\t\t"
        "The width of the window function.\n\r\t"
        "Outputs:\n\r\t\t"
        "coss (numpy.ndarray):\n\r\t\t\t"
        "The squared cosine function of x.\n\r\t"
        "Example:\n\r\t\t"
        ">>> import numpy\n\r\t\t"
        ">>> import special_functions\n\r\t\t"
        ">>> x = numpy.arange(-20,20,0.1)\n\r\t\t"
        ">>> W = 10.0\n\r\t\t"
        ">>> y = special_functions.coss(x, W)"
    },
    {
        "rect",
        rect,
        METH_VARARGS,
        "\r\t"
        "Function:\n\r\t\t"
        "special_functions.rect\n\r\t"
        "Purpose:\n\r\t\t"
        "Compute the rectangular window function.\n\r\t"
        "Arguments\n\r\t\t"
        "x (numpy.ndarray):\n\r\t\t\t"
        "A numpy array of real numbers. Independent variable for rect(x)\n\r\t"
        "W (float):\n\r\t\t\t"
        "The width of the window function.\n\r\t"
        "Outputs:\n\r\t\t"
        "rect (numpy.ndarray):\n\r\t\t\t"
        "The rect function of x.\n\r\t"
        "Example:\n\r\t\t"
        ">>> import numpy\n\r\t\t"
        ">>> import special_functions\n\r\t\t"
        ">>> x = numpy.arange(-20,20,0.1)\n\r\t\t"
        ">>> W = 10.0"
        ">>> y = special_functions.rect(x, W)"
    },
    {
        "kb20",
        kb20,
        METH_VARARGS,
        "\r\t"
        "Function:\n\r\t\t"
        "special_functions.kb20\n\r\t"
        "Purpose:\n\r\t\t"
        "Compute the Kaiser-Bessel function with alpha = 2.0."
        "\n\r\tArguments\n\r\t\t"
        "x (numpy.ndarray):\n\r\t\t\t"
        "A numpy array of real numbers. Independent variable for kb20(x)\n\r\t"
        "W (float):\n\r\t\t\t"
        "The width of the Kaiser-Bessel window.\n\r\t"
        "Outputs:\n\r\t\t"
        "kb20 (numpy.ndarray):\n\r\t\t\t"
        "The kb20 function of x.\n\r\t"
        "Example:\n\r\t\t"
        ">>> import numpy\n\r\t\t"
        ">>> import special_functions\n\r\t\t"
        ">>> x = numpy.arange(-20,20,0.1)\n\r\t\t"
        ">>> W = 10.0"
        ">>> y = special_functions.kb20(x, W)"
    },
    {
        "kb25",
        kb25,
        METH_VARARGS,
        "\r\t"
        "Function:\n\r\t\t"
        "special_functions.kb25\n\r\t"
        "Purpose:\n\r\t\t"
        "Compute the Kaiser-Bessel function with alpha = 2.5."
        "\n\r\tArguments\n\r\t\t"
        "x (numpy.ndarray):\n\r\t\t\t"
        "A numpy array of real numbers. Independent variable for kb25(x)\n\r\t"
        "W (float):\n\r\t\t\t"
        "The width of the Kaiser-Bessel window.\n\r\t"
        "Outputs:\n\r\t\t"
        "kb25 (numpy.ndarray):\n\r\t\t\t"
        "The kb25 function of x.\n\r\t"
        "Example:\n\r\t\t"
        ">>> import numpy\n\r\t\t"
        ">>> import special_functions\n\r\t\t"
        ">>> x = numpy.arange(-20,20,0.1)\n\r\t\t"
        ">>> W = 10.0"
        ">>> y = special_functions.kb25(x, W)"
    },
    {
        "kb35",
        kb35,
        METH_VARARGS,
        "\r\t"
        "Function:\n\r\t\t"
        "special_functions.kb35\n\r\t"
        "Purpose:\n\r\t\t"
        "Compute the Kaiser-Bessel function with alpha = 3.5."
        "\n\r\tArguments\n\r\t\t"
        "x (numpy.ndarray):\n\r\t\t\t"
        "A numpy array of real numbers. Independent variable for kb35(x)\n\r\t"
        "W (float):\n\r\t\t\t"
        "The width of the Kaiser-Bessel window.\n\r\t"
        "Outputs:\n\r\t\t"
        "kb35 (numpy.ndarray):\n\r\t\t\t"
        "The kb35 function of x.\n\r\t"
        "Example:\n\r\t\t"
        ">>> import numpy\n\r\t\t"
        ">>> import special_functions\n\r\t\t"
        ">>> x = numpy.arange(-20,20,0.1)\n\r\t\t"
        ">>> W = 10.0"
        ">>> y = special_functions.kb35(x, W)"
    },
    {
        "kbal",
        kbal,
        METH_VARARGS,
        "\r\t"
        "Function:\n\r\t\t"
        "special_functions.kbal\n\r\t"
        "Purpose:\n\r\t\t"
        "Compute the Kaiser-Bessel function with arbitrary alpha."
        "\n\r\tArguments\n\r\t\t"
        "x (numpy.ndarray):\n\r\t\t\t"
        "A numpy array of real numbers. Independent variable for kbal(x)\n\r\t"
        "W (float):\n\r\t\t\t"
        "The width of the Kaiser-Bessel window.\n\r\t"
        "Outputs:\n\r\t\t"
        "kbal (numpy.ndarray):\n\r\t\t\t"
        "The kbal function of x.\n\r\t"
        "Example:\n\r\t\t"
        ">>> import numpy\n\r\t\t"
        ">>> import special_functions\n\r\t\t"
        ">>> x = numpy.arange(-20,20,0.1)\n\r\t\t"
        ">>> W = 10.0\n\r\t\t"
        ">>> alpha = 1.8\n\r\t\t"
        ">>> y = special_functions.kbal(x, W, alpha)"
    },
    {
        "kbmd20",
        kbmd20,
        METH_VARARGS,
        "\r\t"
        "Function:\n\r\t\t"
        "special_functions.kbmd20\n\r\t"
        "Purpose:\n\r\t\t"
        "Compute the Modified Kaiser-Bessel function with alpha = 2.0."
        "\n\r\tArguments\n\r\t\t"
        "x (numpy.ndarray):\n\r\t\t\t"
        "A numpy array of real numbers. Independent variable for kbmd20(x)"
        "\n\r\tW (float):\n\r\t\t\t"
        "The width of the Kaiser-Bessel window.\n\r\t"
        "Outputs:\n\r\t\t"
        "kbmd20 (numpy.ndarray):\n\r\t\t\t"
        "The kbmd20 function of x.\n\r\t"
        "Example:\n\r\t\t"
        ">>> import numpy\n\r\t\t"
        ">>> import special_functions\n\r\t\t"
        ">>> x = numpy.arange(-20,20,0.1)\n\r\t\t"
        ">>> W = 10.0"
        ">>> y = special_functions.kbmd20(x, W)"
    },
    {
        "kbmd25",
        kbmd25,
        METH_VARARGS,
        "\r\t"
        "Function:\n\r\t\t"
        "special_functions.kbmd25\n\r\t"
        "Purpose:\n\r\t\t"
        "Compute the Modified Kaiser-Bessel function with alpha = 2.5."
        "\n\r\tArguments\n\r\t\t"
        "x (numpy.ndarray):\n\r\t\t\t"
        "A numpy array of real numbers. Independent variable for kbmd25(x)"
        "\n\r\tW (float):\n\r\t\t\t"
        "The width of the Kaiser-Bessel window.\n\r\t"
        "Outputs:\n\r\t\t"
        "kbmd25 (numpy.ndarray):\n\r\t\t\t"
        "The kbmd25 function of x.\n\r\t"
        "Example:\n\r\t\t"
        ">>> import numpy\n\r\t\t"
        ">>> import special_functions\n\r\t\t"
        ">>> x = numpy.arange(-20,20,0.1)\n\r\t\t"
        ">>> W = 10.0"
        ">>> y = special_functions.kbmd25(x, W)"
    },
    {
        "kbmd35",
        kbmd35,
        METH_VARARGS,
        "\r\t"
        "Function:\n\r\t\t"
        "special_functions.kbmd35\n\r\t"
        "Purpose:\n\r\t\t"
        "Compute the Modified Kaiser-Bessel function with alpha = 3.5."
        "\n\r\tArguments\n\r\t\t"
        "x (numpy.ndarray):\n\r\t\t\t"
        "A numpy array of real numbers. Independent variable for kbmd35(x)"
        "\n\r\tW (float):\n\r\t\t\t"
        "The width of the Kaiser-Bessel window.\n\r\t"
        "Outputs:\n\r\t\t"
        "kbmd35 (numpy.ndarray):\n\r\t\t\t"
        "The kbmd35 function of x.\n\r\t"
        "Example:\n\r\t\t"
        ">>> import numpy\n\r\t\t"
        ">>> import special_functions\n\r\t\t"
        ">>> x = numpy.arange(-20,20,0.1)\n\r\t\t"
        ">>> W = 10.0"
        ">>> y = special_functions.kbmd35(x, W)"
    },
    {
        "kbmdal",
        kbmdal,
        METH_VARARGS,
        "\r\t"
        "Function:\n\r\t\t"
        "special_functions.kbmdal\n\r\t"
        "Purpose:\n\r\t\t"
        "Compute the Modified Kaiser-Bessel function with arbitrary alpha."
        "\n\r\tArguments\n\r\t\t"
        "x (numpy.ndarray):\n\r\t\t\t"
        "A numpy array of real numbers. Independent variable for kbmdal(x)"
        "\n\r\tW (float):\n\r\t\t\t"
        "The width of the Kaiser-Bessel window.\n\r\t"
        "Outputs:\n\r\t\t"
        "kbmdal (numpy.ndarray):\n\r\t\t\t"
        "The kbmdal function of x.\n\r\t"
        "Example:\n\r\t\t"
        ">>> import numpy\n\r\t\t"
        ">>> import special_functions\n\r\t\t"
        ">>> x = numpy.arange(-20,20,0.1)\n\r\t\t"
        ">>> W = 10.0"
        ">>> y = special_functions.kbmdal(x, W)"
    },
    {
        "besselJ0",
        besselJ0,
        METH_VARARGS,
        "\r\t"
        "Function:\n\r\t\t"
        "special_functions.besselJ0\n\r\t"
        "Purpose:\n\r\t\t"
        "Compute the Zeroth Bessel Function of the First Kind (J0).\n\r\t"
        "Arguments\n\r\t\t"
        "x (numpy.ndarray):\n\r\t\t\t"
        "A numpy array of real numbers. Independent variable for J_0(x)\n\r\t"
        "Outputs:\n\r\t\t"
        "J0\n\r\t\t\t"
        "The Bessel Function J0 as a function of x.\n\r\t"
        "Example:\n\r\t\t"
        ">>> import numpy\n\r\t\t"
        ">>> import special_functions\n\r\t\t"
        ">>> x = numpy.arange(0,100,0.01)\n\r\t\t"
        ">>> y = special_functions.besselJ0(x)"
    },
    {
        "besselI0",
        besselI0,
        METH_VARARGS,
        "\r\t"
        "Function:\n\r\t\t"
        "special_functions.besselI0\n\r\t"
        "Purpose:\n\r\t\t"
        "Compute the Zeroth Modified Bessel Function of the First Kind (I0)."
        "\n\r\tArguments\n\r\t\t"
        "x (numpy.ndarray):\n\r\t\t\t"
        "A numpy array of real numbers. Independent variable for I_0(x)\n\r\t"
        "Outputs:\n\r\t\t"
        "I0 (numpy.ndarray):\n\r\t\t\t"
        "The Bessel Function I0 as a function of x.\n\r\t"
        "Example:\n\r\t\t"
        ">>> import numpy\n\r\t\t"
        ">>> import special_functions\n\r\t\t"
        ">>> x = numpy.arange(0,100,0.01)\n\r\t\t"
        ">>> y = special_functions.besselI0(x)"
    },
    {
        "fresnel_sin",
        fresnel_sin,
        METH_VARARGS,
        "\r\t"
        "Function:\n\r\t\t"
        "special_functions.fresnel_sin\n\r\t"
        "Purpose:\n\r\t\t"
        "Compute the Fresnel sine function."
        "\n\r\tArguments\n\r\t\t"
        "x (numpy.ndarray):\n\r\t\t\t"
        "A numpy array of real numbers. Independent variable for fresnel_sin(x)"
        "\n\r\tOutputs:\n\r\t\t"
        "fresnel_sin (numpy.ndarray):\n\r\t\t\t"
        "The fresnel sine function of x.\n\r\t"
        "Example:\n\r\t\t"
        ">>> import numpy\n\r\t\t"
        ">>> import special_functions\n\r\t\t"
        ">>> x = numpy.arange(0, 10, 0.01)\n\r\t\t"
        ">>> y = special_functions.fresnel_sin(x)"
    },
    {
        "fresnel_cos",
        fresnel_cos,
        METH_VARARGS,
        "\r\t"
        "Function:\n\r\t\t"
        "special_functions.fresnel_cos\n\r\t"
        "Purpose:\n\r\t\t"
        "Compute the Fresnel cosine function."
        "\n\r\tArguments\n\r\t\t"
        "x (numpy.ndarray):\n\r\t\t\t"
        "A numpy array of real numbers. Independent variable for fresnel_cos(x)"
        "\n\r\tOutputs:\n\r\t\t"
        "fresnel_cos (numpy.ndarray):\n\r\t\t\t"
        "The fresnel sine function of x.\n\r\t"
        "Example:\n\r\t\t"
        ">>> import numpy\n\r\t\t"
        ">>> import special_functions\n\r\t\t"
        ">>> x = numpy.arange(0, 10, 0.01)\n\r\t\t"
        ">>> y = special_functions.fresnel_cos(x)"
    },
    {
        "lambertw",
        lambertw,
        METH_VARARGS,
        "\r\t"
        "Function:\n\r\t\t"
        "special_functions.lambertw\n\r\t"
        "Purpose:\n\r\t\t"
        "Compute the Lambert W function, inverse of x*exp(x)."
        "\n\r\tArguments\n\r\t\t"
        "x (numpy.ndarray):\n\r\t\t\t"
        "A numpy array of real numbers. Independent variable.\n\r\t"
        "Outputs:\n\r\t\t"
        "y (numpy.ndarray):\n\r\t\t\t"
        "The Lambert W function of x.\n\r\t"
        "Example:\n\r\t\t"
        ">>> import numpy\n\r\t\t"
        ">>> import special_functions\n\r\t\t"
        ">>> x = numpy.arange(0,100,0.01)\n\r\t\t"
        ">>> y = special_functions.lambertw(x)"
    },
    {
        "sinc",
        sinc,
        METH_VARARGS,
        "\r\t"
        "Function:\n\r\t\t"
        "special_functions.sinc\n\r\t"
        "Purpose:\n\r\t\t"
        "Compute the sinc function, sin(x)/x."
        "\n\r\tArguments\n\r\t\t"
        "x (numpy.ndarray):\n\r\t\t\t"
        "A numpy array of real numbers. Independent variable for sinc(x)\n\r\t"
        "Outputs:\n\r\t\t"
        "sinc (numpy.ndarray):\n\r\t\t\t"
        "The sinc function of x.\n\r\t"
        "Example:\n\r\t\t"
        ">>> import numpy\n\r\t\t"
        ">>> import special_functions\n\r\t\t"
        ">>> x = numpy.arange(0,100,0.01)\n\r\t\t"
        ">>> y = special_functions.sinc(x)"
    },
    {
        "compute_norm_eq",
        compute_norm_eq,
        METH_VARARGS,
        "\r\t"
        "Function:\n\r\t\t"
        "special_functions.compute_norm_eq\n\r\t"
        "Purpose:\n\r\t\t"
        "Compute normalized equivalenth width of a given function.\n\r\t"
        "Arguments:\n\r\t\t"
        "w_func (*numpy.ndarray*):\n\r\t\t\t"
        "Function to compute the normalized equivalent width.\n\r\t"
        "Outputs:\n\r\t\t"
        "normeq (*float*):\n\r\t\t\t"
        "The normalized equivalent width of w_func.\n\r\t"
        "Notes:\n\r\t\t"
        "The normalized equivalent width is computed using Riemann\n\r\t\t"
        "sums to approximate integrals. Therefore large dx values\n\r\t\t"
        "(Spacing between points) will result in an inaccurate\n\r\t\t"
        "normeq. One should keep this in mind during calculations.\n\r\t"
        "Examples:\n\r\t\t"
        "Compute the Kaiser-Bessel 2.5 window of width 20km and\n\r\t\t"
        "spacing 0.1 and compute the normalized equivalent width:\n\r\t\t\t"
        ">>> import special_functions\n\r\t\t\t"
        ">>> import numpy\n\r\t\t\t"
        ">>> x = numpy.arange(-10, 10, 0.1)\n\r\t\t\t"
        ">>> w = special_functions.kb25(x, 20)\n\r\t\t\t"
        ">>> special_functions.compute_norm_eq(w)\n\r\t\t\t"
        "1.651925635118099\n\r\t\t"
        "In contrast, the actual value is 1.6519208. Compute the\n\r\t\t"
        "normalized equivalent width for the squared cosine window of\n\r\t\t"
        "width 20 and spacing 0.25.\n\r\t\t\t"
        ">>> import special_functions\n\r\t\t\t"
        ">>> import numpy\n\r\t\t\t"
        ">>> x = numpy.arange(-10, 10, 0.25)\n\r\t\t\t"
        ">>> w = special_functions.kb25(x, 20)\n\r\t\t\t"
        ">>> special_functions.compute_norm_eq(w)\n\r\t\t\t"
        "1.5000000000000013\n\r\t\t"
        "The normalized equivalent width of the squared cosine\n\r\t\t"
        "function can be computed exactly using standard methods\n\r\t\t"
        "from a calculus course. It's value is exactly 1.5."
    },
    {
        "fresnel_transform",
        fresnel_transform,
        METH_VARARGS,
        "Compute the Fresnel Transform."
    },
    {
        "frequency_to_wavelength",
        frequency_to_wavelength,
        METH_VARARGS,
        "\r\t"
        "Function:\n\r\t\t"
        "special_functions.frequency_to_wavelength\n\r\t"
        "Purpose\n\r\t\t"
        "Convert a non-zero frequency to the equivalent wavelength.\n\r\t"
        "Arguments:\n\r\t\t"
        "frequency (numpy.ndarray/int/float/list):\n\r\t\t\t"
        "A numpy array or list of real numbers, input frequency in Hz.\n\r\t\t"
        "Outputs:\n\r\t\t"
        "wavelength (numpy.ndarry/list/float):\n\r\t\t\t"
        "The corresponding wavelength in km.\n\r\t"
        "Example:\n\r\t\t"
        ">>> import numpy\n\r\t\t"
        ">>> import special_functions\n\r\t\t"
        ">>> x = numpy.arange(1,10,0.1)"
        ">>> y = special_functions.frequency_to_wavelength(x)"
    },
    {
        "square_wave_diffraction",
        square_wave_diffraction,
        METH_VARARGS,
        "Compute the normalized equivalent width of an array."
    },
    {
        "gap_diffraction",
        gap_diffraction,
        METH_VARARGS,
        "\r\t"
        "Function:\n\r\t\t"
        "special_functions.gap_diffraction\n\r\t"
        "Purpose\n\r\t\t"
        "Compute the diffraction pattern of an annular gap in the plane.\n\r\t"
        "Arguments:\n\r\t\t"
        "rho (numpy.ndarray):\n\r\t\t\t"
        "A numpy array of positive real numbers.\n\r\t\t"
        "a (float)\n\r\t\t\t"
        "A positive real number, the inner radius of the annulus.\n\r\t\t"
        "b (float)\n\r\t\t\t"
        "A positive real number, the outter radius of the annulus.\n\r\t\t"
        "F (float)\n\r\t\t\t"
        "A positive real number, the Fresnel scale (same units as rho).\n\r\t"
        "Outputs:\n\r\t\t"
        "T_hat (numpy.ndarray):\n\r\t\t\t"
        "Numpy array of complex numbers equal to the diffraction pattern.\n\r\t"
        "Example:\n\r\t\t"
        ">>> import numpy\n\r\t\t"
        ">>> import special_functions\n\r\t\t"
        ">>> x = numpy.arange(0,100,0.01)\n\r\t\t"
        ">>> a = 45\n\r\t\t"
        ">>> b = 55\n\r\t\t"
        ">>> F = 0.05\n\r\t\t"
        ">>> y = special_functions.gap_diffraction(x, 45, 55, 0.05)"
    },
    {
        "ringlet_diffraction",
        ringlet_diffraction,
        METH_VARARGS,
        "\r\t"
        "Function:\n\r\t\t"
        "special_functions.gap_diffraction\n\r\t"
        "Purpose\n\r\t\t"
        "Compute the diffraction pattern of a ringlet in the plane.\n\r\t"
        "Arguments:\n\r\t\t"
        "rho (numpy.ndarray):\n\r\t\t\t"
        "A numpy array of positive real numbers.\n\r\t\t"
        "a (float)\n\r\t\t\t"
        "A positive real number, the inner radius of the ring.\n\r\t\t"
        "b (float)\n\r\t\t\t"
        "A positive real number, the outter radius of the ring.\n\r\t\t"
        "F (float)\n\r\t\t\t"
        "A positive real number, the Fresnel scale (same units as rho).\n\r\t"
        "Outputs:\n\r\t\t"
        "T_hat (numpy.ndarray):\n\r\t\t\t"
        "Numpy array of complex numbers equal to the diffraction pattern.\n\r\t"
        "Example:\n\r\t\t"
        ">>> import numpy\n\r\t\t"
        ">>> import special_functions\n\r\t\t"
        ">>> x = numpy.arange(0,100,0.01)\n\r\t\t"
        ">>> a = 45\n\r\t\t"
        ">>> b = 55\n\r\t\t"
        ">>> F = 0.05\n\r\t\t"
        ">>> y = special_functions.ringlet_diffraction(x, a, b, F)"
    },
    {
        "right_straightedge",
        right_straightedge,
        METH_VARARGS,
        "\r\t"
        "Function:\n\r\t\t"
        "special_functions.right_straightedge\n\r\t"
        "Purpose\n\r\t\t"
        "Compute the diffraction pattern of a straightedge.\n\r\t"
        "Arguments:\n\r\t\t"
        "rho (numpy.ndarray):\n\r\t\t\t"
        "A numpy array of positive real numbers.\n\r\t\t"
        "a (float)\n\r\t\t\t"
        "A positive real number, starting point of the straightedge.\n\r\t\t"
        "F (float)\n\r\t\t\t"
        "A positive real number, the Fresnel scale (same units as rho).\n\r\t"
        "Outputs:\n\r\t\t"
        "T_hat (numpy.ndarray):\n\r\t\t\t"
        "Numpy array of complex numbers equal to the diffraction pattern.\n\r\t"
        "Example:\n\r\t\t"
        ">>> import numpy\n\r\t\t"
        ">>> import special_functions\n\r\t\t"
        ">>> x = numpy.arange(0,100,0.01)\n\r\t\t"
        ">>> a = 45\n\r\t\t"
        ">>> F = 0.05\n\r\t\t"
        ">>> y = special_functions.right_straightedge(x, a, F)"
    },
    {
        "left_straightedge",
        left_straightedge,
        METH_VARARGS,
        "\r\t"
        "Function:\n\r\t\t"
        "special_functions.left_straightedge\n\r\t"
        "Purpose\n\r\t\t"
        "Compute the diffraction pattern of a straightedge.\n\r\t"
        "Arguments:\n\r\t\t"
        "rho (numpy.ndarray):\n\r\t\t\t"
        "A numpy array of positive real numbers.\n\r\t\t"
        "a (float)\n\r\t\t\t"
        "A positive real number, starting point of the straightedge.\n\r\t\t"
        "F (float)\n\r\t\t\t"
        "A positive real number, the Fresnel scale (same units as rho).\n\r\t"
        "Outputs:\n\r\t\t"
        "T_hat (numpy.ndarray):\n\r\t\t\t"
        "Numpy array of complex numbers equal to the diffraction pattern.\n\r\t"
        "Example:\n\r\t\t"
        ">>> import numpy\n\r\t\t"
        ">>> import special_functions\n\r\t\t"
        ">>> x = numpy.arange(0,100,0.01)\n\r\t\t"
        ">>> a = 45\n\r\t\t"
        ">>> F = 0.05\n\r\t\t"
        ">>> y = special_functions.left_straightedge(x, a, F)"
    },
    {
        "max",
        max,
        METH_VARARGS,
        "\r\t"
        "Function:\n\r\t\t"
        "special_functions.max\n\r\t"
        "Purpose\n\r\t\t"
        "Compute the maximum of a numpy array.\n\r\t"
        "Arguments:\n\r\t\t"
        "arr (numpy.ndarray):\n\r\t\t\t"
        "A numpy array of real numbers.\n\r\t"
        "Outputs:\n\r\t\t"
        "max (int or float):\n\r\t\t\t"
        "The maximum value of the input array.\n\r\t"
        "Example:\n\r\t\t"
        ">>> import numpy\n\r\t\t"
        ">>> import special_functions\n\r\t\t"
        ">>> x = numpy.random.rand(100)\n\r\t\t"
        ">>> y = special_functions.max(x)"
    },
    {
        "min",
        min,
        METH_VARARGS,
        "\r\t"
        "Function:\n\r\t\t"
        "special_functions.min\n\r\t"
        "Purpose\n\r\t\t"
        "Compute the minimum of a numpy array.\n\r\t"
        "Arguments:\n\r\t\t"
        "arr (numpy.ndarray):\n\r\t\t\t"
        "A numpy array of real numbers.\n\r\t"
        "Outputs:\n\r\t\t"
        "min (int or float):\n\r\t\t\t"
        "The minimum value of the input array.\n\r\t"
        "Example:\n\r\t\t"
        ">>> import numpy\n\r\t\t"
        ">>> import _special_functions\n\r\t\t"
        ">>> x = numpy.random.rand(100)\n\r\t\t"
        ">>> y = special_functions.min(x)"
    },
    {
        "wavelength_to_wavenumber",
        wavelength_to_wavenumber,
        METH_VARARGS,
        "\r\t"
        "Function:\n\r\t\t"
        "special_functions.wavelength_to_wavenumber\n\r\t"
        "Purpose\n\r\t\t"
        "Convert a non-zero wavelength to the equivalent wavenumber.\n\r\t"
        "Arguments:\n\r\t\t"
        "wavelength (numpy.ndarray/int/float/list):\n\r\t\t\t"
        "A numpy array or list of real numbers, the input wavelength.\n\r\t\t"
        "Outputs:\n\r\t\t"
        "wavenumber (float):\n\r\t\t\t"
        "The corresponding wavenumber.\n\r\t"
        "Example:\n\r\t\t"
        ">>> import numpy\n\r\t\t"
        ">>> import special_functions\n\r\t\t"
        ">>> x = numpy.arange(1,10,0.1)"
        ">>> y = special_functions.wavelength_to_wavenumber(x)"
    },
    {
        "resolution_inverse",
        resolution_inverse,
        METH_VARARGS,
        "\r\t"
        "Function:\n\r\t\t"
        "special_functions.resolution_inverse\n\r\t"
        "Purpose\n\r\t\t"
        "Compute the inverse of y = x/(exp(-x)+x-1).\n\r\t"
        "Arguments:\n\r\t\t"
        "x (numpy.ndarray/int/float/list):\n\r\t\t\t"
        "A numpy array or list of real numbers.\n\r\t\t"
        "Outputs:\n\r\t\t"
        "y (float):\n\r\t\t\t"
        "The inverse of x/(exp(-x)+x-1).\n\r\t"
        "Example:\n\r\t\t"
        ">>> import numpy\n\r\t\t"
        ">>> import special_functions\n\r\t\t"
        ">>> x = numpy.arange(1,10,0.1)"
        ">>> y = special_functions.resolution_inverse(x)"
    },
    {
        "where_greater",
        where_greater,
        METH_VARARGS,
        "\r\t"
        "Function:\n\r\t\t"
        "special_functions.where_greater\n\r\t"
        "Purpose\n\r\t\t"
        "Given a real-valued numpy array arr, and a real number\n\r\t\t"
        "threshold, compute the indices n such that arr[n] > threshold\n\r\t"
        "Arguments:\n\r\t\t"
        "arr (numpy.ndarray):\n\r\t\t\t"
        "A numpy array of real numbers.\n\r\t\t"
        "threshold (int or float):\n\r\t\t\t"
        "The threshold value for comparing arr with."
        "Outputs:\n\r\t\t"
        "where_arr (numpy.ndarray):\n\r\t\t\t"
        "The array of indices such that arr[n] > threshold.\n\r\t"
        "Example:\n\r\t\t"
        ">>> import numpy\n\r\t\t"
        ">>> import special_functions\n\r\t\t"
        ">>> x = numpy.arange(-5, 5, 0.01)\n\r\t\t"
        ">>> y = special_functions.where_greater(x, 1.0)"
    },
    {
        "where_lesser",
        where_lesser,
        METH_VARARGS,
        "\r\t"
        "Function:\n\r\t\t"
        "special_functions.where_lesser\n\r\t"
        "Purpose\n\r\t\t"
        "Given a real-valued numpy array arr, and a real number\n\r\t\t"
        "threshold, compute the indices n such that arr[n] < threshold\n\r\t"
        "Arguments:\n\r\t\t"
        "arr (numpy.ndarray):\n\r\t\t\t"
        "A numpy array of real numbers.\n\r\t\t"
        "threshold (int or float):\n\r\t\t\t"
        "The threshold value for comparing arr with."
        "Outputs:\n\r\t\t"
        "where_arr (numpy.ndarray):\n\r\t\t\t"
        "The array of indices such that arr[n] < threshold.\n\r\t"
        "Example:\n\r\t\t"
        ">>> import numpy\n\r\t\t"
        ">>> import special_functions\n\r\t\t"
        ">>> x = numpy.arange(-5, 5, 0.01)\n\r\t\t"
        ">>> y = special_functions.where_lesser(x, 1.0)"
    },
    {
        "window_norm",
        window_norm,
        METH_VARARGS,
        "\r\t"
        "Function:\n\r\t\t"
        "special_functions.window_norm\n\r\t"
        "Purpose\n\r\t\t"
        "Compute the window normalization scheme.\n\r\t"
        "Arguments:\n\r\t\t"
        "ker (numpy.ndarray):\n\r\t\t\t"
        "A numpy array of real numbers, the input function.\n\r\t\t"
        "dx (int or float):\n\r\t\t\t"
        "The sample spacing of the input function.\n\r\t\t"
        "f_scale (int or float):\n\r\t\t\t"
        "The Fresnel scale in the same units as dx.\n\r\t\t"
        "Outputs:\n\r\t\t"
        "window_norm (float):\n\r\t\t\t"
        "The normalization factor.\n\r\t"
        "Example:\n\r\t\t"
        ">>> import numpy\n\r\t\t"
        ">>> import special_functions\n\r\t\t"
        ">>> dx = 0.1"
        ">>> x = numpy.arange(-10,10,dx)\n\r\t\t"
        ">>> ker = special_functions.coss(x, 5)"
        ">>> f_scale = 0.5"
        ">>> y = special_functions.window_norm(ker, dx, f_scale)"
    },
    {NULL, NULL, 0, NULL}
};
/*-------------------------DEFINE UNIVERSAL FUNCTIONS-------------------------*/

/*  Wrappers for the Fresnel scale function.                                  */

static void float_fresnel_scale(char **args, npy_intp *dimensions,
                                npy_intp *steps, void *data)
{
    npy_intp i, j, k, l, m, dim;

    float *lambda  = (float *)args[0];
    float *d       = (float *)args[1];
    float *phi     = (float *)args[2];
    float *b       = (float *)args[3];
    float *out     = (float *)args[4];

    npy_intp l_step = (steps[0] != 0);
    npy_intp d_step = (steps[1] != 0);
    npy_intp p_step = (steps[2] != 0);
    npy_intp b_step = (steps[3] != 0);

    dim = dimensions[0];

    j = 0;
    k = 0;
    l = 0;
    m = 0;

    for (i = 0; i < dim; i++) {
        out[i] = Fresnel_Scale_Float(lambda[j], d[k], phi[l], b[m]);

        j += l_step;
        k += d_step;
        l += p_step;
        m += b_step;
    }
}

static void double_fresnel_scale(char **args, npy_intp *dimensions,
                                 npy_intp *steps, void *data)
{
    npy_intp i, j, k, l, m, dim;

    double *lambda  = (double *)args[0];
    double *d       = (double *)args[1];
    double *phi     = (double *)args[2];
    double *b       = (double *)args[3];
    double *out     = (double *)args[4];

    npy_intp l_step = (steps[0] != 0);
    npy_intp d_step = (steps[1] != 0);
    npy_intp p_step = (steps[2] != 0);
    npy_intp b_step = (steps[3] != 0);

    dim = dimensions[0];


    j = 0;
    k = 0;
    l = 0;
    m = 0;

    for (i = 0; i < dim; i++) {
        out[i] = Fresnel_Scale_Double(lambda[j], d[k], phi[l], b[m]);

        j += l_step;
        k += d_step;
        l += p_step;
        m += b_step;
    }
}

static void long_double_fresnel_scale(char **args, npy_intp *dimensions,
                                      npy_intp *steps, void *data)
{
    npy_intp i, j, k, l, m, dim;

    long double *lambda  = (long double *)args[0];
    long double *d       = (long double *)args[1];
    long double *phi     = (long double *)args[2];
    long double *b       = (long double *)args[3];
    long double *out     = (long double *)args[4];

    npy_intp l_step = (steps[0] != 0);
    npy_intp d_step = (steps[1] != 0);
    npy_intp p_step = (steps[2] != 0);
    npy_intp b_step = (steps[3] != 0);

    dim = dimensions[0];
    

    j = 0;
    k = 0;
    l = 0;
    m = 0;

    for (i = 0; i < dim; i++) {
        out[i] = Fresnel_Scale_Long_Double(lambda[j], d[k], phi[l], b[m]);

        j += l_step;
        k += d_step;
        l += p_step;
        m += b_step;
    }
}

/************Double Slit Diffraction Using Fraunhofer Approximation************/

/******************************************************************************
 *  Function:                                                                 *
 *      float_double_slit_diffraction                                         *
 *  Purpose:                                                                  *
 *      Compute the diffraction pattern from a plane wave incident on a       *
 *      single slit using the Fraunhofer approximation.                       *
 *  Arguments:                                                                *
 *      args (char **):                                                       *
 *          Input and output arguments passed from python.                    *
 *      dimensions (npy_intp *):                                              *
 *          Dimensions of the arguments found in the args pointer.            *
 *      steps (npy_intp):                                                     *
 *          The number of strides in memory from the nth point to the (n+1)th *
 *          point for the arguments found in the args pointer.                *
 *      data (void *):                                                        *
 *          Data pointer.                                                     *
 *  Notes:                                                                    *
 *      1.) This is a wrapper for Double_Slit_Fraunhofer_Diffraction_Float,   *
 *          which is defined in _fraunhofer_diffraction.h. This allows       *
 *          Python to use that function, and allows for numpy arrays to be    *
 *          passed in. Relies on the Numpy UFUNC API and the C-Python API.    *
 *                                                                            *
 *      2.) This function relies on the C99 standard, or higher.              *
 *                                                                            *
 *      3.) There are no error checks in this code. This is handled at the    *
 *          Python level, see special_functions.py.                           *
 ******************************************************************************/
static void float_double_slit_diffraction(char **args, npy_intp *dimensions,
                                          npy_intp* steps, void* data)
{
    /* Declare i for indexing, n is the number of elements in the array.      */
    npy_intp i;
    npy_intp n = dimensions[0];

    /* Extract input data and convert to appropriate types.                   */
    float *x  =  (float *)args[0];
    float  z  = *(float *)args[1];
    float  a  = *(float *)args[2];
    float  d  = *(float *)args[3];
    float  l  = *(float *)args[4];

    /* The output is a pointer to a complex float.                            */
    float *out = (float *)args[5];

    /* Loop over the square well function found in _fresnel_diffraction.h    */
    for (i = 0; i < n; i++) {
        out[i] = Double_Slit_Fraunhofer_Diffraction_Float(x[i], z, a, d, l);
    }
}

static void double_double_slit_diffraction(char **args, npy_intp *dimensions,
                                           npy_intp* steps, void* data)
{
    /* Declare i for indexing, n is the number of elements in the array.      */
    npy_intp i;
    npy_intp n = dimensions[0];

    /* Extract input data and convert to appropriate types.                   */
    double *x  =  (double *)args[0];
    double  z  = *(double *)args[1];
    double  a  = *(double *)args[2];
    double  d  = *(double *)args[3];
    double  l  = *(double *)args[4];

    /* The output is a pointer to a complex float.                            */
    double *out = (double *)args[5];

    /* Loop over the square well function found in _fresnel_diffraction.h    */
    for (i = 0; i < n; i++) {
        out[i] = Double_Slit_Fraunhofer_Diffraction_Double(x[i], z, a, d, l);
    }
}

static void long_double_double_slit_diffraction(char **args,
                                                npy_intp *dimensions,
                                                npy_intp* steps, void* data)
{
    /* Declare i for indexing, n is the number of elements in the array.      */
    npy_intp i;
    npy_intp n = dimensions[0];

    /* Extract input data and convert to appropriate types.                   */
    long double *x  =  (long double *)args[0];
    long double  z  = *(long double *)args[1];
    long double  a  = *(long double *)args[2];
    long double  d  = *(long double *)args[3];
    long double  l  = *(long double *)args[4];

    /* The output is a pointer to a complex float.                            */
    long double *out = (long double *)args[5];

    /* Loop over the square well function found in _fresnel_diffraction.h    */
    for (i = 0; i < n; i++) {
        out[i] = Double_Slit_Fraunhofer_Diffraction_Long_Double(x[i], z,
                                                                a, d, l);
    }
}

/******************************************************************************
 *  Function:                                                                 *
 *      float_single_slit_diffraction                                         *
 *  Purpose:                                                                  *
 *      Compute the diffraction pattern from a plane wave incident on a       *
 *      single slit using the Fraunhofer approximation.                       *
 *  Arguments:                                                                *
 *      args (char **):                                                       *
 *          Input and output arguments passed from python.                    *
 *      dimensions (npy_intp *):                                              *
 *          Dimensions of the arguments found in the args pointer.            *
 *      steps (npy_intp):                                                     *
 *          The number of strides in memory from the nth point to the (n+1)th *
 *          point for the arguments found in the args pointer.                *
 *      data (void *):                                                        *
 *          Data pointer.                                                     *
 *  Notes:                                                                    *
 *      1.) This is a wrapper for Single_Slit_Fraunhofer_Diffraction_Float,   *
 *          which is defined in _fraunhofer_diffraction.h. This allows       *
 *          Python to use that function, and allows for numpy arrays to be    *
 *          passed in. Relies on the Numpy UFUNC API and the C-Python API.    *
 *                                                                            *
 *      2.) This function relies on the C99 standard, or higher.              *
 *                                                                            *
 *      3.) There are no error checks in this code. This is handled at the    *
 *          Python level, see special_functions.py.                           *
 ******************************************************************************/
static void float_single_slit_diffraction(char **args, npy_intp *dimensions,
                                          npy_intp* steps, void* data)
{
    /* Declare i for indexing, n is the number of elements in the array.      */
    npy_intp i;
    npy_intp n = dimensions[0];

    /* Extract input data and convert to appropriate types.                   */
    float *x  =  (float *)args[0];
    float  z  = *(float *)args[1];
    float  a  = *(float *)args[2];

    /* The output is a pointer to a complex float.                            */
    float *out = (float *)args[3];

    /* Loop over the square well function found in _fresnel_diffraction.h    */
    for (i = 0; i < n; i++) {
        out[i] = Single_Slit_Fraunhofer_Diffraction_Float(x[i], z, a);
    }
}

static void double_single_slit_diffraction(char **args, npy_intp *dimensions,
                                           npy_intp* steps, void* data)
{
    /* Declare i for indexing, n is the number of elements in the array.      */
    npy_intp i;
    npy_intp n = dimensions[0];

    /* Extract input data and convert to appropriate types.                   */
    double *x  =  (double *)args[0];
    double  z  = *(double *)args[1];
    double  a  = *(double *)args[2];

    /* The output is a pointer to a complex float.                            */
    double *out = (double *)args[3];

    /* Loop over the square well function found in _fresnel_diffraction.h    */
    for (i = 0; i < n; i++) {
        out[i] = Single_Slit_Fraunhofer_Diffraction_Double(x[i], z, a);
    }
}

static void long_double_single_slit_diffraction(char **args,
                                                npy_intp *dimensions,
                                                npy_intp* steps, void* data)
{
    /* Declare i for indexing, n is the number of elements in the array.      */
    npy_intp i;
    npy_intp n = dimensions[0];

    /* Extract input data and convert to appropriate types.                   */
    long double *x  =  (long double *)args[0];
    long double  z  = *(long double *)args[1];
    long double  a  = *(long double *)args[2];

    /* The output is a pointer to a complex float.                            */
    long double *out = (long double *)args[3];

    /* Loop over the square well function found in _fresnel_diffraction.h    */
    for (i = 0; i < n; i++) {
        out[i] = Single_Slit_Fraunhofer_Diffraction_Long_Double(x[i], z, a);
    }
}

/*  Functions from _fresnel_kernel.h                                         */
static void float_fresnel_psi(char **args, npy_intp *dimensions,
                              npy_intp* steps, void* data)
{
    /*  Variables for indexing the various input arguments.                   */
    long i;
    long kD_i   = 0;
    long rho_i  = 0;
    long rho0_i = 0;
    long phi_i  = 0;
    long phi0_i = 0;
    long B_i    = 0;
    long D_i    = 0;

    /*  The number of elements in the output array.                           */
    long n = dimensions[0];

    float *kD   = (float *)args[0];
    float *rho  = (float *)args[1];
    float *rho0 = (float *)args[2];
    float *phi  = (float *)args[3];
    float *phi0 = (float *)args[4];
    float *B    = (float *)args[5];
    float *D    = (float *)args[6];
    float *out  = (float *)args[7];

    /*  Since both arrays and numbers are allowed as inputs, we must make     *
     *  sure we index the variables appropriately. If an argument is only a   *
     *  single chunk of memory, this variable is a single number and we       *
     *  shouldn't try to index it. Otherwise, the input is an array and we    *
     *  should index over it. The variables below are either zero or one,     *
     *  corresponding to whether the argument is a number or an array.        */
    unsigned char kD_steps   = (steps[0] != 0);
    unsigned char rho_steps  = (steps[1] != 0);
    unsigned char rho0_steps = (steps[2] != 0);
    unsigned char phi_steps  = (steps[3] != 0);
    unsigned char phi0_steps = (steps[4] != 0);
    unsigned char B_steps    = (steps[5] != 0);
    unsigned char D_steps    = (steps[6] != 0);

    /*  Compute the Fresnel Psi function, looping over the variables.         */
    for (i = 0; i < n; i++) {
        out[i] = Fresnel_Psi_Float(kD[kD_i], rho[rho_i], rho0[rho0_i],
                                   phi[phi_i], phi0[phi0_i], B[B_i], D[D_i]);

        kD_i   += kD_steps;
        rho_i  += rho_steps;
        rho0_i += rho0_steps;
        phi_i  += phi_steps;
        phi0_i += phi0_steps;
        B_i    += B_steps;
        D_i    += D_steps;
    }
}

static void double_fresnel_psi(char **args, npy_intp *dimensions,
                               npy_intp* steps, void* data)
{
    /*  Variables for indexing the various input arguments.                   */
    long i;
    long kD_i   = 0;
    long rho_i  = 0;
    long rho0_i = 0;
    long phi_i  = 0;
    long phi0_i = 0;
    long B_i    = 0;
    long D_i    = 0;

    /*  The number of elements in the output array.                           */
    long n = dimensions[0];

    double *kD   = (double *)args[0];
    double *rho  = (double *)args[1];
    double *rho0 = (double *)args[2];
    double *phi  = (double *)args[3];
    double *phi0 = (double *)args[4];
    double *B    = (double *)args[5];
    double *D    = (double *)args[6];
    double *out  = (double *)args[7];

    /*  Since both arrays and numbers are allowed as inputs, we must make     *
     *  sure we index the variables appropriately. If an argument is only a   *
     *  single chunk of memory, this variable is a single number and we       *
     *  shouldn't try to index it. Otherwise, the input is an array and we    *
     *  should index over it. The variables below are either zero or one,     *
     *  corresponding to whether the argument is a number or an array.        */
    unsigned char kD_steps   = (steps[0] != 0);
    unsigned char rho_steps  = (steps[1] != 0);
    unsigned char rho0_steps = (steps[2] != 0);
    unsigned char phi_steps  = (steps[3] != 0);
    unsigned char phi0_steps = (steps[4] != 0);
    unsigned char B_steps    = (steps[5] != 0);
    unsigned char D_steps    = (steps[6] != 0);

    /*  Compute the Fresnel Psi function, looping over the variables.         */
    for (i = 0; i < n; i++) {
        out[i] = Fresnel_Psi_Double(kD[kD_i], rho[rho_i], rho0[rho0_i],
                                    phi[phi_i], phi0[phi0_i], B[B_i], D[D_i]);

        kD_i   += kD_steps;
        rho_i  += rho_steps;
        rho0_i += rho0_steps;
        phi_i  += phi_steps;
        phi0_i += phi0_steps;
        B_i    += B_steps;
        D_i    += D_steps;
    }
}

static void long_double_fresnel_psi(char **args, npy_intp *dimensions,
                                    npy_intp* steps, void* data)
{
    /*  Variables for indexing the various input arguments.                   */
    long i;
    long kD_i   = 0;
    long rho_i  = 0;
    long rho0_i = 0;
    long phi_i  = 0;
    long phi0_i = 0;
    long B_i    = 0;
    long D_i    = 0;

    /*  The number of elements in the output array.                           */
    long n = dimensions[0];

    long double *kD   = (long double *)args[0];
    long double *rho  = (long double *)args[1];
    long double *rho0 = (long double *)args[2];
    long double *phi  = (long double *)args[3];
    long double *phi0 = (long double *)args[4];
    long double *B    = (long double *)args[5];
    long double *D    = (long double *)args[6];
    long double *out  = (long double *)args[7];

    /*  Since both arrays and numbers are allowed as inputs, we must make     *
     *  sure we index the variables appropriately. If an argument is only a   *
     *  single chunk of memory, this variable is a single number and we       *
     *  shouldn't try to index it. Otherwise, the input is an array and we    *
     *  should index over it. The variables below are either zero or one,     *
     *  corresponding to whether the argument is a number or an array.        */
    unsigned char kD_steps   = (steps[0] != 0);
    unsigned char rho_steps  = (steps[1] != 0);
    unsigned char rho0_steps = (steps[2] != 0);
    unsigned char phi_steps  = (steps[3] != 0);
    unsigned char phi0_steps = (steps[4] != 0);
    unsigned char B_steps    = (steps[5] != 0);
    unsigned char D_steps    = (steps[6] != 0);

    /*  Compute the Fresnel Psi function, looping over the variables.         */
    for (i = 0; i < n; i++) {
        out[i] = Fresnel_Psi_Long_Double(kD[kD_i], rho[rho_i],
                                         rho0[rho0_i], phi[phi_i],
                                         phi0[phi0_i], B[B_i], D[D_i]);

        kD_i   += kD_steps;
        rho_i  += rho_steps;
        rho0_i += rho0_steps;
        phi_i  += phi_steps;
        phi0_i += phi0_steps;
        B_i    += B_steps;
        D_i    += D_steps;
    }
}

static void float_fresnel_dpsi_dphi(char **args, npy_intp *dimensions,
                                    npy_intp* steps, void* data)
{
    /*  Variables for indexing the various input arguments.                   */
    long i;
    long kD_i   = 0;
    long rho_i  = 0;
    long rho0_i = 0;
    long phi_i  = 0;
    long phi0_i = 0;
    long B_i    = 0;
    long D_i    = 0;

    /*  The number of elements in the output array.                           */
    long n = dimensions[0];

    float *kD   = (float *)args[0];
    float *rho  = (float *)args[1];
    float *rho0 = (float *)args[2];
    float *phi  = (float *)args[3];
    float *phi0 = (float *)args[4];
    float *B    = (float *)args[5];
    float *D    = (float *)args[6];
    float *out  = (float *)args[7];

    /*  Since both arrays and numbers are allowed as inputs, we must make     *
     *  sure we index the variables appropriately. If an argument is only a   *
     *  single chunk of memory, this variable is a single number and we       *
     *  shouldn't try to index it. Otherwise, the input is an array and we    *
     *  should index over it. The variables below are either zero or one,     *
     *  corresponding to whether the argument is a number or an array.        */
    unsigned char kD_steps   = (steps[0] != 0);
    unsigned char rho_steps  = (steps[1] != 0);
    unsigned char rho0_steps = (steps[2] != 0);
    unsigned char phi_steps  = (steps[3] != 0);
    unsigned char phi0_steps = (steps[4] != 0);
    unsigned char B_steps    = (steps[5] != 0);
    unsigned char D_steps    = (steps[6] != 0);

    /*  Compute the Fresnel Psi function, looping over the variables.         */
    for (i = 0; i < n; i++) {
        out[i] = Fresnel_dPsi_dPhi_Float(kD[kD_i], rho[rho_i],
                                         rho0[rho0_i], phi[phi_i],
                                         phi0[phi0_i], B[B_i], D[D_i]);

        kD_i   += kD_steps;
        rho_i  += rho_steps;
        rho0_i += rho0_steps;
        phi_i  += phi_steps;
        phi0_i += phi0_steps;
        B_i    += B_steps;
        D_i    += D_steps;
    }
}

static void double_fresnel_dpsi_dphi(char **args, npy_intp *dimensions,
                                     npy_intp* steps, void* data)
{
    /*  Variables for indexing the various input arguments.                   */
    long i;
    long kD_i   = 0;
    long rho_i  = 0;
    long rho0_i = 0;
    long phi_i  = 0;
    long phi0_i = 0;
    long B_i    = 0;
    long D_i    = 0;

    /*  The number of elements in the output array.                           */
    long n = dimensions[0];

    double *kD   = (double *)args[0];
    double *rho  = (double *)args[1];
    double *rho0 = (double *)args[2];
    double *phi  = (double *)args[3];
    double *phi0 = (double *)args[4];
    double *B    = (double *)args[5];
    double *D    = (double *)args[6];
    double *out  = (double *)args[7];

    /*  Since both arrays and numbers are allowed as inputs, we must make     *
     *  sure we index the variables appropriately. If an argument is only a   *
     *  single chunk of memory, this variable is a single number and we       *
     *  shouldn't try to index it. Otherwise, the input is an array and we    *
     *  should index over it. The variables below are either zero or one,     *
     *  corresponding to whether the argument is a number or an array.        */
    unsigned char kD_steps   = (steps[0] != 0);
    unsigned char rho_steps  = (steps[1] != 0);
    unsigned char rho0_steps = (steps[2] != 0);
    unsigned char phi_steps  = (steps[3] != 0);
    unsigned char phi0_steps = (steps[4] != 0);
    unsigned char B_steps    = (steps[5] != 0);
    unsigned char D_steps    = (steps[6] != 0);

    /*  Compute the Fresnel Psi function, looping over the variables.         */
    for (i = 0; i < n; i++) {
        out[i] = Fresnel_dPsi_dPhi_Double(kD[kD_i], rho[rho_i],
                                          rho0[rho0_i], phi[phi_i],
                                          phi0[phi0_i], B[B_i], D[D_i]);

        kD_i   += kD_steps;
        rho_i  += rho_steps;
        rho0_i += rho0_steps;
        phi_i  += phi_steps;
        phi0_i += phi0_steps;
        B_i    += B_steps;
        D_i    += D_steps;
    }
}

static void long_double_fresnel_dpsi_dphi(char **args, npy_intp *dimensions,
                                          npy_intp* steps, void* data)
{
    /*  Variables for indexing the various input arguments.                   */
    long i;
    long kD_i   = 0;
    long rho_i  = 0;
    long rho0_i = 0;
    long phi_i  = 0;
    long phi0_i = 0;
    long B_i    = 0;
    long D_i    = 0;

    /*  The number of elements in the output array.                           */
    long n = dimensions[0];

    long double *kD   = (long double *)args[0];
    long double *rho  = (long double *)args[1];
    long double *rho0 = (long double *)args[2];
    long double *phi  = (long double *)args[3];
    long double *phi0 = (long double *)args[4];
    long double *B    = (long double *)args[5];
    long double *D    = (long double *)args[6];
    long double *out  = (long double *)args[7];

    /*  Since both arrays and numbers are allowed as inputs, we must make     *
     *  sure we index the variables appropriately. If an argument is only a   *
     *  single chunk of memory, this variable is a single number and we       *
     *  shouldn't try to index it. Otherwise, the input is an array and we    *
     *  should index over it. The variables below are either zero or one,     *
     *  corresponding to whether the argument is a number or an array.        */
    unsigned char kD_steps   = (steps[0] != 0);
    unsigned char rho_steps  = (steps[1] != 0);
    unsigned char rho0_steps = (steps[2] != 0);
    unsigned char phi_steps  = (steps[3] != 0);
    unsigned char phi0_steps = (steps[4] != 0);
    unsigned char B_steps    = (steps[5] != 0);
    unsigned char D_steps    = (steps[6] != 0);

    /*  Compute the Fresnel Psi function, looping over the variables.         */
    for (i = 0; i < n; i++) {
        out[i] = Fresnel_dPsi_dPhi_Long_Double(kD[kD_i], rho[rho_i],
                                               rho0[rho0_i], phi[phi_i],
                                               phi0[phi0_i], B[B_i], D[D_i]);

        kD_i   += kD_steps;
        rho_i  += rho_steps;
        rho0_i += rho0_steps;
        phi_i  += phi_steps;
        phi0_i += phi0_steps;
        B_i    += B_steps;
        D_i    += D_steps;
    }
}

static void float_fresnel_d2psi_dphi2(char **args, npy_intp *dimensions,
                                      npy_intp* steps, void* data)
{
    /*  Variables for indexing the various input arguments.                   */
    long i;
    long kD_i   = 0;
    long rho_i  = 0;
    long rho0_i = 0;
    long phi_i  = 0;
    long phi0_i = 0;
    long B_i    = 0;
    long D_i    = 0;

    /*  The number of elements in the output array.                           */
    long n = dimensions[0];

    float *kD   = (float *)args[0];
    float *rho  = (float *)args[1];
    float *rho0 = (float *)args[2];
    float *phi  = (float *)args[3];
    float *phi0 = (float *)args[4];
    float *B    = (float *)args[5];
    float *D    = (float *)args[6];
    float *out  = (float *)args[7];

    /*  Since both arrays and numbers are allowed as inputs, we must make     *
     *  sure we index the variables appropriately. If an argument is only a   *
     *  single chunk of memory, this variable is a single number and we       *
     *  shouldn't try to index it. Otherwise, the input is an array and we    *
     *  should index over it. The variables below are either zero or one,     *
     *  corresponding to whether the argument is a number or an array.        */
    unsigned char kD_steps   = (steps[0] != 0);
    unsigned char rho_steps  = (steps[1] != 0);
    unsigned char rho0_steps = (steps[2] != 0);
    unsigned char phi_steps  = (steps[3] != 0);
    unsigned char phi0_steps = (steps[4] != 0);
    unsigned char B_steps    = (steps[5] != 0);
    unsigned char D_steps    = (steps[6] != 0);

    /*  Compute the Fresnel Psi function, looping over the variables.         */
    for (i = 0; i < n; i++) {
        out[i] = Fresnel_d2Psi_dPhi2_Float(kD[kD_i], rho[rho_i],
                                           rho0[rho0_i], phi[phi_i],
                                           phi0[phi0_i], B[B_i], D[D_i]);

        kD_i   += kD_steps;
        rho_i  += rho_steps;
        rho0_i += rho0_steps;
        phi_i  += phi_steps;
        phi0_i += phi0_steps;
        B_i    += B_steps;
        D_i    += D_steps;
    }
}

static void double_fresnel_d2psi_dphi2(char **args, npy_intp *dimensions,
                                       npy_intp* steps, void* data)
{
    /*  Variables for indexing the various input arguments.                   */
    long i;
    long kD_i   = 0;
    long rho_i  = 0;
    long rho0_i = 0;
    long phi_i  = 0;
    long phi0_i = 0;
    long B_i    = 0;
    long D_i    = 0;

    /*  The number of elements in the output array.                           */
    long n = dimensions[0];

    double *kD   = (double *)args[0];
    double *rho  = (double *)args[1];
    double *rho0 = (double *)args[2];
    double *phi  = (double *)args[3];
    double *phi0 = (double *)args[4];
    double *B    = (double *)args[5];
    double *D    = (double *)args[6];
    double *out  = (double *)args[7];

    /*  Since both arrays and numbers are allowed as inputs, we must make     *
     *  sure we index the variables appropriately. If an argument is only a   *
     *  single chunk of memory, this variable is a single number and we       *
     *  shouldn't try to index it. Otherwise, the input is an array and we    *
     *  should index over it. The variables below are either zero or one,     *
     *  corresponding to whether the argument is a number or an array.        */
    unsigned char kD_steps   = (steps[0] != 0);
    unsigned char rho_steps  = (steps[1] != 0);
    unsigned char rho0_steps = (steps[2] != 0);
    unsigned char phi_steps  = (steps[3] != 0);
    unsigned char phi0_steps = (steps[4] != 0);
    unsigned char B_steps    = (steps[5] != 0);
    unsigned char D_steps    = (steps[6] != 0);

    /*  Compute the Fresnel Psi function, looping over the variables.         */
    for (i = 0; i < n; i++) {
        out[i] = Fresnel_d2Psi_dPhi2_Double(kD[kD_i], rho[rho_i],
                                            rho0[rho0_i], phi[phi_i],
                                            phi0[phi0_i], B[B_i], D[D_i]);

        kD_i   += kD_steps;
        rho_i  += rho_steps;
        rho0_i += rho0_steps;
        phi_i  += phi_steps;
        phi0_i += phi0_steps;
        B_i    += B_steps;
        D_i    += D_steps;
    }
}

static void long_double_fresnel_d2psi_dphi2(char **args, npy_intp *dimensions,
                                            npy_intp* steps, void* data)
{
    /*  Variables for indexing the various input arguments.                   */
    long i;
    long kD_i   = 0;
    long rho_i  = 0;
    long rho0_i = 0;
    long phi_i  = 0;
    long phi0_i = 0;
    long B_i    = 0;
    long D_i    = 0;

    /*  The number of elements in the output array.                           */
    long n = dimensions[0];

    long double *kD   = (long double *)args[0];
    long double *rho  = (long double *)args[1];
    long double *rho0 = (long double *)args[2];
    long double *phi  = (long double *)args[3];
    long double *phi0 = (long double *)args[4];
    long double *B    = (long double *)args[5];
    long double *D    = (long double *)args[6];
    long double *out  = (long double *)args[7];

    /*  Since both arrays and numbers are allowed as inputs, we must make     *
     *  sure we index the variables appropriately. If an argument is only a   *
     *  single chunk of memory, this variable is a single number and we       *
     *  shouldn't try to index it. Otherwise, the input is an array and we    *
     *  should index over it. The variables below are either zero or one,     *
     *  corresponding to whether the argument is a number or an array.        */
    unsigned char kD_steps   = (steps[0] != 0);
    unsigned char rho_steps  = (steps[1] != 0);
    unsigned char rho0_steps = (steps[2] != 0);
    unsigned char phi_steps  = (steps[3] != 0);
    unsigned char phi0_steps = (steps[4] != 0);
    unsigned char B_steps    = (steps[5] != 0);
    unsigned char D_steps    = (steps[6] != 0);

    /*  Compute the Fresnel Psi function, looping over the variables.         */
    for (i = 0; i < n; i++) {
        out[i] = Fresnel_d2Psi_dPhi2_Long_Double(kD[kD_i], rho[rho_i],
                                                 rho0[rho0_i], phi[phi_i],
                                                 phi0[phi0_i], B[B_i], D[D_i]);

        kD_i   += kD_steps;
        rho_i  += rho_steps;
        rho0_i += rho0_steps;
        phi_i  += phi_steps;
        phi0_i += phi0_steps;
        B_i    += B_steps;
        D_i    += D_steps;
    }
}

static void float_fresnel_dpsi_dphi_ellipse(char **args, npy_intp *dimensions,
                                            npy_intp* steps, void* data)
{
    /*  Variables for indexing the various input arguments.                   */
    long i;
    long kD_i   = 0;
    long rho_i  = 0;
    long rho0_i = 0;
    long phi_i  = 0;
    long phi0_i = 0;
    long B_i    = 0;
    long D_i    = 0;

    /*  The number of elements in the output array.                           */
    long n = dimensions[0];

    float *kD   =  (float *)args[0];
    float *rho  =  (float *)args[1];
    float *rho0 =  (float *)args[2];
    float *phi  =  (float *)args[3];
    float *phi0 =  (float *)args[4];
    float *B    =  (float *)args[5];
    float *D    =  (float *)args[6];
    float ecc   = *(float *)args[7];
    float peri  = *(float *)args[8];
    float *out  =  (float *)args[9];

    /*  Since both arrays and numbers are allowed as inputs, we must make     *
     *  sure we index the variables appropriately. If an argument is only a   *
     *  single chunk of memory, this variable is a single number and we       *
     *  shouldn't try to index it. Otherwise, the input is an array and we    *
     *  should index over it. The variables below are either zero or one,     *
     *  corresponding to whether the argument is a number or an array.        */
    unsigned char kD_steps   = (steps[0] != 0);
    unsigned char rho_steps  = (steps[1] != 0);
    unsigned char rho0_steps = (steps[2] != 0);
    unsigned char phi_steps  = (steps[3] != 0);
    unsigned char phi0_steps = (steps[4] != 0);
    unsigned char B_steps    = (steps[5] != 0);
    unsigned char D_steps    = (steps[6] != 0);

    /*  Compute the Fresnel Psi function, looping over the variables.         */
    for (i = 0; i < n; i++) {
        out[i] = Fresnel_dPsi_dPhi_Ellipse_Float(kD[kD_i], rho[rho_i],
                                                 rho0[rho0_i], phi[phi_i],
                                                 phi0[phi0_i], B[B_i], D[D_i],
                                                 ecc, peri);

        kD_i   += kD_steps;
        rho_i  += rho_steps;
        rho0_i += rho0_steps;
        phi_i  += phi_steps;
        phi0_i += phi0_steps;
        B_i    += B_steps;
        D_i    += D_steps;
    }
}

static void double_fresnel_dpsi_dphi_ellipse(char **args, npy_intp *dimensions,
                                             npy_intp* steps, void* data)
{
    /*  Variables for indexing the various input arguments.                   */
    long i;
    long kD_i   = 0;
    long rho_i  = 0;
    long rho0_i = 0;
    long phi_i  = 0;
    long phi0_i = 0;
    long B_i    = 0;
    long D_i    = 0;

    /*  The number of elements in the output array.                           */
    long n = dimensions[0];

    double *kD   =  (double *)args[0];
    double *rho  =  (double *)args[1];
    double *rho0 =  (double *)args[2];
    double *phi  =  (double *)args[3];
    double *phi0 =  (double *)args[4];
    double *B    =  (double *)args[5];
    double *D    =  (double *)args[6];
    double ecc   = *(double *)args[7];
    double peri  = *(double *)args[8];
    double *out  =  (double *)args[9];

    /*  Since both arrays and numbers are allowed as inputs, we must make     *
     *  sure we index the variables appropriately. If an argument is only a   *
     *  single chunk of memory, this variable is a single number and we       *
     *  shouldn't try to index it. Otherwise, the input is an array and we    *
     *  should index over it. The variables below are either zero or one,     *
     *  corresponding to whether the argument is a number or an array.        */
    unsigned char kD_steps   = (steps[0] != 0);
    unsigned char rho_steps  = (steps[1] != 0);
    unsigned char rho0_steps = (steps[2] != 0);
    unsigned char phi_steps  = (steps[3] != 0);
    unsigned char phi0_steps = (steps[4] != 0);
    unsigned char B_steps    = (steps[5] != 0);
    unsigned char D_steps    = (steps[6] != 0);

    /*  Compute the Fresnel Psi function, looping over the variables.         */
    for (i = 0; i < n; i++) {
        out[i] = Fresnel_dPsi_dPhi_Ellipse_Double(kD[kD_i], rho[rho_i],
                                                  rho0[rho0_i], phi[phi_i],
                                                  phi0[phi0_i], B[B_i], D[D_i],
                                                  ecc, peri);

        kD_i   += kD_steps;
        rho_i  += rho_steps;
        rho0_i += rho0_steps;
        phi_i  += phi_steps;
        phi0_i += phi0_steps;
        B_i    += B_steps;
        D_i    += D_steps;
    }
}

static void long_double_fresnel_dpsi_dphi_ellipse(char **args,
                                                  npy_intp *dimensions,
                                                  npy_intp* steps, void* data)
{
    /*  Variables for indexing the various input arguments.                   */
    long i;
    long kD_i   = 0;
    long rho_i  = 0;
    long rho0_i = 0;
    long phi_i  = 0;
    long phi0_i = 0;
    long B_i    = 0;
    long D_i    = 0;

    /*  The number of elements in the output array.                           */
    long n = dimensions[0];

    long double *kD   =  (long double *)args[0];
    long double *rho  =  (long double *)args[1];
    long double *rho0 =  (long double *)args[2];
    long double *phi  =  (long double *)args[3];
    long double *phi0 =  (long double *)args[4];
    long double *B    =  (long double *)args[5];
    long double *D    =  (long double *)args[6];
    long double ecc   = *(long double *)args[7];
    long double peri  = *(long double *)args[8];
    long double *out  =  (long double *)args[9];

    /*  Since both arrays and numbers are allowed as inputs, we must make     *
     *  sure we index the variables appropriately. If an argument is only a   *
     *  single chunk of memory, this variable is a single number and we       *
     *  shouldn't try to index it. Otherwise, the input is an array and we    *
     *  should index over it. The variables below are either zero or one,     *
     *  corresponding to whether the argument is a number or an array.        */
    unsigned char kD_steps   = (steps[0] != 0);
    unsigned char rho_steps  = (steps[1] != 0);
    unsigned char rho0_steps = (steps[2] != 0);
    unsigned char phi_steps  = (steps[3] != 0);
    unsigned char phi0_steps = (steps[4] != 0);
    unsigned char B_steps    = (steps[5] != 0);
    unsigned char D_steps    = (steps[6] != 0);

    /*  Compute the Fresnel Psi function, looping over the variables.         */
    for (i = 0; i < n; i++) {
        out[i] = Fresnel_dPsi_dPhi_Ellipse_Long_Double(kD[kD_i], rho[rho_i],
                                                       rho0[rho0_i], phi[phi_i],
                                                       phi0[phi0_i], B[B_i],
                                                       D[D_i], ecc, peri);

        kD_i   += kD_steps;
        rho_i  += rho_steps;
        rho0_i += rho0_steps;
        phi_i  += phi_steps;
        phi0_i += phi0_steps;
        B_i    += B_steps;
        D_i    += D_steps;
    }
}

PyUFuncGenericFunction double_slit_funcs[3] = {
    &float_double_slit_diffraction,
    &double_double_slit_diffraction,
    &long_double_double_slit_diffraction
};

PyUFuncGenericFunction fresnel_scale_funcs[3] = {
    &float_fresnel_scale,
    &double_fresnel_scale,
    &long_double_fresnel_scale
};

PyUFuncGenericFunction psi_funcs[3] = {
    &float_fresnel_psi,
    &double_fresnel_psi,
    &long_double_fresnel_psi
};

PyUFuncGenericFunction dpsi_funcs[3] = {
    &float_fresnel_dpsi_dphi,
    &double_fresnel_dpsi_dphi,
    &long_double_fresnel_dpsi_dphi
};

PyUFuncGenericFunction d2psi_funcs[3] = {
    &float_fresnel_d2psi_dphi2,
    &double_fresnel_d2psi_dphi2,
    &long_double_fresnel_d2psi_dphi2
};

PyUFuncGenericFunction dpsi_ellipse_funcs[3] = {
    &float_fresnel_dpsi_dphi_ellipse,
    &double_fresnel_dpsi_dphi_ellipse,
    &long_double_fresnel_dpsi_dphi_ellipse
};

PyUFuncGenericFunction single_slit_funcs[3] = {
    &float_single_slit_diffraction,
    &double_single_slit_diffraction,
    &long_double_single_slit_diffraction
};

/*  Input and return types for double input and out.                          */
static void *PyuFunc_None_3[3] = {NULL, NULL, NULL};

static char three_real_in_one_real_out[12] = {
    NPY_FLOAT, NPY_FLOAT, NPY_FLOAT, NPY_FLOAT,
    NPY_DOUBLE, NPY_DOUBLE, NPY_DOUBLE, NPY_DOUBLE,
    NPY_LONGDOUBLE, NPY_LONGDOUBLE, NPY_LONGDOUBLE, NPY_LONGDOUBLE
};

static char four_real_in_one_real_out[15] = {
    NPY_FLOAT, NPY_FLOAT, NPY_FLOAT, NPY_FLOAT, NPY_FLOAT,
    NPY_DOUBLE, NPY_DOUBLE, NPY_DOUBLE, NPY_DOUBLE, NPY_DOUBLE,
    NPY_LONGDOUBLE, NPY_LONGDOUBLE, NPY_LONGDOUBLE, NPY_LONGDOUBLE,
    NPY_LONGDOUBLE
};

static char five_real_in_one_real_out[18] = {
    NPY_FLOAT, NPY_FLOAT, NPY_FLOAT, NPY_FLOAT, NPY_FLOAT, NPY_FLOAT,
    NPY_DOUBLE, NPY_DOUBLE, NPY_DOUBLE, NPY_DOUBLE, NPY_DOUBLE, NPY_DOUBLE,
    NPY_LONGDOUBLE, NPY_LONGDOUBLE, NPY_LONGDOUBLE, NPY_LONGDOUBLE,
    NPY_LONGDOUBLE, NPY_LONGDOUBLE
};

static char seven_real_in_one_real_out[24] = {
    NPY_FLOAT, NPY_FLOAT, NPY_FLOAT, NPY_FLOAT,
    NPY_FLOAT, NPY_FLOAT, NPY_FLOAT, NPY_FLOAT,
    NPY_DOUBLE, NPY_DOUBLE, NPY_DOUBLE, NPY_DOUBLE,
    NPY_DOUBLE, NPY_DOUBLE, NPY_DOUBLE, NPY_DOUBLE,
    NPY_LONGDOUBLE, NPY_LONGDOUBLE, NPY_LONGDOUBLE, NPY_LONGDOUBLE,
    NPY_LONGDOUBLE, NPY_LONGDOUBLE, NPY_LONGDOUBLE, NPY_LONGDOUBLE
};

static char nine_real_in_one_real_out[30] = {
    NPY_FLOAT, NPY_FLOAT, NPY_FLOAT, NPY_FLOAT, NPY_FLOAT,
    NPY_FLOAT, NPY_FLOAT, NPY_FLOAT, NPY_FLOAT, NPY_FLOAT,
    NPY_DOUBLE, NPY_DOUBLE, NPY_DOUBLE, NPY_DOUBLE, NPY_DOUBLE,
    NPY_DOUBLE, NPY_DOUBLE, NPY_DOUBLE, NPY_DOUBLE, NPY_DOUBLE,
    NPY_LONGDOUBLE, NPY_LONGDOUBLE, NPY_LONGDOUBLE, NPY_LONGDOUBLE,
    NPY_LONGDOUBLE, NPY_LONGDOUBLE, NPY_LONGDOUBLE, NPY_LONGDOUBLE,
    NPY_LONGDOUBLE, NPY_LONGDOUBLE
};

#if PY_VERSION_HEX >= 0x03000000
static struct PyModuleDef moduledef = {
    PyModuleDef_HEAD_INIT,
    "_special_functions",
    NULL,
    -1,
    _special_functions_methods,
    NULL,
    NULL,
    NULL,
    NULL
};

PyMODINIT_FUNC PyInit__special_functions(void)
{
    PyObject *double_slit_diffraction;
    PyObject *fresnel_psi;
    PyObject *fresnel_dpsi_dphi;
    PyObject *fresnel_d2psi_dphi2;
    PyObject *fresnel_dpsi_dphi_ellipse;
    PyObject *fresnel_scale;
    PyObject *single_slit_diffraction;
    PyObject *m, *d;

    m = PyModule_Create(&moduledef);
    if (!m) {
        return NULL;
    }

    import_array();
    import_umath();

    double_slit_diffraction = PyUFunc_FromFuncAndData(
        double_slit_funcs, PyuFunc_None_3, five_real_in_one_real_out,
        3, 5, 1, PyUFunc_None, "double_slit_diffraction", 
        "double_slit_diffraction_docstring", 0
    );

    fresnel_dpsi_dphi = PyUFunc_FromFuncAndData(
        dpsi_funcs, PyuFunc_None_3, seven_real_in_one_real_out, 3, 7, 1,
        PyUFunc_None, "fresnel_dpsi_dphi",  "fresnel_dpsi_dphi_docstring", 0
    );

    fresnel_d2psi_dphi2 = PyUFunc_FromFuncAndData(
        d2psi_funcs, PyuFunc_None_3, seven_real_in_one_real_out, 3, 7, 1,
        PyUFunc_None, "fresnel_d2psi_dphi2",  "fresnel_d2psi_dphi2_docstring", 0
    );

    fresnel_dpsi_dphi_ellipse = PyUFunc_FromFuncAndData(
        dpsi_ellipse_funcs, PyuFunc_None_3, nine_real_in_one_real_out, 3, 9, 1,
        PyUFunc_None, "fresnel_dpsi_dphi_ellipse", 
        "fresnel_dpsi_dphi_ellipse_docstring", 0
    );

    fresnel_psi = PyUFunc_FromFuncAndData(
        psi_funcs, PyuFunc_None_3, seven_real_in_one_real_out, 3, 7, 1,
        PyUFunc_None, "fresnel_psi",  "fresnel_psi_docstring", 0
    );

    fresnel_scale = PyUFunc_FromFuncAndData(
        fresnel_scale_funcs, PyuFunc_None_3, four_real_in_one_real_out, 3, 4, 1,
        PyUFunc_None, "fresnel_scale", "fresnel_scale_docstring", 0
    );

    single_slit_diffraction = PyUFunc_FromFuncAndData(
        single_slit_funcs, PyuFunc_None_3, three_real_in_one_real_out,
        3, 3, 1, PyUFunc_None, "single_slit_diffraction", 
        "single_slit_diffraction_docstring", 0
    );

    d = PyModule_GetDict(m);

    PyDict_SetItemString(d, "double_slit_diffraction", double_slit_diffraction);
    PyDict_SetItemString(d, "fresnel_psi", fresnel_psi);
    PyDict_SetItemString(d, "fresnel_dpsi_dphi", fresnel_dpsi_dphi);
    PyDict_SetItemString(d, "fresnel_d2psi_dphi2", fresnel_d2psi_dphi2);
    PyDict_SetItemString(d, "fresnel_dpsi_dphi_ellipse",
                         fresnel_dpsi_dphi_ellipse);
    PyDict_SetItemString(d, "fresnel_scale", fresnel_scale);
    PyDict_SetItemString(d, "single_slit_diffraction", single_slit_diffraction);

    Py_DECREF(double_slit_diffraction);
    Py_DECREF(fresnel_psi);
    Py_DECREF(fresnel_dpsi_dphi);
    Py_DECREF(fresnel_d2psi_dphi2);
    Py_DECREF(fresnel_dpsi_dphi_ellipse);
    Py_DECREF(fresnel_scale);
    Py_DECREF(single_slit_diffraction);

    return m;
}
#else
PyMODINIT_FUNC init_funcs(void)
{
    PyObject *double_slit_diffraction;
    PyObject *fresnel_psi;
    PyObject *fresnel_dpsi_dphi;
    PyObject *fresnel_d2psi_dphi2;
    PyObject *fresnel_dpsi_dphi_ellipse;
    PyObject *fresnel_scale;
    PyObject *single_slit_diffraction;
    PyObject *m, *d;

    m = Py_InitModule("_funcs", _special_functions_methods);
    if (m == NULL) {
        return;
    }

    import_array();
    import_umath();

    double_slit_diffraction = PyUFunc_FromFuncAndData(
        double_slit_funcs, PyuFunc_None_3, five_real_in_one_real_out,
        3, 5, 1, PyUFunc_None, "double_slit_diffraction", 
        "double_slit_diffraction_docstring", 0
    );

    fresnel_dpsi_dphi = PyUFunc_FromFuncAndData(
        dpsi_funcs, PyuFunc_None_3, seven_real_in_one_real_out, 3, 7, 1,
        PyUFunc_None, "fresnel_dpsi_dphi",  "fresnel_dpsi_dphi_docstring", 0
    );

    fresnel_d2psi_dphi2 = PyUFunc_FromFuncAndData(
        d2psi_funcs, PyuFunc_None_3, seven_real_in_one_real_out, 3, 7, 1,
        PyUFunc_None, "fresnel_d2psi_dphi2",  "fresnel_d2psi_dphi2_docstring", 0
    );

    fresnel_dpsi_dphi_ellipse = PyUFunc_FromFuncAndData(
        dpsi_ellipse_funcs, PyuFunc_None_3, nine_real_in_one_real_out, 3, 9, 1,
        PyUFunc_None, "fresnel_dpsi_dphi_ellipse", 
        "fresnel_dpsi_dphi_ellipse_docstring", 0
    );

    fresnel_psi = PyUFunc_FromFuncAndData(
        psi_funcs, PyuFunc_None_3, seven_real_in_one_real_out, 3, 7, 1,
        PyUFunc_None, "fresnel_psi",  "fresnel_psi_docstring", 0
    );

    fresnel_scale = PyUFunc_FromFuncAndData(
        fresnel_scale_funcs, PyuFunc_None_3, four_real_in_one_real_out, 3, 4, 1,
        PyUFunc_None, "fresnel_scale", "fresnel_scale_docstring", 0
    );

    single_slit_diffraction = PyUFunc_FromFuncAndData(
        single_slit_funcs, PyuFunc_None_3, three_real_in_one_real_out,
        3, 3, 1, PyUFunc_None, "single_slit_diffraction", 
        "single_slit_diffraction_docstring", 0
    );


    d = PyModule_GetDict(m);

    PyDict_SetItemString(d, "double_slit_diffraction", double_slit_diffraction);
    PyDict_SetItemString(d, "fresnel_psi", fresnel_psi);
    PyDict_SetItemString(d, "fresnel_dpsi_dphi", fresnel_dpsi_dphi);
    PyDict_SetItemString(d, "fresnel_d2psi_dphi2", fresnel_d2psi_dphi2);
    PyDict_SetItemString(d, "fresnel_dpsi_dphi_ellipse",
                         fresnel_dpsi_dphi_ellipse);
    PyDict_SetItemString(d, "fresnel_scale", fresnel_scale);
    PyDict_SetItemString(d, "single_slit_diffraction", single_slit_diffraction);

    Py_DECREF(double_slit_diffraction);
    Py_DECREF(fresnel_psi);
    Py_DECREF(fresnel_dpsi_dphi);
    Py_DECREF(fresnel_d2psi_dphi2);
    Py_DECREF(fresnel_dpsi_dphi_ellipse);
    Py_DECREF(fresnel_scale);
    Py_DECREF(single_slit_diffraction);

    return m;
}
#endif
