/*
 * floating-point arctangent
 *
 * atan returns the value of the arctangent of its
 * argument in the range [-pi/2,pi/2].
 *
 * atan2 returns the arctangent of arg1/arg2
 * in the range [-pi,pi].
 *
 * there are no error returns.
 *
 * coefficients are #5077 from Hart & Cheney. (19.56D)
 */

#include <math.h>

static const double sq2p1  = 2.414213562373095048802e0;
static const double sq2m1  = 0.414213562373095048802e0;
static const double pio2   = 1.570796326794896619231e0;
static const double pio4   = 0.785398163397448309615e0;
static const double p4     = 0.161536412982230228262e2;
static const double p3     = 0.26842548195503973794141e3;
static const double p2     = 0.11530293515404850115428136e4;
static const double p1     = 0.178040631643319697105464587e4;
static const double p0     = 0.89678597403663861959987488e3;
static const double q4     = 0.5895697050844462222791e2;
static const double q3     = 0.536265374031215315104235e3;
static const double q2     = 0.16667838148816337184521798e4;
static const double q1     = 0.207933497444540981287275926e4;
static const double q0     = 0.89678597403663861962481162e3;

/*
 * xatan evaluates a series valid in the
 * range [-0.414...,+0.414...].
 */
static double xatan(double arg)
{
    double argsq;
    double value;

    argsq = arg * arg;
    value = ((((p4*argsq + p3)*argsq + p2)*argsq + p1)*argsq + p0);
    value = value / (((((argsq + q4) * argsq + q3) * argsq + q2) * argsq + q1) *
                     argsq + q0);
    return value * arg;
}

/*
 * satan reduces its argument (known to be positive)
 * to the range [0,0.414...] and calls xatan.
 */
static double satan(double arg)
{
    if (arg < sq2m1)
        return xatan(arg);
    else if (arg > sq2p1)
        return pio2 - xatan(1.0 / arg);
    else
        return pio4 + xatan((arg - 1.0) / (arg + 1.0));
}

/*
 * atan makes its argument positive and
 * calls the inner routine satan.
 */
double atan(double arg)
{
    if (arg > 0)
        return satan(arg);
    else
        return -satan(-arg);
}


/*
 * atan2 discovers what quadrant the angle
 * is in and calls atan.
 */
double atan2(double arg1, double arg2)
{
    if ((arg1 + arg2) == arg1) {
        if (arg1 >= 0.)
            return pio2;
        else
            return -pio2;
    } else if (arg2 < 0.) {
        if (arg1 >= 0.)
            return pio2 + pio2 - satan(-arg1 / arg2);
        else
            return -pio2 - pio2 + satan(arg1 / arg2);
    } else if (arg1 > 0) {
        return satan(arg1 / arg2);
    } else {
        return -satan(-arg1 / arg2);
    }
}
