double R_pow(double x, double y);
static R_INLINE double R_POW(double x, double y) /* handle x ^ 2 inline */
{
    return y == 2.0 ? x * x : R_pow(x, y);
}
