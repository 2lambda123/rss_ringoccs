/******************************************************************************
 *                                 LICENSE                                    *
 ******************************************************************************
 *  This file is part of rss_ringoccs.                                        *
 *                                                                            *
 *  rss_ringoccs is free software: you can redistribute it and/or modify it   *
 *  it under the terms of the GNU General Public License as published by      *
 *  the Free Software Foundation, either version 3 of the License, or         *
 *  (at your option) any later version.                                       *
 *                                                                            *
 *  rss_ringoccs is distributed in the hope that it will be useful,           *
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of            *
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the             *
 *  GNU General Public License for more details.                              *
 *                                                                            *
 *  You should have received a copy of the GNU General Public License         *
 *  along with rss_ringoccs.  If not, see <https://www.gnu.org/licenses/>.    *
 ******************************************************************************
 *                        rss_ringoccs_complex_abs                            *
 ******************************************************************************
 *  Purpose:                                                                  *
 *      Contains the source code for f(z) = |z|^2.                            *
 ******************************************************************************
 *                               DEPENDENCIES                                 *
 ******************************************************************************
 *  1.) rss_ringoccs_complex.h:                                               *
 *          Header where complex types and function prototypes are defined.   *
 ******************************************************************************
 *  Author:     Ryan Maguire, Wellesley College                               *
 *  Date:       November 30, 2020                                             *
 ******************************************************************************
 *                             Revision History                               *
 ******************************************************************************
 *  2020/12/02 (Ryan Maguire):                                                *
 *      Moved functions here from rss_ringoccs_complex_abs.c                  *
 *      Frozen for v1.3.                                                      *
 ******************************************************************************/

/*  Where the prototypes are declared and where complex types are defined.    */
#include <rss_ringoccs/include/rss_ringoccs_complex.h>

/*  C99 does not provide |z|^2 functions, so we can use the same algorithm    *
 *  for both the C89 and C99 renditions of rss_ringoccs.                      */

/*  Single precision abs squared function.                                    */
float rssringoccs_CFloat_Abs_Squared(rssringoccs_ComplexFloat z)
{
    /*  Declare necessary variables. C89 requires declarations at the top.    */
    float real, imag, abs_sq;

    /*  Extract the real and imaginary parts from the input complex number.   */
    real = rssringoccs_CFloat_Real_Part(z);
    imag = rssringoccs_CFloat_Imag_Part(z);

    /*  |z|^2 = x^2 + y^2 so compute this.                                    */
    abs_sq = real*real + imag*imag;
    return abs_sq;
}
/*  End of rssringoccs_CFloat_Abs_Squared.                                    */

/*  Double precision abs squared function.                                    */
double rssringoccs_CDouble_Abs_Squared(rssringoccs_ComplexDouble z)
{
    /*  Declare necessary variables. C89 requires declarations at the top.    */
    double real, imag, abs_sq;

    /*  Extract the real and imaginary parts from the input complex number.   */
    real = rssringoccs_CDouble_Real_Part(z);
    imag = rssringoccs_CDouble_Imag_Part(z);

    /*  |z|^2 = x^2 + y^2 so compute this.                                    */
    abs_sq = real*real + imag*imag;
    return abs_sq;
}
/*  End of rssringoccs_CDouble_Abs_Squared.                                   */

/*  Long double precision abs squared function.                               */
long double
rssringoccs_CLDouble_Abs_Squared(rssringoccs_ComplexLongDouble z)
{
    /*  Declare necessary variables. C89 requires declarations at the top.    */
    long double real, imag, abs_sq;

    /*  Extract the real and imaginary parts from the input complex number.   */
    real = rssringoccs_CLDouble_Real_Part(z);
    imag = rssringoccs_CLDouble_Imag_Part(z);

    /*  |z|^2 = x^2 + y^2 so compute this.                                    */
    abs_sq = real*real + imag*imag;
    return abs_sq;
}
/*  End of rssringoccs_CLDouble_Abs_Squared.                                  */