/**
 * @file
 *
 * @date Nov 25, 2020
 * @author Attila Kovacs
 *
 * \brief
 *      A collection of commonly used functions for standard data exchange for scalars and arrays, and
 *      ASCII representations.
 *
 */

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>      // isspace()
#include <unistd.h>     // sleep()
#include <pthread.h>
#include <float.h>
#include <math.h>
#include <errno.h>

#define __XCHANGE_INTERNAL_API__      ///< Use internal definitions
#include "xchange.h"

#ifndef INFINITY
/// Check if we need to parse special floating point values, such as 'nan', 'infinity' or 'inf'...
/// These were added in the C99 standard, at the same time as the constant INFINITY was added.
#define EIXPLICIT_PARSE_SPECIAL_DOUBLES   TRUE
#else
/// Check if we need to parse special floating point values, such as 'nan', 'infinity' or 'inf'...
/// These were added in the C99 standard, at the same time as the constant INFINITY was added.
#define EIXPLICIT_PARSE_SPECIAL_DOUBLES   FALSE
#endif

boolean xVerbose;

#ifdef DEBUG
boolean xDebug = TRUE;
#else
boolean xDebug = FALSE;
#endif

/**
 * (<i>for internal use</i>) Propagates an error (if any). If the error is
 * non-zero, it returns with the offset error value. Otherwise it keeps going as if it weren't
 * even there...
 *
 * @param loc     Function [:location] where error was produced.
 * @param op      (optional) further info or NULL.
 * @param n       error code that was received.
 *
 * @return        n
 */
int x_trace(const char *loc, const char *op, int n) {
  if(n < 0 && xDebug) {
    fprintf(stderr, "       @ %s", loc);
    if(op) fprintf(stderr, " [%s]", op);
    fprintf(stderr, " [=> %d]\n", n);
  }
  return n;
}

/**
 * (<i>for internal use</i>) Traces an error before returning NULL.
 *
 * @param loc     Function [:location] where error was produced.
 * @param op      (optional) further info or NULL.
 * @return        NULL
 */
void *x_trace_null(const char *loc, const char *op) {
  if(xDebug) {
    fprintf(stderr, "       @ %s", loc);
    if(op) fprintf(stderr, " [%s]", op);
    fprintf(stderr, " [=> NULL]\n");
  }
  return NULL;
}

/**
 * (<i>for internal use</i>) Sets errno and reports errors to the standard error, depending
 * on the current debug mode, before returning the supplied return code.
 *
 * @param ret   return value
 * @param en    UNIX error code (see errno.h)
 * @param from  function (:location) where error originated
 * @param desc  description of error, with information to convey to user.
 *
 * @sa x_set_errno()
 * @sa x_trace()
 */
int x_error(int ret, int en, const char *from, const char *desc, ...) {
  va_list varg;

  va_start(varg, desc);
  if(xDebug) {
    fprintf(stderr, "\n  ERROR! %s: ", from);
    vfprintf(stderr, desc, varg);
    if(ret) fprintf(stderr, " [=> %d]\n", ret);
  }
  va_end(varg);

  errno = en;
  return ret;
}

/**
 * (<i>for internal use</i>) Prints a warning message.
 *
 * @param from  function (:location) where error originated
 * @param desc  description of error, with information to convey to user.
 *
 * @sa x_set_errno()
 * @sa x_trace()
 */
int x_warn(const char *from, const char *desc, ...) {
  va_list varg;

  va_start(varg, desc);
  if(xDebug) {
    fprintf(stderr, "\n  WARNING! %s: ", from);
    vfprintf(stderr, desc, varg);
  }
  va_end(varg);

  return 0;
}


/**
 * Checks if verbosity is enabled for the xchange library.
 *
 * @return    TRUE (1) if verbosity is enabled, or else FALSE (0).
 *
 * @sa xSetVerbose()
 * @sa xSetDebug()
 */
boolean xIsVerbose() {
  return xVerbose;
}

/**
 * Sets verbose output for the xchange library.
 *
 * @param value   TRUE (non-zero) to enable verbose output, or else FALSE (0).
 *
 * @sa xIsVerbose()
 */
void xSetVerbose(boolean value) {
  xVerbose = value ? TRUE : FALSE;
}

/**
 * Enables or disables debugging output.
 *
 * @param value   TRUE (non-zero) to enable verbose output, or else FALSE (0).
 *
 * @sa xSetVerbose()
 */
void xSetDebug(boolean value) {
  xDebug = value ? TRUE : FALSE;
}

/**
 * Checks if the type represents a fixed-size character / binary sequence.
 *
 * \param type      xchange type to check.
 *
 * \return          TRUE (1) if it is a type for a (fixed size) character array, otherwise FALSE (0).
 *
 */
boolean xIsCharSequence(XType type) {
  return type < 0;
}

/**
 * Checks if the type represents a signed integer value of any width.
 *
 * @param type    xchange type to check.
 * @return        TRUE (1) if the type is for an integer value, or else FALSE (0)
 *
 * @sa xIsDecimal()
 * @sa xIsNumeric()
 * @sa xGetAsLong()
 */
boolean xIsInteger(XType type) {
  switch(type) {
    case X_BOOLEAN:
      case X_BYTE:
      case X_INT16:
      case X_INT32:
      case X_INT64:
        return TRUE;
      default:
        return FALSE;
  }
}

/**
 * Checks if the type represents a floating-point value of any width.
 *
 * @param type    xchange type to check.
 * @return        TRUE (1) if the type is for a floating-point value, or else FALSE (0)
 *
 * @sa xIsInteger()
 * @sa xIsNumeric()
 * @sa xGetAsDouble()
 */
boolean xIsDecimal(XType type) {
  return (type == X_FLOAT || type == X_DOUBLE);
}

/**
 * Checks if the type represents a numerical value.
 *
 * @param type    xchange type to check.
 * @return        TRUE (1) if the type is for a number value, or else FALSE (0)
 *
 * @sa xIsInteger()
 * @sa xIsDecimal()
 */
boolean xIsNumeric(XType type) {
  return (xIsInteger(type) || xIsDecimal(type));
}

static int xStringSizeForIntBytes(int bytes) {
  switch(bytes) {
    case 1: return 4;   // -128
    case 2: return 6;   // -32768
    case 4: return 11;  // -2147483647
    case 8: return 19;  // −9223372036854775807
    default : return x_error(-1, EINVAL, "xStringSizeForIntBytes", "invalid bytes: %d", bytes);
  }
}

/**
 * Returns the number of characters, including a '\0' termination that a single element of the
 * might be expected to fill.
 *
 * \param type      X-Change type to check.
 *
 * \return      Number of characters (including termination) required for the string representation
 *              of an element of the given variable, or 0 if the variable is of unknown type.
 */
int xStringElementSizeOf(XType type) {
  int l = 0; // default

  if(type < 0) l = -type;
  else switch(type) {
    case X_BOOLEAN: l = 5; break;    // "false"
    case X_BYTE: return xStringSizeForIntBytes(sizeof(char));
    case X_INT16: return xStringSizeForIntBytes(sizeof(int16_t));
    case X_INT32: return xStringSizeForIntBytes(sizeof(int32_t));
    case X_INT64: return xStringSizeForIntBytes(sizeof(int64_t));
    case X_FLOAT : l = 16; break;     // 1 leading + 8 significant figs + 2 signs + 1 dot + 1 E + 3 exponent
    case X_DOUBLE : l = 25; break;    // 1 leading + 16 significant figs + (4) + 4 exponent
    default : return x_error(-1, EINVAL, "xStringElementSizeOf", "invalid type: %d", type);
  }
  return (l+1); // with terminating '\0'
}

/**
 * Returns the storage byte size of a single element of a given type.
 *
 * @param type    The data type, as defined in 'xchange.h'
 *
 * @return [bytes] the native storage size of a single element of that type. E.g. for X_CHAR(20) it will
 *         return 20. X_DOUBLE will return 8, etc. Unrecognised types will return 0.
 *
 */
int xElementSizeOf(XType type) {
  if(type < 0) return -type;
  switch(type) {
    case X_RAW: return sizeof(char *);
    case X_BOOLEAN: return sizeof(boolean);
    case X_BYTE: return sizeof(char);
    case X_INT16: return sizeof(int16_t);
    case X_INT32: return sizeof(int32_t);
    case X_INT64: return sizeof(int64_t);
    case X_FLOAT: return sizeof(float);
    case X_DOUBLE: return sizeof(double);
    case X_STRUCT: return sizeof(XStructure);
    case X_STRING: return sizeof(char *);
    case X_FIELD: return sizeof(XField);
  }

  return 0;
}

/**
 * Returns the character of the field type. For X_CHAR types it returns 'C' (without the length
 * specification), and for all other
 * types it returns the constant XType value itself.
 *
 * @param type    The single-character IF of the field type.
 * @return        A character that represented the type.
 */
char xTypeChar(XType type) {
  if(type < 0) return 'C';
  if(type < 0x20 || type > 0x7E) return (char) x_error('?', EINVAL, "xTypeChar", "invalid type: %d", type);
  return (char) (type & 0xff);
}

/**
 * Returns the total element count specified by along a number of dimensions. It ignores dimensions
 * that have size components <= 0;
 *
 * \param ndim      Number of dimensions
 * \param sizes     Sizes along each dimension.
 *
 * \return          Total element count specified by the dimensions. Defaults to 1.
 */
long xGetElementCount(int ndim, const int *sizes) {
  int i;
  long N = 1L;

  if(ndim > 0 && !sizes)
    return x_error(1, EINVAL, "xGetElementCount", "input 'sizes' is NULL (ndim = %d)", ndim);

  if(ndim > X_MAX_DIMS) ndim = X_MAX_DIMS;
  for(i = 0; i < ndim; i++) N *= sizes[i];
  return N;
}

/**
 * Serializes the dimensions to a string as a space-separated list of integers.
 *
 * \param[out]  dst       Pointer to a string buffer with at least X_MAX_STRING_DIMS bytes size.
 * \param[in]   ndim      Number of dimensions
 * \param[in]   sizes     Sizes along each dimension.
 *
 * \return          Number of characters written into the destination buffer, not counting the string
 *                  termination, or -1 if an the essential pointer arguments is NULL.
 *
 */
int xPrintDims(char *dst, int ndim, const int *sizes) {
  static const char *fn = "xPrintDims";

  int i, N=0;
  char *next = dst;

  if(!dst)
    return x_error(-1, EINVAL, fn, "'dst' is NULL");

  if(ndim > 0 && !sizes)
    return x_error(-1, EINVAL, fn, "input 'sizes' is NULL (ndim = %d)", ndim);

  if(ndim <= 0) return sprintf(dst, "1");           // default, will be overwritten with actual sizes, if any.
  else if(ndim > X_MAX_DIMS) ndim = X_MAX_DIMS;

  for(i=0; i<ndim; i++) {
    if(sizes[i] <= 0) continue;                     // Ignore 0 or negative dimensions
    next += sprintf(next, "%d ", sizes[N++]);       // Print the next dimension
  }

  if(next > dst) next--;
  *next = '\0';    // Replace the last space with a string termination.

  return next - dst;
}


/**
 * Deserializes the sizes from a space-separated list of dimensions. The parsing will terminate at the first non integer
 * value or the end of string, whichever comes first. Integer values <= 0 are ignored.
 *
 * \param src       Pointer to a string buffer that contains the serialized dimensions, as a list of space separated integers.
 * \param sizes     Pointer to an array of ints (usually of X_MAX_DIMS size) to which the valid dimensions are deserialized.
 *
 * \return          Number of valid (i.e. positive) dimensions parsed.
 */
int xParseDims(const char *src, int *sizes) {
  static const char *fn = "xParseDims";

  int ndim;
  char *next = (char *) src;

  if(!src) return x_error(0, EINVAL, fn, "'src' is NULL");
  if(!sizes) return 0;

  for(ndim = 0; ndim <= X_MAX_DIMS; ) {
    char *from = next;

    errno = 0;
    *sizes = (int) strtol(from, &next, 10);
    if(next == from) break;     // Empty string
    if(errno) {
      x_error(0, errno, fn, "invalid dimension: %s\n", from);
      break;
    }

    if(*sizes <= 0) continue;   // ignore 0 or negative sizes.

    sizes++;                    // move to the next dimension
    ndim++;
  }

  return ndim;
}


/**
 * Allocates a buffer for a given SMA-X type and element count. The buffer is initialized
 * with zeroes.
 *
 * \param type      SMA-X type
 * \param count     number of elements.
 *
 * \return      Pointer to the initialized buffer or NULL if there was an error (errno will
 *              be set accordingly).
 */
void *xAlloc(XType type, int count) {
  static const char *fn = "xAlloc";
  int eSize;
  void *ptr;

  if(count < 1) {
    x_error(0, EINVAL, fn, "invalid count=%d", count);
    return NULL;
  }

  eSize = xElementSizeOf(type);
  if(eSize < 1)
    return x_trace_null(fn, NULL);

  ptr = calloc(eSize, count);
  if(!ptr) x_error(0, errno, fn, "calloc() error");

  return ptr;
}

/**
 * Zeroes out the contents of an SMA-X buffer.
 *
 * \param buf       Pointer to the buffer to fill with zeroes.
 * \param type      SMA-X type
 * \param count     number of elements.
 */
void xZero(void *buf, XType type, int count) {
  size_t eSize = xElementSizeOf(type);
  if(!buf) return;
  if(eSize < 1) return;
  if(count < 1) return;
  memset(buf, 0, eSize * count);
}

/**
 * Returns a freshly allocated string with the same content as the argument.
 *
 * \param str       Pointer to string we want to copy.
 *
 * \return          A copy of the supplied string, or NULL if the argument itself was NULL.
 *
 */
char *xStringCopyOf(const char *str) {
  char *copy;
  int n;

  if(str == NULL) return NULL;

  n = strlen(str) + 1;
  copy = (char *) malloc(n);
  x_check_alloc(copy);

  memcpy(copy, str, n);
  return copy;
}

static int TokenMatch(char *a, char *b) {
  // Check characters...
  for(; *a && *b; a++, b++) if(*a != *b) {
    if(isspace(*a)) return isspace(*b);
    else if(isspace(*b)) return FALSE;
    if(*a >= 'a' && *a <= 'z') if(toupper(*a) != *b) return FALSE;
    if(*b >= 'a' && *b <= 'z') if(toupper(*b) != *a) return FALSE;
  }
  return *a == *b;
}

/**
 * Parses a boolean value, either as a zero/non-zero number or as a case-insensitive
 * match to the next token to one of the recognized boolean terms, such as
 * "true"/"false", "on"/"off", "yes"/"no", "t"/"f", "y"/"n", "enabled"/"disabled" or "active"/"inactive".
 * If a boolean value cannot be matched, FALSE is returned, and errno is set to ERANGE.
 *
 * @param str   Pointer to the string token.
 * @param end   Where the pointer to after the successfully parsed token is returned, on NULL.
 * @return      TRUE (1) or FALSE (0).
 */
boolean xParseBoolean(char *str, char **end) {
  static const char *fn = "xParseBoolean";

  static char *trues[] = { "true", "t", "on", "yes", "y", "enabled", "active", NULL };
  static char *falses[] = { "false", "f", "off", "no", "n", "disabled", "invactive", NULL };

  int i;
  long l;

  if(!str) {
    if(end) *end = NULL;
    return x_error(FALSE, EINVAL, fn, "input string is NULL");
  }

  while(isspace(*str)) str++;

  if(end) *end = str;

  for(i = 0; trues[i]; i++) if(TokenMatch(str, trues[i])) {
    if(end) *end = str + strlen(trues[i]);
    return TRUE;
  }

  for(i = 0; falses[i]; i++) if(TokenMatch(str, falses[i])) {
    if(end) *end = str + strlen(falses[i]);
    return FALSE;
  }

  /* Try parse as a number... */
  errno = 0;
  l = strtol(str, end, 0);
  if(errno) x_error(FALSE, errno, fn, "invalid argument: %s", str);

  return (l != 0);
}


#if EXPLCIT_PARSE_SPECIAL_DOUBLES
static int CompareToken(const char *a, const char *b) {
  int i;

  if(a == b) return 0;                      // both point to the same string (or NULL)

  if(!a || !b) return b > a ? 1 : -1;       // one is NULL

  for(i=0; a[i] && !isspace(a[i]); i++) {
    char A = tolower(a[i]);
    char B = tolower(b[i]);

    if(A == B) continue;                    // so far so good. Keep going...

    if(!B) return -1;                       // B is shorter...
    return B > A ? 1 : -1;                  // they differ...
  }

  if(b[i] && !isspace(b[i])) return 1;      // B is longer...
  return 0;                                 // OK, they are the same up.
}
#endif

/**
 * Same as strtod() on C99, but with explicit parsing of NaN and Infinity values on older platforms also.
 *
 * @param str       String to parse floating-point value from
 * @param tail      (optional) reference to pointed in which to return the parse position after successfully
 *                  parsing a floating-point value.
 * @return          the floating-point value at the head of the string, or NAN if the input string is NULL.
 */
double xParseDouble(const char *str, char **tail) {
  if(!str) {
    x_error(0, EINVAL, "xParseDouble", "input string is NULL");
    return NAN;
  }

#if EXPLCIT_PARSE_SPECIAL_DOUBLES
  {
    char *next = (char *) str;
    int sign = 1;

    while(isspace(*next)) next++;

    if(*next == '+') next++;          // Skip sign (if any)
    else if(*next == '-') {
      sign = -1;
      next++;          // Skip sign (if any)
    }

    // If leading character is not a number or a decimal point, then check for special values.
    if(*next) if((*next < '0' || *next > '9') && *next != '.') {
      if(!CompareToken("nan", next)) {
        if(tail) *tail = next + sizeof("nan") - 1;
        return NAN;
      }
      if(!CompareToken("inf", next)) {
        if(tail) *tail = next + sizeof("inf") - 1;
        // cppcheck-suppress nanInArithmeticExpression
        return sign > 0 ? INFINITY : -INFINITY;
      }
      if(!CompareToken("infinity", next)) {
        if(tail) *tail = next + sizeof("infinity") - 1;
        // cppcheck-suppress nanInArithmeticExpression
        return sign > 0 ? INFINITY : -INFINITY;
      }
    }
  }
#endif

  errno = 0;
  return strtod(str, tail);
}

/**
 * Prints a double precision number, restricted to legal double-precision range. If the native
 * value has abolute value smaller than the smallest non-zero value, then 0 will printed instead.
 * For values that exceed the legal double precision range, "-inf" or "inf" will be used as
 * appropriate, and NAN values will be printed as "nan".
 *
 * @param str       Pointer to buffer for printed value. It should have at least 25 bytes of
 *                  space allocated after the specidied address.
 * @param value     Value to print.
 * @return          Number of characters printed into the buffer, or -1 if there was an error.
 */
int xPrintDouble(char *str, double value) {
  if(!str)
    return x_error(-1, EINVAL, "xPrintDouble", "input string is NULL");

  // For double-precision restrict range to IEEE range
  if(value > DBL_MAX) return sprintf(str, "inf");
  else if(value < -DBL_MAX) return sprintf(str, "-inf");
  else if(isnan(value)) return sprintf(str, "nan");
  else if(value < DBL_MIN) if(value > -DBL_MIN) return sprintf(str, "0");

  return sprintf(str, "%.16g", value);
}

/**
 * Prints a single-precision number, restricted to the legal single-precision range. If the native
 * value has abolute value smaller than the smallest non-zero value, then 0 will printed instead.
 * For values that exceed the legal double precision range, "-inf" or "inf" will be used as
 * appropriate, and NAN values will be printed as "nan".
 *
 * @param str       Pointer to buffer for printed value. It should have at least 16 bytes of
 *                  space allocated after the specified address.
 * @param value     Value to print.
 * @return          Number of characters printed into the buffer.
 */
int xPrintFloat(char *str, float value) {
  if(!str)
    return x_error(-1, EINVAL, "xPrintFloat", "input string is NULL");

  // For single-precision restrict range to IEEE range
  if(value > FLT_MAX) return sprintf(str, "inf");
  else if(value < -FLT_MAX) return sprintf(str, "-inf");
  else if(isnan(value)) return sprintf(str, "nan");
  else if(value < FLT_MIN) if(value > -FLT_MIN) return sprintf(str, "0");

  return sprintf(str, "%.8g", value);
}

/**
 * Prints a descriptive error message to stderr, and returns the error code.
 *
 * \param fn      String that describes the function or location where the error occurred.
 * \param code    The xchange error code that describes the failure (see xchange.h).
 *
 * \return          Same error code as specified on input.
 */
int xError(const char *fn, int code) {
  switch(code) {
    case X_SUCCESS: return 0;
    case X_FAILURE: return x_error(code, ECANCELED, fn, "internal failure");
    case X_ALREADY_OPEN: return x_error(code, EISCONN, fn, "already opened");
    case X_NO_INIT: return x_error(code, ENXIO, fn, "not initialized");
    case X_NO_SERVICE: return x_error(code, EIO, fn, "connection lost?");
    case X_NO_BLOCKED_READ: return x_error(code, 0, fn, "no blocked calls");
    case X_NO_PIPELINE: return x_error(code, EPIPE, fn, "pipeline mode disabled");
    case X_TIMEDOUT: return x_error(code, ETIMEDOUT, fn, "timed out while waiting to complete");
    case X_INTERRUPTED: return x_error(code, EINTR, fn, "wait released without read");
    case X_INCOMPLETE: return x_error(code, EAGAIN, fn, "incomplete data transfer");
    case X_NULL: return x_error(code, EFAULT, fn, "null value");
    case X_PARSE_ERROR: return x_error(code, ECANCELED, fn, "parse error");
    case X_NOT_ENOUGH_TOKENS: return x_error(code, EINVAL, fn, "not enough tokens");
    case X_GROUP_INVALID: return x_error(code, EINVAL, fn, "invalid group");
    case X_NAME_INVALID: return x_error(code, EINVAL, fn, "invalid variable name");
    case X_TYPE_INVALID: return x_error(code, EINVAL, fn, "invalid variable type");
    case X_SIZE_INVALID: return x_error(code, EDOM, fn, "invalid variable dimensions");
  }
  return x_error(code, errno, fn, "unknown error");
}

/**
 * Returns a string description for one of the standard X-change error codes, and sets
 * errno as appropriate also. (The mapping to error codes is not one-to-one. The same
 * errno may be used to describe different X-change errors. Nevertheless, it is a guide
 * that can be used when the X-change error is not directtly available, e.g. because
 * it is not returned by a given function.)
 *
 * \param code      One of the error codes defined in 'xchange.h'
 *
 * \return      A constant string with the error description.
 *
 */
const char *xErrorDescription(int code) {
  switch(code) {
    case X_FAILURE: return "internal failure";
    case X_SUCCESS: return "success!";
    case X_ALREADY_OPEN: return "already opened";
    case X_NO_INIT: return "not initialized";
    case X_NO_SERVICE: return "connection lost?";
    case X_NO_BLOCKED_READ: return "no blocked calls";
    case X_NO_PIPELINE: return "pipeline mode disabled";
    case X_TIMEDOUT: return "timed out while waiting to complete";
    case X_INTERRUPTED: return "wait released without read";
    case X_INCOMPLETE: return "incomplete data transfer";
    case X_NULL: return "null value";
    case X_PARSE_ERROR: return "parse error";
    case X_NOT_ENOUGH_TOKENS: return "not enough tokens";
    case X_GROUP_INVALID: return "invalid group";
    case X_NAME_INVALID: return "invalid variable name";
    case X_TYPE_INVALID: return "invalid variable type";
    case X_SIZE_INVALID: return "invalid variable dimensions";
  }
  return "unknown error";
}


