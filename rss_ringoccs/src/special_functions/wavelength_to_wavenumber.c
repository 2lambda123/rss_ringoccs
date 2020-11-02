/*  The two pi is defined here.                                               */
#include <rss_ringoccs/src/math/rss_ringoccs_math.h>

/*  Prototypes for these functions declared here.                             */
#include "rss_ringoccs_special_functions.h"

/*  Convert a wavelength to a wavelength.                                     */
#define _define_wavelength_to_wavenumber(type, Type)                           \
type rssringoccs_Wavelength_To_Wavenumber_##Type(type lambda)                  \
{                                                                              \
    /*  The conversion is the two pi over the wavelength.                    */\
    return TWO_PI / lambda;                                                    \
}

_define_wavelength_to_wavenumber(float, Float)
_define_wavelength_to_wavenumber(double, Double)
_define_wavelength_to_wavenumber(long double, LongDouble)
