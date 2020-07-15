//----------------------------------------------------------------------------
// This code is copied from the R source code. It attempts to mirror the
// R source code as closely as possible, adding only small changes:
// (a) to allow using custom allocators with results of binary operations, and
// (b) to make the changes in (a) possible while outside of the R runtime.
//----------------------------------------------------------------------------

#define USE_RINTERNALS
#include <R.h>
#include <Rinternals.h>
#include <R_ext/Rallocators.h>
#include <R_ext/Itermacros.h>
#include <complex.h>
#include <Rmath.h>

//----------------------------------------------------------------------------
// Beyond this section we borrow from internal R header files.
//----------------------------------------------------------------------------

// XXX Borrowed from Defn.h START
/* Arithmetic and Relation Operators */
typedef enum {
    PLUSOP = 1,
    MINUSOP,
    TIMESOP,
    DIVOP,
    POWOP,
    MODOP,
    IDIVOP
} ARITHOP_TYPE;

typedef enum {
    EQOP = 1,
    NEOP,
    LTOP,
    LEOP,
    GEOP,
    GTOP
} RELOP_TYPE;

#ifndef NO_NLS
# ifdef ENABLE_NLS
#  include <libintl.h>
#  ifdef Win32
#   define _(String) libintl_gettext (String)
#   undef gettext /* needed for graphapp */
#  else
#   define _(String) gettext (String)
#  endif
#  define gettext_noop(String) String
#  define N_(String) gettext_noop (String)
#  else /* not NLS */
#  define _(String) (String)
#  define N_(String) String
#  define ngettext(String, StringP, N) (N > 1 ? StringP: String)
# endif
#endif

/* Required by C99, but might be slow */
#ifdef HAVE_LONG_DOUBLE
# define LDOUBLE long double
#else
# define LDOUBLE double
#endif
// XXX Borrowed from Defn.h END

//XXX Borrowed from Rcomplex.h START
/*
   Note: this could use the C11 CMPLX() macro.
   As could mycpow, z_tan and some of the substitutes.
 */
static /*R_INLINE*/ double complex toC99(const Rcomplex *x)
{
#if __GNUC__
    double complex ans = (double complex) 0; /* -Wall */
    __real__ ans = x->r;
    __imag__ ans = x->i;
    return ans;
#else
    return x->r + x->i * I;
#endif
}

static /*R_INLINE*/ void
SET_C99_COMPLEX(Rcomplex *x, R_xlen_t i, double complex value)
{
    Rcomplex *ans = x+i;
    ans->r = creal(value);
    ans->i = cimag(value);
}
//XXX Borrowed from Rcomplex.h END

//XXX Borrowed from complex.c START
static /*R_INLINE*/ double complex R_cpow_n(double complex X, int k)
{
    if(k == 0) return (double complex) 1.;
    else if(k == 1) return X;
    else if(k < 0) return 1. / R_cpow_n(X, -k);
    else {/* k > 0 */
    double complex z = (double complex) 1.;;
    while (k > 0) {
        if (k & 1) z = z * X;
        if (k == 1) break;
        k >>= 1; /* efficient division by 2; now have k >= 1 */
        X = X * X;
    }
    return z;
    }
}

#if defined(Win32)
# undef HAVE_CPOW
#endif
/* reason for this:
  1) X^n  (e.g. for n = +/- 2, 3) is unnecessarily inaccurate in glibc;
     cut-off 65536 : guided from empirical speed measurements

  2) On Mingw (but not Mingw-w64) the system cpow is explicitly linked
     against the (slow) MSVCRT pow, and gets (0+0i)^Y as 0+0i for all Y.

  3) PPC macOS crashed on powers of 0+0i (at least under Rosetta).
  Really 0i^-1 should by Inf+NaNi, but getting that portably seems too hard.
  (C1x's CMPLX will eventually be possible.)
*/

static double complex mycpow (double complex X, double complex Y)
{
    double complex Z;
    double yr = creal(Y), yi = cimag(Y);
    int k;
    if (X == 0.0) {
	if (yi == 0.0) Z = R_pow(0.0, yr); else Z = R_NaN + R_NaN*I;
    } else if (yi == 0.0 && yr == (k = (int) yr) && abs(k) <= 65536)
	Z = R_cpow_n(X, k);
    else
#ifdef HAVE_CPOW
	Z = cpow(X, Y);
#else
    {
	/* Used for FreeBSD and MingGW, hence mainly with gcc */
	double rho, r, i, theta;
	r = hypot(creal(X), cimag(X));
	i = atan2(cimag(X), creal(X));
	theta = i * yr;
	if (yi == 0.0)
	    rho = pow(r, yr);
	else {
	    /* rearrangement of cexp(X * clog(Y)) */
	    r = log(r);
	    theta += r * yi;
	    rho = exp(r * yr - i * yi);
	}
#ifdef __GNUC__
	__real__ Z = rho * cos(theta);
	__imag__ Z = rho * sin(theta);
#else
	Z = rho * cos(theta) + (rho * sin(theta)) * I;
#endif
    }
#endif
    return Z;
}
//XXX Borrowed from complex.c END

//XXX Borrowed from util.c START
int mbcslocale = 0;

size_t Mbrtowc(wchar_t *wc, const char *s, size_t n, mbstate_t *ps)
{
    size_t used;

    if(n <= 0 || !*s) return (size_t)0;
    used = mbrtowc(wc, s, n, ps);
    if((int) used < 0) {
	/* This gets called from the menu setup in RGui */
	if (!R_Is_Running) return (size_t)-1;
	/* let's try to print out a readable version */
	R_CheckStack2(4*strlen(s) + 10);
	char err[4*strlen(s) + 1], *q;
	const char *p;
	for(p = s, q = err; *p; ) {
	    /* don't do the first to keep ps state straight */
	    if(p > s) used = mbrtowc(NULL, p, n, ps);
	    if(used == 0) break;
	    else if((int) used > 0) {
		memcpy(q, p, used);
		p += used;
		q += used;
		n -= used;
	    } else {
		sprintf(q, "<%02x>", (unsigned char) *p++);
		q += 4;
		n--;
	    }
	}
	*q = '\0';
	error(_("invalid multibyte string at '%s'"), err);
    }
    return used;
}

/*attribute_hidden*/
char* mbcsTruncateToValid(char *s)
{
    if (!mbcslocale)
	return s;

    mbstate_t mb_st;
    size_t slen = strlen(s);
    size_t goodlen = 0;

    mbs_init(&mb_st);
    while(goodlen < slen) {
	size_t res;
	res = mbrtowc(NULL, s + goodlen, slen - goodlen, &mb_st);
	if (res == (size_t) -1 || res == (size_t) -2) {
	    /* strip off all remaining characters */
	    for(;goodlen < slen; goodlen++)
		s[goodlen] = '\0';
	    return s;
	}
	goodlen += res;
    }
    return s;
}
//XXX Borrowed from util.c END

//XXX Borrowed from errors.c START
/* Rvsnprintf: like vsnprintf, but guaranteed to null-terminate and not to
   split multi-byte characters */
#ifdef Win32
int trio_vsnprintf(char *buffer, size_t bufferSize, const char *format,
		   va_list args);

static int Rvsnprintf(char *buf, size_t size, const char  *format, va_list ap)
{
    int val;
    val = trio_vsnprintf(buf, size, format, ap);
    buf[size-1] = '\0';
    if (val >= size)
	mbcsTruncateToValid(buf);
    return val;
}
#else
static int Rvsnprintf(char *buf, size_t size, const char  *format, va_list ap)
{
    int val;
    val = vsnprintf(buf, size, format, ap);
    buf[size-1] = '\0';
    if (val >= size)
	mbcsTruncateToValid(buf);
    return val;
}
#endif
//XXX Borrowed from errors.c END

//----------------------------------------------------------------------------
// In this section no changes happen: these things are only here because they
// are only available inside R's arithmetic.h/arithmetic.c
//----------------------------------------------------------------------------

#define INTEGER_OVERFLOW_WARNING _("NAs produced by integer overflow")

#define CHECK_INTEGER_OVERFLOW(call, ans, naflag) do {		\
	if (naflag) {						\
	    PROTECT(ans);					\
	    warningcall(call, INTEGER_OVERFLOW_WARNING);	\
	    UNPROTECT(1);					\
	}							\
    } while(0)

#define R_INT_MAX  INT_MAX
#define R_INT_MIN -INT_MAX

static /*R_INLINE*/ int R_integer_plus(int x, int y, Rboolean *pnaflag)
{
    if (x == NA_INTEGER || y == NA_INTEGER)
	return NA_INTEGER;

    if (((y > 0) && (x > (R_INT_MAX - y))) ||
	((y < 0) && (x < (R_INT_MIN - y)))) {
	if (pnaflag != NULL)
	    *pnaflag = TRUE;
	return NA_INTEGER;
    }
    return x + y;
}

static /*R_INLINE*/ int R_integer_minus(int x, int y, Rboolean *pnaflag)
{
    if (x == NA_INTEGER || y == NA_INTEGER)
	return NA_INTEGER;

    if (((y < 0) && (x > (R_INT_MAX + y))) ||
	((y > 0) && (x < (R_INT_MIN + y)))) {
	if (pnaflag != NULL)
	    *pnaflag = TRUE;
	return NA_INTEGER;
    }
    return x - y;
}

#define GOODIPROD(x, y, z) ((double) (x) * (double) (y) == (z))
static /*R_INLINE*/ int R_integer_times(int x, int y, Rboolean *pnaflag)
{
    if (x == NA_INTEGER || y == NA_INTEGER)
	return NA_INTEGER;
    else {
	int z = x * y;  // UBSAN will warn if this overflows (happens in bda)
	if (GOODIPROD(x, y, z) && z != NA_INTEGER)
	    return z;
	else {
	    if (pnaflag != NULL)
		*pnaflag = TRUE;
	    return NA_INTEGER;
	}
    }
}

static /*R_INLINE*/ double R_integer_divide(int x, int y)
{
    if (x == NA_INTEGER || y == NA_INTEGER)
	return NA_REAL;
    else
	return (double) x / (double) y;
}

double R_pow(double x, double y);

static /*R_INLINE*/ double R_POW(double x, double y) /* handle x ^ 2 inline */
{
    return y == 2.0 ? x * x : R_pow(x, y);
}

#if HAVE_LONG_DOUBLE && (SIZEOF_LONG_DOUBLE > SIZEOF_DOUBLE)
# ifdef __powerpc__
 // PowerPC 64 (when gcc has -mlong-double-128) fails constant folding with LDOUBLE
 // Debian Bug#946836 shows it is needed also for 32-bit ppc, not just __PPC64__
#  define q_1_eps (1 / LDBL_EPSILON)
# else
static LDOUBLE q_1_eps = 1 / LDBL_EPSILON;
# endif
#else
static double  q_1_eps = 1 / DBL_EPSILON;
#endif

/* Keep myfmod() and myfloor() in step */
static double myfmod(double x1, double x2)
{
    if (x2 == 0.0) return R_NaN;
    if(fabs(x2) > q_1_eps && R_FINITE(x1) && fabs(x1) <= fabs(x2)) {
	return
	    (fabs(x1) == fabs(x2)) ? 0 :
	    ((x1 < 0 && x2 > 0) ||
	     (x2 < 0 && x1 > 0))
	     ? x1+x2  // differing signs
	     : x1   ; // "same" signs (incl. 0)
    }
    double q = x1 / x2;
    if(R_FINITE(q) && (fabs(q) > q_1_eps))
	warning(_("probable complete loss of accuracy in modulus"));
    LDOUBLE tmp = (LDOUBLE)x1 - floor(q) * (LDOUBLE)x2;
    return (double) (tmp - floorl(tmp/x2) * x2);
}

static double myfloor(double x1, double x2)
{
    double q = x1 / x2;
    if (x2 == 0.0 || fabs(q) > q_1_eps || !R_FINITE(q))
	return q;
    if(fabs(q) < 1)
	return (q < 0) ? -1
	    : ((x1 < 0 && x2 > 0) ||
	       (x1 > 0 && x2 < 0) // differing signs
	       ? -1 : 0);
    LDOUBLE tmp = (LDOUBLE)x1 - floor(q) * (LDOUBLE)x2;
    return (double) (floor(q) + floorl(tmp/x2));
}

/* interval at which to check interrupts, a guess */
#define NINTERRUPT 10000000

double R_pow(double x, double y) /* = x ^ y */
{
    /* squaring is the most common of the specially handled cases so
       check for it first. */
    if(y == 2.0)
	return x * x;
    if(x == 1. || y == 0.)
	return(1.);
    if(x == 0.) {
	if(y > 0.) return(0.);
	else if(y < 0) return(R_PosInf);
	else return(y); /* NA or NaN, we assert */
    }
    if (R_FINITE(x) && R_FINITE(y)) {
	/* There was a special case for y == 0.5 here, but
	   gcc 4.3.0 -g -O2 mis-compiled it.  Showed up with
	   100^0.5 as 3.162278, example(pbirthday) failed. */
#ifdef USE_POWL_IN_R_POW
	// this is used only on 64-bit Windows (so has powl).
	return powl(x, y);
#else
	return pow(x, y);
#endif
    }
    if (ISNAN(x) || ISNAN(y))
	return(x + y);
    if(!R_FINITE(x)) {
	if(x > 0)		/* Inf ^ y */
	    return (y < 0.)? 0. : R_PosInf;
	else {			/* (-Inf) ^ y */
	    if(R_FINITE(y) && y == floor(y)) /* (-Inf) ^ n */
		return (y < 0.) ? 0. : (myfmod(y, 2.) != 0 ? x  : -x);
	}
    }
    if(!R_FINITE(y)) {
	if(x >= 0) {
	    if(y > 0)		/* y == +Inf */
		return (x >= 1) ? R_PosInf : 0.;
	    else		/* y == -Inf */
		return (x < 1) ? R_PosInf : 0.;
	}
    }
    return R_NaN; // all other cases: (-Inf)^{+-Inf, non-int}; (neg)^{+-Inf}
}

//----------------------------------------------------------------------------
// Dirty hacks
//
// These implement partial functionality of R subsystems that are to big and
// too basic to implement in whole.
//----------------------------------------------------------------------------

ARITHOP_TYPE PRIMVAL(SEXP op) {
	int offset = op->u.primsxp.offset;
	switch (offset) {
	case 64: /*+*/   return PLUSOP;
	case 65: /*-*/   return MINUSOP;
	case 66: /***/   return TIMESOP;
	case 67: /*/*/   return DIVOP;
	case 68: /*^*/   return POWOP;
	case 69: /*%%*/  return MODOP;
	case 70: /*%/%*/ return IDIVOP;
	case 71: /*%*%*/ return 0;
	case 72: /*==*/  return EQOP;
	case 73: /*!=*/  return NEOP;
	case 74: /*<*/   return LTOP;
	case 75: /*<=*/  return LEOP;
	case 76: /*>=*/  return GEOP;
	case 77: /*>*/   return GTOP;
	case 78: /*&*/   return 1;
	case 79: /*|*/   return 2;
	case 80: /*!*/   return 3;
	case 81: /*&&*/  return 1;
	case 82: /*||*/  return 2;
	case 83: /*:*/   return 0;
	case 84: /*~*/ 	 return 0;
	case 85: /*all*/ return 1;
	case 86: /*any*/ return 2;
	default: Rf_error("Unknown offset in hacked PRIMVAL (only implements 64-86): %i", offset);
	}
}

int ERROR_TSVEC_MISMATCH = 100;
/*attribute_hidden*/
void /*NORET*/ ErrorMessage(SEXP call, int which_error, ...)
{
	int BUFSIZE = 8192;
	char buf[BUFSIZE];
	va_list(ap);
	char *format;


    if (which_error == ERROR_TSVEC_MISMATCH) {
    	format = "time-series/vector length mismatch";
    } else {
    	Rf_error("Unknown error in hacked ErrorMessage: %i (only implements 100)", which_error);
    }

    va_start(ap, which_error);
    Rvsnprintf(buf, BUFSIZE, _(format), ap);
    va_end(ap);
    errorcall(call, "%s", buf);
}

//----------------------------------------------------------------------------
// In this section changes happen.
//
// Relevant R files: arithmetic. complex.c
//----------------------------------------------------------------------------

SEXP R_unary(SEXP, SEXP, SEXP, R_allocator_t*);
SEXP R_binary(SEXP, SEXP, SEXP, SEXP, R_allocator_t*);
static SEXP logical_unary(ARITHOP_TYPE, SEXP, SEXP, R_allocator_t*);
static SEXP integer_unary(ARITHOP_TYPE, SEXP, SEXP, R_allocator_t*);
static SEXP real_unary(ARITHOP_TYPE, SEXP, SEXP, R_allocator_t*);
static SEXP real_binary(ARITHOP_TYPE, SEXP, SEXP, R_allocator_t*);
static SEXP integer_binary(ARITHOP_TYPE, SEXP, SEXP, SEXP, R_allocator_t*);

SEXP complex_unary(ARITHOP_TYPE code, SEXP s1, SEXP call, R_allocator_t*);
SEXP complex_binary(ARITHOP_TYPE code, SEXP s1, SEXP s2, R_allocator_t*);

/* for binary operations */
/* adapted from Radford Neal's pqR */
static /*R_INLINE*/ SEXP R_allocOrReuseVector(SEXP s1, SEXP s2,
					  SEXPTYPE type , R_xlen_t n,
					  R_allocator_t *custom_allocator)
{
    R_xlen_t n1 = XLENGTH(s1);
    R_xlen_t n2 = XLENGTH(s2);

    /* Try to use space for 2nd arg if both same length, so 1st argument's
       attributes will then take precedence when copied. */

    if (n == n2) {
        if (TYPEOF(s2) == type && NO_REFERENCES(s2)) {
	    if (ATTRIB(s2) != R_NilValue)
		/* need to remove 'names' attribute if present to
		   match what copyMostAttrib does. copyMostAttrib()
		   also skips 'dim' and 'dimnames' but those, here
		   since those, if present, will be replaced by
		   attribute cleanup code in R_Binary) */
		setAttrib(s2, R_NamesSymbol, R_NilValue);
            return s2;
	}
        else
            /* Can use 1st arg's space only if 2nd arg has no attributes, else
               we may not get attributes of result right. */
            if (n == n1 && TYPEOF(s1) == type && NO_REFERENCES(s1)
		&& ATTRIB(s2) == R_NilValue)
                return s1;
    }
    else if (n == n1 && TYPEOF(s1) == type && NO_REFERENCES(s1))
	return s1;

    return allocVector3(type, n, custom_allocator);
}

SEXP /*attribute_hidden*/ R_unary(SEXP call, SEXP op, SEXP s1,
		                          R_allocator_t *custom_allocator)
{
    ARITHOP_TYPE operation = (ARITHOP_TYPE) PRIMVAL(op);
    switch (TYPEOF(s1)) {
    case LGLSXP:
	return logical_unary(operation, s1, call, custom_allocator);
    case INTSXP:
	return integer_unary(operation, s1, call,custom_allocator);
    case REALSXP:
	return real_unary(operation, s1, call, custom_allocator);
    case CPLXSXP:
	return complex_unary(operation, s1, call, custom_allocator);
    default:
	errorcall(call, _("invalid argument to unary operator"));
    }
    return s1;			/* never used; to keep -Wall happy */
}

static SEXP logical_unary(ARITHOP_TYPE code, SEXP s1, SEXP call,
		                  R_allocator_t *custom_allocator)
{
    R_xlen_t n = XLENGTH(s1);
    SEXP ans = PROTECT(allocVector3(INTSXP, n, custom_allocator));
    SEXP names = PROTECT(getAttrib(s1, R_NamesSymbol));
    SEXP dim = PROTECT(getAttrib(s1, R_DimSymbol));
    SEXP dimnames = PROTECT(getAttrib(s1, R_DimNamesSymbol));
    if(names != R_NilValue) setAttrib(ans, R_NamesSymbol, names);
    if(dim != R_NilValue) setAttrib(ans, R_DimSymbol, dim);
    if(dimnames != R_NilValue) setAttrib(ans, R_DimNamesSymbol, dimnames);
    UNPROTECT(3);

    int *pa = INTEGER(ans);
    const int *px = LOGICAL_RO(s1);

    switch (code) {
    case PLUSOP:
	for (R_xlen_t  i = 0; i < n; i++) pa[i] = px[i];
	break;
    case MINUSOP:
	for (R_xlen_t  i = 0; i < n; i++) {
	    int x = px[i];
	    pa[i] = (x == NA_INTEGER) ?
		NA_INTEGER : ((x == 0.0) ? 0 : -x);
	}
	break;
    default:
	errorcall(call, _("invalid unary operator"));
    }
    UNPROTECT(1);
    return ans;
}

static SEXP integer_unary(ARITHOP_TYPE code, SEXP s1, SEXP call,
		                  R_allocator_t *custom_allocator)
{
    R_xlen_t i, n;
    SEXP ans;

    switch (code) {
    case PLUSOP:
	return s1;
    case MINUSOP:
	ans = NO_REFERENCES(s1) ? s1 : duplicate(s1); // FIXME
	int *pa = INTEGER(ans);
	const int *px = INTEGER_RO(s1);
	n = XLENGTH(s1);
	for (i = 0; i < n; i++) {
	    int x = px[i];
	    pa[i] = (x == NA_INTEGER) ?
		NA_INTEGER : ((x == 0.0) ? 0 : -x);
	}
	return ans;
    default:
	errorcall(call, _("invalid unary operator"));
    }
    return s1;			/* never used; to keep -Wall happy */
}

static SEXP real_unary(ARITHOP_TYPE code, SEXP s1, SEXP lcall,
		               R_allocator_t *custom_allocator)
{
    R_xlen_t i, n;
    SEXP ans;

    switch (code) {
    case PLUSOP: return s1;
    case MINUSOP:
	ans = NO_REFERENCES(s1) ? s1 : duplicate(s1); // FIXME
	double *pa = REAL(ans);
	const double *px = REAL_RO(s1);
	n = XLENGTH(s1);
	for (i = 0; i < n; i++)
	    pa[i] = -px[i];
	return ans;
    default:
	errorcall(lcall, _("invalid unary operator"));
    }
    return s1;			/* never used; to keep -Wall happy */
}

#define COERCE_IF_NEEDED(v, tp, vpi) do { \
    if (TYPEOF(v) != (tp)) { \
	int __vo__ = OBJECT(v); \
	REPROTECT(v = coerceVector(v, (tp)), vpi); \
	if (__vo__) SET_OBJECT(v, 1); \
    } \
} while (0)

#define FIXUP_NULL_AND_CHECK_TYPES(v, vpi) do { \
    switch (TYPEOF(v)) { \
    case NILSXP: REPROTECT(v = allocVector(INTSXP,0), vpi); break; \
    case CPLXSXP: case REALSXP: case INTSXP: case LGLSXP: break; \
    default: errorcall(call, _("non-numeric argument to binary operator")); \
    } \
} while (0)

SEXP /*attribute_hidden*/ R_binary(SEXP call, SEXP op, SEXP x, SEXP y,
								   R_allocator_t *custom_allocator)
{
    Rboolean xattr, yattr, xarray, yarray, xts, yts, xS4, yS4;
    PROTECT_INDEX xpi, ypi;
    ARITHOP_TYPE oper = (ARITHOP_TYPE) PRIMVAL(op);
    int nprotect = 2; /* x and y */


    PROTECT_WITH_INDEX(x, &xpi);
    PROTECT_WITH_INDEX(y, &ypi);

    FIXUP_NULL_AND_CHECK_TYPES(x, xpi);
    FIXUP_NULL_AND_CHECK_TYPES(y, ypi);

    R_xlen_t
	nx = XLENGTH(x),
	ny = XLENGTH(y);
    if (ATTRIB(x) != R_NilValue) {
	xattr = TRUE;
	xarray = isArray(x);
	xts = isTs(x);
	xS4 = isS4(x);
    }
    else xattr = xarray = xts = xS4 = FALSE;
    if (ATTRIB(y) != R_NilValue) {
	yattr = TRUE;
	yarray = isArray(y);
	yts = isTs(y);
	yS4 = isS4(y);
    }
    else yattr = yarray = yts = yS4 = FALSE;

#define R_ARITHMETIC_ARRAY_1_SPECIAL

#ifdef R_ARITHMETIC_ARRAY_1_SPECIAL
    /* If either x or y is a matrix with length 1 and the other is a
       vector of a different length, we want to coerce the matrix to be a vector.
       Do we want to?  We don't do it!  BDR 2004-03-06

       From 3.4.0 (Sep. 2016), this signals a warning,
       and in the future we will disable these 2 clauses,
       so it will give an error.
    */

    /* FIXME: Danger Will Robinson.
     * -----  We might be trashing arguments here.
     */
    if (xarray != yarray) {
    	if (xarray && nx==1 && ny!=1) {
	    if(ny != 0)
		warningcall(call, _(
	"Recycling array of length 1 in array-vector arithmetic is deprecated.\n\
  Use c() or as.vector() instead.\n"));
    	    REPROTECT(x = duplicate(x), xpi);
    	    setAttrib(x, R_DimSymbol, R_NilValue);
    	}
    	if (yarray && ny==1 && nx!=1) {
	    if(nx != 0)
		warningcall(call, _(
	"Recycling array of length 1 in vector-array arithmetic is deprecated.\n\
  Use c() or as.vector() instead.\n"));
    	    REPROTECT(y = duplicate(y), ypi);
    	    setAttrib(y, R_DimSymbol, R_NilValue);
    	}
    }
#endif

    SEXP dims, xnames, ynames;
    if (xarray || yarray) {
	/* if one is a length-atleast-1-array and the
	 * other  is a length-0 *non*array, then do not use array treatment */
	if (xarray && yarray) {
	    if (!conformable(x, y))
		errorcall(call, _("non-conformable arrays"));
	    PROTECT(dims = getAttrib(x, R_DimSymbol)); nprotect++;
	}
	else if (xarray && (ny != 0 || nx == 0)) {
	    PROTECT(dims = getAttrib(x, R_DimSymbol)); nprotect++;
	}
	else if (yarray && (nx != 0 || ny == 0)) {
	    PROTECT(dims = getAttrib(y, R_DimSymbol)); nprotect++;
	} else
	    dims = R_NilValue;
	if (xattr) {
	    PROTECT(xnames = getAttrib(x, R_DimNamesSymbol));
	    nprotect++;
	}
	else xnames = R_NilValue;
	if (yattr) {
	    PROTECT(ynames = getAttrib(y, R_DimNamesSymbol));
	    nprotect++;
	}
	else ynames = R_NilValue;
    }
    else {
	dims = R_NilValue;
	if (xattr) {
	    PROTECT(xnames = getAttrib(x, R_NamesSymbol));
	    nprotect++;
	}
	else xnames = R_NilValue;
	if (yattr) {
	    PROTECT(ynames = getAttrib(y, R_NamesSymbol));
	    nprotect++;
	}
	else ynames = R_NilValue;
    }

    SEXP klass = NULL, tsp = NULL; // -Wall
    if (xts || yts) {
	if (xts && yts) {
	    /* could check ts conformance here */
	    PROTECT(tsp = getAttrib(x, R_TspSymbol));
	    PROTECT(klass = getAttrib(x, R_ClassSymbol));
	}
	else if (xts) {
	    if (nx < ny)
		ErrorMessage(call, ERROR_TSVEC_MISMATCH);
	    PROTECT(tsp = getAttrib(x, R_TspSymbol));
	    PROTECT(klass = getAttrib(x, R_ClassSymbol));
	}
	else {			/* (yts) */
	    if (ny < nx)
		ErrorMessage(call, ERROR_TSVEC_MISMATCH);
	    PROTECT(tsp = getAttrib(y, R_TspSymbol));
	    PROTECT(klass = getAttrib(y, R_ClassSymbol));
	}
	nprotect += 2;
    }

    if (nx > 0 && ny > 0 &&
	((nx > ny) ? nx % ny : ny % nx) != 0) // mismatch
	warningcall(call,
		    _("longer object length is not a multiple of shorter object length"));

    SEXP val;
    /* need to preserve object here, as *_binary copies class attributes */
    if (TYPEOF(x) == CPLXSXP || TYPEOF(y) == CPLXSXP) {
	COERCE_IF_NEEDED(x, CPLXSXP, xpi);
	COERCE_IF_NEEDED(y, CPLXSXP, ypi);
	val = complex_binary(oper, x, y, custom_allocator);
    }
    else if (TYPEOF(x) == REALSXP || TYPEOF(y) == REALSXP) {
	/* real_binary can handle REALSXP or INTSXP operand, but not LGLSXP. */
	/* Can get a LGLSXP. In base-Ex.R on 24 Oct '06, got 8 of these. */
	if (TYPEOF(x) != INTSXP) COERCE_IF_NEEDED(x, REALSXP, xpi);
	if (TYPEOF(y) != INTSXP) COERCE_IF_NEEDED(y, REALSXP, ypi);
	val = real_binary(oper, x, y, custom_allocator);
    }
    else val = integer_binary(oper, x, y, call, custom_allocator);

    /* quick return if there are no attributes */
    if (! xattr && ! yattr) {
	UNPROTECT(nprotect);
	return val;
    }

    PROTECT(val);
    nprotect++;

    if (dims != R_NilValue) {
	    setAttrib(val, R_DimSymbol, dims);
	    if (xnames != R_NilValue)
		setAttrib(val, R_DimNamesSymbol, xnames);
	    else if (ynames != R_NilValue)
		setAttrib(val, R_DimNamesSymbol, ynames);
    }
    else {
	if (XLENGTH(val) == xlength(xnames))
	    setAttrib(val, R_NamesSymbol, xnames);
	else if (XLENGTH(val) == xlength(ynames))
	    setAttrib(val, R_NamesSymbol, ynames);
    }

    if (xts || yts) {		/* must set *after* dims! */
	setAttrib(val, R_TspSymbol, tsp);
	setAttrib(val, R_ClassSymbol, klass);
    }

    if(xS4 || yS4) {   /* Only set the bit:  no method defined! */
	val = asS4(val, TRUE, TRUE);
    }
    UNPROTECT(nprotect);
    return val;
}


static SEXP integer_binary(ARITHOP_TYPE code, SEXP s1, SEXP s2, SEXP lcall,
						   R_allocator_t *custom_allocator)
{
    R_xlen_t i, i1, i2, n, n1, n2;
    int x1, x2;
    SEXP ans;
    Rboolean naflag = FALSE;

    n1 = XLENGTH(s1);
    n2 = XLENGTH(s2);
    /* S4-compatibility change: if n1 or n2 is 0, result is of length 0 */
    if (n1 == 0 || n2 == 0) n = 0; else n = (n1 > n2) ? n1 : n2;

    if (code == DIVOP || code == POWOP)
	ans = allocVector3(REALSXP, n, custom_allocator);
    else
	ans = R_allocOrReuseVector(s1, s2, INTSXP, n, custom_allocator);
    if (n == 0) return(ans);
    PROTECT(ans);

    switch (code) {
    case PLUSOP:
	{
	    int *pa = INTEGER(ans);
	    const int *px1 = INTEGER_RO(s1);
	    const int *px2 = INTEGER_RO(s2);
	    MOD_ITERATE2_CHECK(NINTERRUPT, n, n1, n2, i, i1, i2, {
		    x1 = px1[i1];
		    x2 = px2[i2];
		    pa[i] = R_integer_plus(x1, x2, &naflag);
		});
	    if (naflag)
		warningcall(lcall, INTEGER_OVERFLOW_WARNING);
	}
	break;
    case MINUSOP:
	{
	    int *pa = INTEGER(ans);
	    const int *px1 = INTEGER_RO(s1);
	    const int *px2 = INTEGER_RO(s2);
	    MOD_ITERATE2_CHECK(NINTERRUPT, n, n1, n2, i, i1, i2, {
		    x1 = px1[i1];
		    x2 = px2[i2];
		    pa[i] = R_integer_minus(x1, x2, &naflag);
		});
	    if (naflag)
		warningcall(lcall, INTEGER_OVERFLOW_WARNING);
	}
	break;
    case TIMESOP:
	{
	    int *pa = INTEGER(ans);
	    const int *px1 = INTEGER_RO(s1);
	    const int *px2 = INTEGER_RO(s2);
	    MOD_ITERATE2_CHECK(NINTERRUPT, n, n1, n2, i, i1, i2, {
		    x1 = px1[i1];
		    x2 = px2[i2];
		    pa[i] = R_integer_times(x1, x2, &naflag);
		});
	    if (naflag)
		warningcall(lcall, INTEGER_OVERFLOW_WARNING);
	}
	break;
    case DIVOP:
	{
	    double *pa = REAL(ans);
	    const int *px1 = INTEGER_RO(s1);
	    const int *px2 = INTEGER_RO(s2);
	    MOD_ITERATE2_CHECK(NINTERRUPT, n, n1, n2, i, i1, i2, {
		    x1 = px1[i1];
		    x2 = px2[i2];
		    pa[i] = R_integer_divide(x1, x2);
		});
	}
	break;
    case POWOP:
	{
	    double *pa = REAL(ans);
	    const int *px1 = INTEGER_RO(s1);
	    const int *px2 = INTEGER_RO(s2);
	    MOD_ITERATE2_CHECK(NINTERRUPT, n, n1, n2, i, i1, i2, {
		    if((x1 = px1[i1]) == 1 || (x2 = px2[i2]) == 0)
			pa[i] = 1.;
		    else if (x1 == NA_INTEGER || x2 == NA_INTEGER)
			pa[i] = NA_REAL;
		    else
			pa[i] = R_POW((double) x1, (double) x2);
		});
	}
	break;
    case MODOP:
	{
	    int *pa = INTEGER(ans);
	    const int *px1 = INTEGER_RO(s1);
	    const int *px2 = INTEGER_RO(s2);
	    MOD_ITERATE2_CHECK(NINTERRUPT, n, n1, n2, i, i1, i2, {
		    x1 = px1[i1];
		    x2 = px2[i2];
		    if (x1 == NA_INTEGER || x2 == NA_INTEGER || x2 == 0)
			pa[i] = NA_INTEGER;
		    else {
			pa[i] = /* till 0.63.2:	x1 % x2 */
			    (x1 >= 0 && x2 > 0) ? x1 % x2 :
			    (int)myfmod((double)x1,(double)x2);
		    }
		});
	}
	break;
    case IDIVOP:
	{
	    int *pa = INTEGER(ans);
	    const int *px1 = INTEGER_RO(s1);
	    const int *px2 = INTEGER_RO(s2);
	    MOD_ITERATE2_CHECK(NINTERRUPT, n, n1, n2, i, i1, i2, {
		    x1 = px1[i1];
		    x2 = px2[i2];
		    /* This had x %/% 0 == 0 prior to 2.14.1, but
		       it seems conventionally to be undefined */
		    if (x1 == NA_INTEGER || x2 == NA_INTEGER || x2 == 0)
			pa[i] = NA_INTEGER;
		    else
			pa[i] = (int) floor((double)x1 / (double)x2);
		});
	}
	break;
    }
    UNPROTECT(1);

    /* quick return if there are no attributes */
    if (ATTRIB(s1) == R_NilValue && ATTRIB(s2) == R_NilValue)
	return ans;

    /* Copy attributes from longer argument. */

    if (ans != s2 && n == n2 && ATTRIB(s2) != R_NilValue)
	copyMostAttrib(s2, ans);
    if (ans != s1 && n == n1 && ATTRIB(s1) != R_NilValue)
	copyMostAttrib(s1, ans); /* Done 2nd so s1's attrs overwrite s2's */

    return ans;
}


#define R_INTEGER(x) (double) ((x) == NA_INTEGER ? NA_REAL : (x))

static SEXP real_binary(ARITHOP_TYPE code, SEXP s1, SEXP s2,
		  	  	  	    R_allocator_t *custom_allocator)
{
    R_xlen_t i, i1, i2, n, n1, n2;
    SEXP ans;

    /* Note: "s1" and "s2" are protected above. */
    n1 = XLENGTH(s1);
    n2 = XLENGTH(s2);

    /* S4-compatibility change: if n1 or n2 is 0, result is of length 0 */
    if (n1 == 0 || n2 == 0) return(allocVector(REALSXP, 0));

    n = (n1 > n2) ? n1 : n2;
    PROTECT(ans = R_allocOrReuseVector(s1, s2, REALSXP, n, custom_allocator));

    switch (code) {
    case PLUSOP:
	if(TYPEOF(s1) == REALSXP && TYPEOF(s2) == REALSXP) {
	    double *da = REAL(ans);
	    const double *dx = REAL_RO(s1);
	    const double *dy = REAL_RO(s2);
	    if (n2 == 1) {
		double tmp = dy[0];
		R_ITERATE_CHECK(NINTERRUPT, n, i, da[i] = dx[i] + tmp;);
	    }
	    else if (n1 == 1) {
		double tmp = dx[0];
		R_ITERATE_CHECK(NINTERRUPT, n, i, da[i] = tmp + dy[i];);
	    }
	    else if (n1 == n2)
		R_ITERATE_CHECK(NINTERRUPT, n, i, da[i] = dx[i] + dy[i];);
	    else
		MOD_ITERATE2_CHECK(NINTERRUPT, n, n1, n2, i, i1, i2,
				  da[i] = dx[i1] + dy[i2];);
	}
	else if(TYPEOF(s1) == INTSXP ) {
	    double *da = REAL(ans);
	    const int *px1 = INTEGER_RO(s1);
	    const double *px2 = REAL_RO(s2);
	    MOD_ITERATE2_CHECK(NINTERRUPT, n, n1, n2, i, i1, i2,
			       da[i] = R_INTEGER(px1[i1]) + px2[i2];);
	}
	else if(TYPEOF(s2) == INTSXP ) {
	    double *da = REAL(ans);
	    const double *px1 = REAL_RO(s1);
	    const int *px2 = INTEGER_RO(s2);
	    MOD_ITERATE2_CHECK(NINTERRUPT, n, n1, n2, i, i1, i2,
			       da[i] = px1[i1] + R_INTEGER(px2[i2]););
	}
	break;
    case MINUSOP:
	if(TYPEOF(s1) == REALSXP && TYPEOF(s2) == REALSXP) {
	    double *da = REAL(ans);
	    const double *dx = REAL_RO(s1);
	    const double *dy = REAL_RO(s2);
	    if (n2 == 1) {
		double tmp = dy[0];
		R_ITERATE_CHECK(NINTERRUPT, n, i, da[i] = dx[i] - tmp;);
	    }
	    else if (n1 == 1) {
		double tmp = dx[0];
		R_ITERATE_CHECK(NINTERRUPT, n, i, da[i] = tmp - dy[i];);
	    }
	    else if (n1 == n2)
		R_ITERATE_CHECK(NINTERRUPT, n, i, da[i] = dx[i] - dy[i];);
	    else
		MOD_ITERATE2_CHECK(NINTERRUPT, n, n1, n2, i, i1, i2,
				  da[i] = dx[i1] - dy[i2];);
	}
	else if(TYPEOF(s1) == INTSXP ) {
	    double *da = REAL(ans);
	    const int *px1 = INTEGER_RO(s1);
	    const double *px2 = REAL_RO(s2);
	    MOD_ITERATE2_CHECK(NINTERRUPT, n, n1, n2, i, i1, i2,
			       da[i] = R_INTEGER(px1[i1]) - px2[i2];);
	}
	else if(TYPEOF(s2) == INTSXP ) {
	    double *da = REAL(ans);
	    const double *px1 = REAL_RO(s1);
	    const int *px2 = INTEGER_RO(s2);
	    MOD_ITERATE2_CHECK(NINTERRUPT, n, n1, n2, i, i1, i2,
			       da[i] = px1[i1] - R_INTEGER(px2[i2]););
	}
	break;
    case TIMESOP:
	if(TYPEOF(s1) == REALSXP && TYPEOF(s2) == REALSXP) {
	    double *da = REAL(ans);
	    const double *dx = REAL_RO(s1);
	    const double *dy = REAL_RO(s2);
	    if (n2 == 1) {
		double tmp = dy[0];
		R_ITERATE_CHECK(NINTERRUPT, n, i, da[i] = dx[i] * tmp;);
	    }
	    else if (n1 == 1) {
		double tmp = dx[0];
		R_ITERATE_CHECK(NINTERRUPT, n, i, da[i] = tmp * dy[i];);
	    }
	    else if (n1 == n2)
		R_ITERATE_CHECK(NINTERRUPT, n, i, da[i] = dx[i] * dy[i];);
	    else
		MOD_ITERATE2_CHECK(NINTERRUPT, n, n1, n2, i, i1, i2,
				  da[i] = dx[i1] * dy[i2];);
	}
	else if(TYPEOF(s1) == INTSXP ) {
	    double *da = REAL(ans);
	    const int *px1 = INTEGER_RO(s1);
	    const double *px2 = REAL_RO(s2);
	    MOD_ITERATE2_CHECK(NINTERRUPT, n, n1, n2, i, i1, i2,
			       da[i] = R_INTEGER(px1[i1]) * px2[i2];);
	}
	else if(TYPEOF(s2) == INTSXP ) {
	    double *da = REAL(ans);
	    const double *px1 = REAL_RO(s1);
	    const int *px2 = INTEGER_RO(s2);
	    MOD_ITERATE2_CHECK(NINTERRUPT, n, n1, n2, i, i1, i2,
			       da[i] = px1[i1] * R_INTEGER(px2[i2]););
	}
	break;
    case DIVOP:
	if(TYPEOF(s1) == REALSXP && TYPEOF(s2) == REALSXP) {
	    double *da = REAL(ans);
	    const double *dx = REAL_RO(s1);
	    const double *dy = REAL_RO(s2);
	    if (n2 == 1) {
		double tmp = dy[0];
		R_ITERATE_CHECK(NINTERRUPT, n, i, da[i] = dx[i] / tmp;);
	    }
	    else if (n1 == 1) {
		double tmp = dx[0];
		R_ITERATE_CHECK(NINTERRUPT, n, i, da[i] = tmp / dy[i];);
	    }
	    else if (n1 == n2)
		R_ITERATE_CHECK(NINTERRUPT, n, i, da[i] = dx[i] / dy[i];);
	    else
		MOD_ITERATE2_CHECK(NINTERRUPT, n, n1, n2, i, i1, i2,
				  da[i] = dx[i1] / dy[i2];);
	}
	else if(TYPEOF(s1) == INTSXP ) {
	    double *da = REAL(ans);
	    const int *px1 = INTEGER_RO(s1);
	    const double *px2 = REAL_RO(s2);
	    MOD_ITERATE2_CHECK(NINTERRUPT, n, n1, n2, i, i1, i2,
			       da[i] = R_INTEGER(px1[i1]) / px2[i2];);
	}
	else if(TYPEOF(s2) == INTSXP ) {
	    double *da = REAL(ans);
	    const double *px1 = REAL_RO(s1);
	    const int *px2 = INTEGER_RO(s2);
	    MOD_ITERATE2_CHECK(NINTERRUPT, n, n1, n2, i, i1, i2,
			       da[i] = px1[i1] / R_INTEGER(px2[i2]););
	}
	break;
    case POWOP:
	if(TYPEOF(s1) == REALSXP && TYPEOF(s2) == REALSXP) {
	    double *da = REAL(ans);
	    const double *dx = REAL_RO(s1);
	    const double *dy = REAL_RO(s2);
	    if (n2 == 1) {
		double tmp = dy[0];
		R_ITERATE_CHECK(NINTERRUPT, n, i, da[i] = R_POW(dx[i], tmp););
	    }
	    else if (n1 == 1) {
		double tmp = dx[0];
		R_ITERATE_CHECK(NINTERRUPT, n, i, da[i] = R_POW(tmp, dy[i]););
	    }
	    else if (n1 == n2)
		R_ITERATE_CHECK(NINTERRUPT, n, i, da[i] = R_POW(dx[i], dy[i]););
	    else
		MOD_ITERATE2_CHECK(NINTERRUPT, n, n1, n2, i, i1, i2,
				  da[i] = R_POW(dx[i1], dy[i2]););
	}
	else if(TYPEOF(s1) == INTSXP ) {
	    double *da = REAL(ans);
	    const int *px1 = INTEGER_RO(s1);
	    const double *px2 = REAL_RO(s2);
	    MOD_ITERATE2_CHECK(NINTERRUPT, n, n1, n2, i, i1, i2,
			       da[i] = R_POW( R_INTEGER(px1[i1]), px2[i2]););
	}
	else if(TYPEOF(s2) == INTSXP ) {
	    double *da = REAL(ans);
	    const double *px1 = REAL_RO(s1);
	    const int *px2 = INTEGER_RO(s2);
	    MOD_ITERATE2_CHECK(NINTERRUPT, n, n1, n2, i, i1, i2,
			       da[i] = R_POW(px1[i1], R_INTEGER(px2[i2])););
	}
	break;
    case MODOP:
	if(TYPEOF(s1) == REALSXP && TYPEOF(s2) == REALSXP) {
	    double *da = REAL(ans);
	    const double *px1 = REAL_RO(s1);
	    const double *px2 = REAL_RO(s2);
	    MOD_ITERATE2_CHECK(NINTERRUPT, n, n1, n2, i, i1, i2,
			       da[i] = myfmod(px1[i1], px2[i2]););
	}
	else if(TYPEOF(s1) == INTSXP ) {
	    double *da = REAL(ans);
	    const int *px1 = INTEGER_RO(s1);
	    const double *px2 = REAL_RO(s2);
	    MOD_ITERATE2_CHECK(NINTERRUPT, n, n1, n2, i, i1, i2,
			       da[i] = myfmod(R_INTEGER(px1[i1]), px2[i2]););
	}
	else if(TYPEOF(s2) == INTSXP ) {
	    double *da = REAL(ans);
	    const double *px1 = REAL_RO(s1);
	    const int *px2 = INTEGER_RO(s2);
	    MOD_ITERATE2_CHECK(NINTERRUPT, n, n1, n2, i, i1, i2,
			       da[i] = myfmod(px1[i1], R_INTEGER(px2[i2])););
	}
	break;
    case IDIVOP:
	if(TYPEOF(s1) == REALSXP && TYPEOF(s2) == REALSXP) {
	    double *da = REAL(ans);
	    const double *px1 = REAL_RO(s1);
	    const double *px2 = REAL_RO(s2);
	    MOD_ITERATE2_CHECK(NINTERRUPT, n, n1, n2, i, i1, i2,
			       da[i] = myfloor(px1[i1], px2[i2]););
	}
	else if(TYPEOF(s1) == INTSXP ) {
	    double *da = REAL(ans);
	    const int *px1 = INTEGER_RO(s1);
	    const double *px2 = REAL_RO(s2);
	    MOD_ITERATE2_CHECK(NINTERRUPT, n, n1, n2, i, i1, i2,
			       da[i] = myfloor(R_INTEGER(px1[i1]), px2[i2]););
	}
	else if(TYPEOF(s2) == INTSXP ) {
	    double *da = REAL(ans);
	    const double *px1 = REAL_RO(s1);
	    const int *px2 = INTEGER_RO(s2);
	    MOD_ITERATE2_CHECK(NINTERRUPT, n, n1, n2, i, i1, i2,
			       da[i] = myfloor(px1[i1], R_INTEGER(px2[i2])););
	}
	break;
    }
    UNPROTECT(1);

    /* quick return if there are no attributes */
    if (ATTRIB(s1) == R_NilValue && ATTRIB(s2) == R_NilValue)
	return ans;

    /* Copy attributes from longer argument. */

    if (ans != s2 && n == n2 && ATTRIB(s2) != R_NilValue)
	copyMostAttrib(s2, ans);
    if (ans != s1 && n == n1 && ATTRIB(s1) != R_NilValue)
	copyMostAttrib(s1, ans); /* Done 2nd so s1's attrs overwrite s2's */

    return ans;
}
SEXP /*attribute_hidden*/ complex_unary(ARITHOP_TYPE code, SEXP s1, SEXP call,
		                                R_allocator_t* custom_allocator)
{
    R_xlen_t i, n;
    SEXP ans;

    switch(code) {
    case PLUSOP:
	return s1;
    case MINUSOP:
	ans = NO_REFERENCES(s1) ? s1 : duplicate(s1); // FIXME
	Rcomplex *pans = COMPLEX(ans);
	const Rcomplex *ps1 = COMPLEX_RO(s1);
	n = XLENGTH(s1);
	for (i = 0; i < n; i++) {
	    Rcomplex x = ps1[i];
	    pans[i].r = -x.r;
	    pans[i].i = -x.i;
	}
	return ans;
    default:
	errorcall(call, _("invalid complex unary operator"));
    }
    return R_NilValue; /* -Wall */
}

SEXP /*attribute_hidden*/ complex_binary(ARITHOP_TYPE code, SEXP s1, SEXP s2,
									     R_allocator_t *custom_allocator)
{
    R_xlen_t i, i1, i2, n, n1, n2;
    SEXP ans;

    /* Note: "s1" and "s2" are protected in the calling code. */
    n1 = XLENGTH(s1);
    n2 = XLENGTH(s2);
     /* S4-compatibility change: if n1 or n2 is 0, result is of length 0 */
    if (n1 == 0 || n2 == 0) return(allocVector(CPLXSXP, 0));

    n = (n1 > n2) ? n1 : n2;
    ans = R_allocOrReuseVector(s1, s2, CPLXSXP, n, custom_allocator);
    PROTECT(ans);

    Rcomplex *pans = COMPLEX(ans);
    const Rcomplex *ps1 = COMPLEX_RO(s1);
    const Rcomplex *ps2 = COMPLEX_RO(s2);

    switch (code) {
    case PLUSOP:
	MOD_ITERATE2_CHECK(NINTERRUPT, n, n1, n2, i, i1, i2, {
	    Rcomplex x1 = ps1[i1];
	    Rcomplex x2 = ps2[i2];
	    pans[i].r = x1.r + x2.r;
	    pans[i].i = x1.i + x2.i;
	});
	break;
    case MINUSOP:
	MOD_ITERATE2_CHECK(NINTERRUPT, n, n1, n2, i, i1, i2, {
	    Rcomplex x1 = ps1[i1];
	    Rcomplex x2 = ps2[i2];
	    pans[i].r = x1.r - x2.r;
	    pans[i].i = x1.i - x2.i;
	});
	break;
    case TIMESOP:
	MOD_ITERATE2_CHECK(NINTERRUPT, n, n1, n2, i, i1, i2, {
	    SET_C99_COMPLEX(pans, i,
			    toC99(&ps1[i1]) * toC99(&ps2[i2]));
	});
	break;
    case DIVOP:
	MOD_ITERATE2_CHECK(NINTERRUPT, n, n1, n2, i, i1, i2, {
	    SET_C99_COMPLEX(pans, i,
			    toC99(&ps1[i1]) / toC99(&ps2[i2]));
	});
	break;
    case POWOP:
	MOD_ITERATE2_CHECK(NINTERRUPT, n, n1, n2, i, i1, i2, {
	    SET_C99_COMPLEX(pans, i,
			    mycpow(toC99(&ps1[i1]), toC99(&ps2[i2])));
	});
	break;
    default:
	error(_("unimplemented complex operation"));
    }
    UNPROTECT(1);

    /* quick return if there are no attributes */
    if (ATTRIB(s1) == R_NilValue && ATTRIB(s2) == R_NilValue)
	return ans;

    /* Copy attributes from longer argument. */

    if (ans != s2 && n == n2 && ATTRIB(s2) != R_NilValue)
	copyMostAttrib(s2, ans);
    if (ans != s1 && n == n1 && ATTRIB(s1) != R_NilValue)
	copyMostAttrib(s1, ans); /* Done 2nd so s1's attrs overwrite s2's */

    return ans;
}

// TODO
// - binary
// - duplicate
// - coerce
