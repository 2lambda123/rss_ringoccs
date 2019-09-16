#include "__where.h"

/*  Make sure the name __get_where_pointer is available.                      */
#ifdef __get_where_pointer
#undef __get_where_pointer
#endif

/*  To avoid repeating the same code over and over again, define this macro   *
 *  to be used for all of the where_greater functions. Since the only thing   *
 *  that changes between the various functions is the type of the input       *
 *  pointer, the code is exactly the same.                                    */

#define __get_where_pointer(data, dim, threshold) ({\
    /*  Declare necessary variables.                                         */\
    long i;\
    long arr_size = 0;\
    \
    /*  Create a dummy array to store the correct indices in for now.        */\
    long *dummy = (long *)malloc(sizeof(long) * dim);\
    \
    /*  Loop over the input array data to check which indices match.         */\
    for (i=0; i<dim; ++i) {\
        if (data[i] > threshold){\
            dummy[arr_size] = i;\
            arr_size += 1;\
        }\
    }\
    \
    /*  Resize the array dummy to avoid hogging extra memory.                */\
    long *arr = (long *)malloc(sizeof(long) * arr_size);\
    for (i=0; i<arr_size; ++i){\
        arr[i] = dummy[i];\
    }\
    \
    /*  Since arr now contains all of the necessary info, free dummy.        */\
    free(dummy);\
    \
    /*  Declare a pointer to a pointer to containg all info to be returned.  */\
    long **where = (long **)malloc(sizeof(long) * 2);\
    \
    /*  The first index is the actual array of indices.                      */\
    where[0] = arr;\
    \
    /*  The second index is the size of the returning array.                 */\
    where[1] = &arr_size;\
    return where;\
})
 
long **Where_Greater_Char(char *data, long dim, double threshold)
{
    __get_where_pointer(data, dim, threshold);
}

long **Where_Greater_UChar(unsigned char *data, long dim, double threshold)
{
    __get_where_pointer(data, dim, threshold);
}

long **Where_Greater_Short(short *data, long dim, double threshold)
{
    __get_where_pointer(data, dim, threshold);
}

long **Where_Greater_UShort(unsigned short *data, long dim, double threshold)
{
    __get_where_pointer(data, dim, threshold);
}

long **Where_Greater_Int(int *data, long dim, double threshold)
{
    __get_where_pointer(data, dim, threshold);
}

long **Where_Greater_UInt(unsigned int *data, long dim, double threshold)
{
    __get_where_pointer(data, dim, threshold);
}

long **Where_Greater_Long(long *data, long dim, double threshold)
{
    __get_where_pointer(data, dim, threshold);
}

long **Where_Greater_ULong(unsigned long *data, long dim, double threshold)
{
    __get_where_pointer(data, dim, threshold);
}

long **Where_Greater_Long_Long(long long *data, long dim, double threshold)
{
    __get_where_pointer(data, dim, threshold);
}

long **Where_Greater_ULong_Long(unsigned long long *data,
                                long dim, double threshold)
{
    __get_where_pointer(data, dim, threshold);
}

long **Where_Greater_Float(float *data, long dim, double threshold)
{
    __get_where_pointer(data, dim, threshold);
}

long **Where_Greater_Double(double *data, long dim, double threshold)
{
    __get_where_pointer(data, dim, threshold);
}

long **Where_Greater_Long_Double(long double *data, long dim,
                                 long double threshold)
{
    __get_where_pointer(data, dim, threshold);
}

/*  Erase the __get_where_pointer macro.                                      */
#undef __get_where_pointer