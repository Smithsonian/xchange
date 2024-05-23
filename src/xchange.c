/**
 * @file
 *
 * @date Nov 25, 2020
 * @author Attila Kovacs
 *
 * \brief
 *      A collection of commonly used functions for standard ASCII string based data exchange.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>      // isspace()
#include <unistd.h>     // sleep()
#include <pthread.h>
#include <float.h>
#include <math.h>
#include <errno.h>

#include "xchange.h"

// Check if we need to parse special floating point values, such as 'nan', 'infinity' or 'inf'...
// These were added in the C99 standard, at the same time as the constant INFINITY was added.
#ifndef INFINITY
#define EXPLCIT_PARSE_SPECIAL_DOUBLES   TRUE
#else
#define EXPLCIT_PARSE_SPECIAL_DOUBLES   FALSE
#endif

boolean xVerbose;
boolean xDebug;

boolean xIsVerbose() {
  return xVerbose;
}

void xSetVerbose(boolean value) {
  xVerbose = value ? TRUE : FALSE;
}

/**
 * Checks if the type represents a (fixed size) character sequence.
 *
 * \param type      X-Change type to check.
 *
 * \return          TRUE if it is a type for a (fixed size) character array, otherwise FALSE.
 *
 */
boolean xIsCharSequence(XType type) {
  return type < 0;
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
    case X_BOOLEAN : l = 5; break;    // "false"
    case X_BYTE : l = 4; break;       // -255
    case X_BYTE_HEX : l = 4; break;   // 0xff
    case X_SHORT : l = 6; break;      // -65536
    case X_SHORT_HEX : l = 6; break;  // 0xffff
    case X_INT : l = 11; break;       // -2147483647
    case X_INT_HEX : l = 10; break;   // 0xffffffff
    case X_LONG : l = 19; break;
    case X_LONG_HEX : l = 18; break;
    case X_FLOAT : l = 16; break;     // 1 leading + 8 significant figs + 2 signs + 1 dot + 1 E + 3 exponent
    case X_DOUBLE : l = 25; break;    // 1 leading + 16 significant figs + (4) + 4 exponent
    default : return -1;
  }
  return (l+1); // with terminating '\0'
}


/**
 * Returns the storage byte size of a single element of a given type.
 *
 */
int xElementSizeOf(XType type) {
  if(type < 0) return -type;
  switch(type) {
    case X_RAW: return sizeof(char *);
    case X_BYTE:
    case X_BYTE_HEX: return 1;
    case X_SHORT:
    case X_SHORT_HEX: return sizeof(short);
    case X_BOOLEAN: return sizeof(boolean);
    case X_INT:
    case X_INT_HEX: return sizeof(int);
    case X_LONG:
    case X_LONG_HEX: return sizeof(long long);
    case X_FLOAT: return sizeof(float);
    case X_DOUBLE: return sizeof(double);
    case X_STRUCT: return sizeof(XStructure);
    case X_STRING: return sizeof(char *);
  }
  return 0;
}



/**
 * Returns the XType for a given case-sensitive type string. For example "float" -> X_FLOAT. The value "raw" will
 * return X_RAW.
 *
 * \param type      String type, e.g. "struct".
 *
 * \return          Corresponding XType, e.g. X_STRUCT. (The default return value is X_RAW, since all Redis
 *                  values can be represented as raw strings.)
 *
 */
XType xTypeForString(const char *type) {
  if(!type) return X_RAW;
  if(!strcmp("int", type) || !strcmp("integer", type)) return X_INT;
  if(!strcmp("boolean", type) || !strcmp("bool", type)) return X_BOOLEAN;
  if(!strcmp("int8", type)) return X_BYTE;
  if(!strcmp("int16", type)) return X_SHORT;
  if(!strcmp("int32", type)) return X_INT;
  if(!strcmp("int64", type)) return X_LONG;
  if(!strcmp("float", type)) return X_FLOAT;
  if(!strcmp("float32", type)) return X_FLOAT;
  if(!strcmp("float64", type)) return X_DOUBLE;
  if(!strcmp("double", type)) return X_DOUBLE;
  if(!strcmp("string", type) || !strcmp("str", type)) return X_STRING;
  if(!strcmp("struct", type)) return X_STRUCT;
  if(!strcmp("raw", type)) return X_RAW;
  return X_UNKNOWN;
}

char xTypeChar(XType type) {
  if(type < 0) return 'C';
  if(type < 0x20) return '?';
  if(type > 0x7E) return '?';
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
int xGetElementCount(int ndim, const int *sizes) {
  int i, N = 1;
  if(ndim > X_MAX_DIMS) ndim = X_MAX_DIMS;
  for(i = 0; i < ndim; i++) N *= sizes[i];
  return N;
}

/**
 * Serializes the dimensions to a string.
 *
 * \param[out]  dst       Pointer to a string buffer with at least X_MAX_STRING_DIMS bytes size.
 * \param[in]   ndim      Number of dimensions
 * \param[in]   sizes     Sizes along each dimension.
 *
 * \return          Number of characters written into the destonation buffer, not counting the string
 *                  termination.
 *
 */
int xPrintDims(char *dst, int ndim, const int *sizes) {
  int i, N=0;
  char *next = dst;

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
 * Deserializes the sizes from a multi-dimensional specification. The parsing will terminate at the first non integer
 * value or the end of string, whichever comes first. Integer values <= 0 are ignored.
 *
 * \param src       Pointer to a string buffer that contains the serialized dimensions, as a list of space separated integers.
 * \param sizes     Pointer to an array of ints (usually of X_MAX_DIMS size) to which the valid dimensions are deserialized.
 *
 * \return          Number of valid (i.e. positive) dimensions parsed.
 */
int xParseDims(const char *src, int *sizes) {
  int ndim, N = 1;
  char *next = (char *) src;

  if(!src) return 0;
  if(!sizes) return 0;

  for(ndim = 0; ndim <= X_MAX_DIMS; ) {
    char *from = next;

    *sizes = strtol(from, &next, 10);
    if(next == from) break;
    if(errno == ERANGE) break;

    if(*sizes <= 0) continue;  // ignore 0 or negative sizes.

    N *= *sizes;

    sizes++;            // move to the next dimension
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
  int eSize = xElementSizeOf(type);
  if(eSize < 1 || count < 1) {
    errno = EINVAL;
    return NULL;
  }
  return calloc(eSize, count);
}


/**
 * Zeroes out the contents of an SMA-X buffer.
 *
 * \param buf       Pointer to the buffer to fill with zeroes.
 * \param type      SMA-X type
 * \param count     number of elements.
 */
void xZero(void *buf, XType type, int count) {
  int eSize = xElementSizeOf(type);
  if(!buf) return;
  if(eSize < 1) return;
  if(count < 1) return;
  memset(buf, 0, eSize * count);
}


/**
 * Returns a pointer to the beginning of the next component in a compound ID. Leading ID separators are ignored.
 *
 * @param id    Aggregate X ID.
 * @return      Pointer to the start of the next compound ID token, or NULL if there is no more components in the ID.
 */
char *xNextIDToken(const char *id) {
  char *next;

  if(!id) return NULL;

  // Ignore leading separator.
  if(!strncmp(id, X_SEP, X_SEP_LENGTH)) id += X_SEP_LENGTH;

  next = strstr(id, X_SEP);
  return next ? next + X_SEP_LENGTH : NULL;
}

/**
 * Returns a copy of the next next component in a compound ID. Leading ID separators are ignored.
 *
 * @param id    Aggregate X ID.
 * @return      Pointer to the start of the next compound ID token, or NULL if there is no more components in the ID.
 */
char *xCopyIDToken(const char *id) {
  const char *next;
  char *token;
  int l;

  if(!id) return NULL;

  // Ignore leading separator.
  if(!strncmp(id, X_SEP, X_SEP_LENGTH)) id += X_SEP_LENGTH;

  next = xNextIDToken(id);
  if(next) l = next - id - X_SEP_LENGTH;
  else l = strlen(id);

  token = malloc(l+1);
  if(!token) return NULL;

  memcpy(token, id, l);
  token[l] = '\0';

  return token;
}

/**
 * Checks if the next component in a compound id matches a given token.
 *
 * @param token     Full token to check for
 * @param id        Compount X ID.
 * @return          X_SUCCESS if it's a match. Otherwise X_FAILURE or another X error if the
 *                  arguments are invalid.
 */
int xMatchNextID(const char *token, const char *id) {
  int L;

  if(!token) return X_NULL;
  if(!id) return X_NULL;
  if(*token == '\0') return X_NAME_INVALID;
  if(*id == '\0') return X_GROUP_INVALID;


  L = strlen(token);
  if(strncmp(id, token, L)) return X_FAILURE;

  if(id[L] == '\0') return X_SUCCESS;

  if(strlen(id) < L + X_SEP_LENGTH) return X_FAILURE;
  if(strncmp(id + L, X_SEP, X_SEP_LENGTH)) return X_FAILURE;

  return X_SUCCESS;
}


/**
 * Returns the aggregated (hierarchical) &lt;table&gt;:&lt;key&gt; ID. The caller is responsible for calling free()
 * on the returned string after use.
 *
 * \param table     SMA-X hastable name
 * \param key       The lower-level id to concatenate.
 *
 * \return          The aggregated ID, or NULL if both arguments were NULL themselves.
 *
 */
char *xGetAggregateID(const char *table, const char *key) {
  char *id;

  if(table == NULL && key == NULL) return NULL;

  if(table == NULL) return xStringCopyOf(key);
  if(key == NULL) return xStringCopyOf(table);

  id = (char *) malloc(strlen(table) + strlen(key) + X_SEP_LENGTH + 1); // <group>:<key>
  if(!id) return NULL;
  sprintf(id, "%s" X_SEP "%s", table, key);

  return id;
}

/**
 * Returns the string pointer to the begining of the last separator in the ID.
 *
 * @param id    Compound SMA-X ID.
 * @return      Pointer to the beginning of the last separator in the ID, or NULL if the ID does not contain a separator.
 */
char *xLastSeparator(const char *id) {
  int l = strlen(id);

  while(--l >= 0) if(id[l] == X_SEP[0]) if(!strncmp(&id[l], X_SEP, X_SEP_LENGTH)) return (char *) &id[l];
  return NULL;
}


/**
 * Returns the string type for a given XType argument as a constant expression. For examples X_LONG -> "int64".
 *
 * \param type      SMA-X type, e.g. X_FLOAT
 *
 * \return          Corresponding string type, e.g. "float". (Default is "string" -- since typically
 *                  anything can be represented as strings.)
 *
 */
char *xStringType(XType type) {
  if(type < 0) return "string";         // X_CHAR(n), legacy fixed size strings.

  switch(type) {
    case X_BOOLEAN: return "boolean";
    case X_BYTE:
    case X_BYTE_HEX: return "int8";
    case X_SHORT:
    case X_SHORT_HEX: return "int16";
    case X_INT:
    case X_INT_HEX: return "int32";
    case X_LONG:
    case X_LONG_HEX: return "int64";
    case X_FLOAT: return "float";
    case X_DOUBLE: return "double";
    case X_STRING: return "string";
    case X_RAW: return "raw";
    case X_STRUCT: return "struct";
    case X_UNKNOWN:
    default: return "unknown";
  }
}



/**
 * Returns an array of dynamically allocated strings from a packed buffer of consecutive 0-terminated
 * or '\r'-separated string elements.
 *
 * \param[in]   data      Pointer to the packed string data buffer.
 * \param[in]   len       length of packed string (excl. termination).
 * \param[in]   count     Number of string elements expected. If fewer than that are found
 *                        in the packed data, then the returned array of pointers will be
 *                        padded with NULL.
 * \param[out]  dst       An array of string pointers (of size 'count') which will point to
 *                        dynamically allocated string (char*) elements. The array is assumed
 *                        to be uninitialized, and elements will be allocated as necessary.
 *
 * \return                X_SUCCESS (0) if successful, or else X_NULL if one of the argument pointers is NULL
 */
int xUnpackStrings(const char *data, int len, int count, char **dst) {
  int i, offset = 0;

  if(!data || !dst) return X_NULL;

  // Make sure the data has proper string termination at its end, so we don't overrun...
  //data[len] = '\0';

  for(i=0; i<count && offset < len; i++) {
    const char *from = &data[offset];
    int l;

    for(l=0; from[l] && offset + l < len; l++) if(from[l] == '\r') break;

    dst[i] = (char *) malloc(l+1);
    if(!dst[i]) return X_INCOMPLETE;
    memcpy(dst[i], from, l);
    dst[i][l] = '\0'; // termination...

    offset += l;
  }

  return X_SUCCESS;
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
  return str ? strdup(str) : NULL;
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
  static char *true[] = { "true", "t", "on", "yes", "y", "enabled", "active", NULL };
  static char *false[] = { "false", "f", "off", "no", "n", "disabled", "invactive", NULL };

  int i;
  long l;

  if(!str) {
    *end = NULL;
    return FALSE;
  }

  while(isspace(*str)) str++;

  if(end) *end = str;

  for(i = 0; true[i]; i++) if(TokenMatch(str, true[i])) {
    if(end) *end = str + strlen(true[i]);
    return TRUE;
  }

  for(i = 0; false[i]; i++) if(TokenMatch(str, false[i])) {
    if(end) *end = str + strlen(false[i]);
    return FALSE;
  }

  /* Try parse as a number... */
  l = strtol(str, end, 0);

  if(errno != ERANGE) return (l != 0);

  return FALSE;
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
 * @return          the floating-point value at the head of the string.
 */
double xParseDouble(const char *str, char **tail) {
#if EXPLCIT_PARSE_SPECIAL_DOUBLES
  char *next = (char *) str;

  while(isspace(*next)) next++;

  if(*next == '+' || *next == '-') next++;          // Skip sign (if any)

  // If leading character is not a number or a decimal point, then check for special values.
  if(*next) if((*next < '0' || *next > '9') && *next != '.') {
    if(!CompareToken("nan", next)) {
      if(tail) *tail = next + sizeof("nan") - 1;
      return NAN;
    }
    if(!CompareToken("inf", next)) {
      if(tail) *tail = next + sizeof("inf") - 1;
      return NAN;
    }
    if(!CompareToken("infinity", next)) {
      if(tail) *tail = next + sizeof("infinity") - 1;
      return NAN;
    }
  }
#endif

  return strtod(str, tail);
}


/**
 * Prints a double precision number, restricted to IEEE double-precision range. If the native
 * value has abolute value smaller than the smallest non-zero value, then 0 will printed instead.
 * For values that exceed the IEEE double precision range, "nan" will be printed.
 *
 *
 * @param str       Pointer to buffer for printed value.
 * @param value     Value to print.
 * @return          Number of characters printed into the buffer.
 */
int xPrintDouble(char *str, double value) {
  // For double-precision restrict range to IEEE range
  if(value > DBL_MAX) return sprintf(str, "nan");
  else if(value < -DBL_MAX) return sprintf(str, "nan");
  else if(isnan(value)) return sprintf(str, "nan");
  else if(value < DBL_MIN) if(value > -DBL_MIN) return sprintf(str, "0");

  return sprintf(str, "%.16g", value);
}

/**
 * Prints a single-precision number, restricted to IEEE single-precision range. If the native
 * value has abolute value smaller than the smallest non-zero value, then 0 will printed instead.
 * For values that exceed the IEEE double precision range, "nan" will be printed.
 *
 *
 * @param str       Pointer to buffer for printed value.
 * @param value     Value to print.
 * @return          Number of characters printed into the buffer.
 */
int xPrintFloat(char *str, float value) {
  // For single-precision restrict range to IEEE range
  if(value > FLT_MAX) return sprintf(str, "nan");
  else if(value < -FLT_MAX) return sprintf(str, "nan");
  else if(isnan(value)) return sprintf(str, "nan");
  else if(value < FLT_MIN) if(value > -FLT_MIN) return sprintf(str, "0");

  return sprintf(str, "%.8g", value);
}


/**
 * Prints a descriptive error message to stderr, and returns the error code.
 *
 * \param func      String that describes the function or location where the error occurred.
 * \param errorCode Error code that describes the failure.
 *
 * \return          Same error code as specified on input.
 */
int xError(const char *func, int errorCode) {
  if(xDebug) fprintf(stderr, "DEBUG-X> %4d (%s) in %s.\n", errorCode, xErrorDescription(errorCode), func);
  return errorCode;
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
  case X_FAILURE: errno = ECANCELED; return "internal failure";
  case X_SUCCESS: errno = 0; return "success!";
  case X_ALREADY_OPEN: errno = EISCONN; return "already opened";
  case X_NO_INIT: errno = ENXIO; return "not initialized";
  case X_NO_SERVICE: errno = EIO; return "connection lost?";
  case X_NO_BLOCKED_READ: errno = 0; return "no blocked calls";
  case X_NO_PIPELINE: errno = EPIPE; return "pipeline mode disabled";
  case X_TIMEDOUT: errno = ETIMEDOUT; return "timed out while waiting to complete";
  case X_INTERRUPTED: errno = EINTR; return "wait released without read";
  case X_INCOMPLETE: errno = EAGAIN; return "incomplete data transfer";
  case X_NULL: errno = EFAULT; return "null value";
  case X_PARSE_ERROR: errno = EINVAL; return "parse error";
  case X_NOT_ENOUGH_TOKENS: errno = EINVAL; return "not enough tokens";
  case X_GROUP_INVALID: errno = EINVAL; return "invalid group";
  case X_NAME_INVALID: errno = EINVAL; return "invalid variable name";
  case X_TYPE_INVALID: errno = EINVAL; return "invalid variable type";
  case X_SIZE_INVALID: errno = EDOM; return "invalid variable dimensions";
  }
  return "unknown error";
}


