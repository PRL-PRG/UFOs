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

static R_INLINE int R_integer_plus(int x, int y, Rboolean *pnaflag)
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

static R_INLINE int R_integer_minus(int x, int y, Rboolean *pnaflag)
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
static R_INLINE int R_integer_times(int x, int y, Rboolean *pnaflag)
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

static R_INLINE double R_integer_divide(int x, int y)
{
    if (x == NA_INTEGER || y == NA_INTEGER)
	return NA_REAL;
    else
	return (double) x / (double) y;
}

double R_pow(double x, double y);

static R_INLINE double R_POW(double x, double y) /* handle x ^ 2 inline */
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
	}
}

//----------------------------------------------------------------------------
// In this section changes happen.
//
// Relevant R files: arithmetic. complex.c
//----------------------------------------------------------------------------

SEXP R_unary(SEXP, SEXP, SEXP, R_allocator_t*);
//SEXP R_binary(SEXP, SEXP, SEXP, SEXP, R_allocator_t*);
static SEXP logical_unary(ARITHOP_TYPE, SEXP, SEXP, R_allocator_t*);
static SEXP integer_unary(ARITHOP_TYPE, SEXP, SEXP, R_allocator_t*);
static SEXP real_unary(ARITHOP_TYPE, SEXP, SEXP, R_allocator_t*);
static SEXP real_binary(ARITHOP_TYPE, SEXP, SEXP, R_allocator_t*);
static SEXP integer_binary(ARITHOP_TYPE, SEXP, SEXP, SEXP, R_allocator_t*);

SEXP complex_unary(ARITHOP_TYPE code, SEXP s1, SEXP call, R_allocator_t*);

/* for binary operations */
/* adapted from Radford Neal's pqR */
static R_INLINE SEXP R_allocOrReuseVector(SEXP s1, SEXP s2,
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

// TODO
// - binary
// - duplicate
