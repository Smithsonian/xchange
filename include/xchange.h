/**
 * \file
 *
 * \date Mar 14, 2019
 * \author Attila Kovacs
 * @version 1.0
 *
 *   A Basic set of utilities to allow platform-independent structured data exchange from C/C++.
 *   It also includes a JSON parser and emitter implementation.
 */

#ifndef XCHANGE_H_
#define XCHANGE_H_

#define __STDC_LIMIT_MACROS         ///< for fixed-width integer limits

#include <stdint.h>
#include <limits.h>

/// API major version
#define XCHANGE_MAJOR_VERSION  1

/// API minor version
#define XCHANGE_MINOR_VERSION  1

/// Integer sub version of the release
#define XCHANGE_PATCHLEVEL     0

/// Additional release information in version, e.g. "-1", or "-rc1".
#define XCHANGE_RELEASE_STRING "-devel"


#ifdef str_2
#  undef str_2
#endif

/// Stringify level 2 macro
#define str_2(s) str_1(s)

#ifdef str_1
#  undef str_1
#endif

/// Stringify level 1 macro
#define str_1(s) #s


/// The version string for this library
#define XCHANGE_VERSION_STRING str_2(XCHANGE_MAJOR_VERSION) "." str_2(XCHANGE_MINOR_VERSION) \
                                  "." str_2(XCHANGE_PATCHLEVEL) XCHANGE_RELEASE_STRING


#ifndef TRUE
#define TRUE    1           ///< Boolean 'true' value, if not already defined.
#endif

#ifndef FALSE
#define FALSE   0           ///< Boolean 'false' value, if not already defined.
#endif

#define X_SUCCESS              0  ///< hooray!
#define X_FAILURE           (-1)  ///< didn't quite work


#define X_ALREADY_OPEN      (-2)  ///< can't open the device twice
#define X_NO_INIT           (-3)  ///< must initialize first
#define X_NO_SERVICE        (-4)  ///< no connection to server
#define X_NO_BLOCKED_READ   (-5)  ///< there are no blocked read calls
#define X_NO_PIPELINE       (-6)  ///< pipelining is not configured or supported
#define X_TIMEDOUT          (-7)  ///< the operation timed out.
#define X_INTERRUPTED       (-8)  ///< a waiting call interrupted by user
#define X_INCOMPLETE        (-9)  ///< incomplete data transfer
#define X_NULL              (-10) ///< something essential is NULL
#define X_PARSE_ERROR       (-11) ///< parsing failure
#define X_NOT_ENOUGH_TOKENS (-12) ///< number of components fewer than expected

#define X_GROUP_INVALID     (-20)  ///< there's no such variable group or hash table
#define X_NAME_INVALID      (-21)  ///< invalid field/key name
#define X_TYPE_INVALID      (-22)  ///< invalid data type
#define X_SIZE_INVALID      (-23)  ///< invalid data size
#define X_STRUCT_INVALID    (-24)  ///< invalid data structure

#define MAX_DEBUG_ERROR_COUNT   100     ///< Stop reporting errors if the number of them exceeds this count.

typedef int XType;          ///< SMA-X data type.

#define X_CHARS(length)      (-(length))    ///< \hideinitializer A fixed-size sequence of 'length' bytes.
#define X_UNKNOWN           0       ///< Unknown XType (default)
#define X_BOOLEAN           '?'     ///< \hideinitializer `boolean` XType
#define X_BYTE              'B'     ///< \hideinitializer single `byte` XType
#define X_INT16             'S'     ///< \hideinitializer 16-bit integer Xtype
#define X_INT32             'L'     ///< \hideinitializer 32-bit integer Xtype
#define X_INT64             'Y'     ///< \hideinitializer 64-bit integer Xtype
#define X_FLOAT             'F'     ///< \hideinitializer 32-bit floating point XType
#define X_DOUBLE            'D'     ///< \hideinitializer double-precision (64) bit floating point XType
#define X_STRING            '$'     ///< \hideinitializer a terminated string XType
#define X_RAW               'R'     ///< \hideinitializer raw Redis (string) value XType, as stored in database
#define X_STRUCT            'X'     ///< \hideinitializer XType for an XStructure or array thereof
#define X_FIELD             '-'     ///< \hideinitializer XType for an XField or array thereof

// Platform-specific width integer types
#if SHRT_MAX == INT16_MAX
#  define X_SHORT             X_INT16     ///< \hideinitializer native `short` XType
#elif SHRT_MAX == INT32_MAX
#  define X_SHORT             X_INT32     ///< \hideinitializer native `short` XType
#else
#  define X_SHORT             X_INT64     ///< \hideinitializer native `short` XType
#endif

#if INT_MAX == INT16_MAX
#  define X_INT               X_INT16     ///< \hideinitializer native `int` XType
#elif INT_MAX == INT32_MAX
#  define X_INT               X_INT32     ///< \hideinitializer native `int` XType
#else
#  define X_INT               X_INT64     ///< \hideinitializer native `int` XType
#endif

#if LONG_MAX == INT32_MAX
#  define X_LONG              X_INT32     ///< \hideinitializer native `long` XType
#else
#  define X_LONG              X_INT64     ///< \hideinitializer native `long` XType
#endif

#if !defined(LLONG_MAX) || LLONG_MAX == INT64_MAX
#  define X_LLONG             X_INT64     ///< \hideinitializer native `long long` XType
#else
#  define X_LLONG            X_UNDEFINED ///< \hideinitializer no native `long long` XType
#endif

#define X_SEP               ":"             ///< sepatator for patterning of notification channels, e.g. "changed:<table>:<key>"
#define X_SEP_LENGTH         (sizeof(X_SEP) - 1)    ///< String length of hierarchical separator.

#define X_TIMESTAMP_LENGTH              18      ///< Characters in timestamp, 10 + 6 + 2 = 18  including termination
#define X_MAX_DIMS                      20      ///< Maximum number of dimensionas (2^20 -> 1 million points).
#define X_MAX_STRING_DIMS               (2 * X_MAX_DIMS + 1)    ///< \hideinitializer Maximum length of string representation of dimensions
#define X_MAX_ELEMENTS                  (1<<X_MAX_DIMS)         ///< \hideinitializer Maximum number of array elements (~1 million).


#ifndef _TYPEDEF_BOOLEAN
#define _TYPEDEF_BOOLEAN           ///< Precompiler constant to indicate if boolean was defined.
typedef int boolean;               ///< boolean TRUE/FALSE data type.
#endif

#ifndef NAN
#  ifdef _NAN
#    define NAN _NAN               ///< Not-a-number in case it's not already defined in math.h
#  else
#    define NAN (0.0/0.0)          ///< Not-a-number in case it's not already defined in math.h
#  endif
#endif

#ifndef INFINITY
#  ifdef _INFINITY
#    define INFINITY _INFINITY     ///< Infinity in case it's not already defined in math.h
#  else
#    define INFINITY (1.0/0.0)     ///< Infinity in case it's not already defined in math.h
#  endif
#endif

/**
 * \brief An SMA-X field, typically as part of an XStructure. A field may be a reference to a nested XStructure itself.
 *
 * \sa smaxCreateField()
 * \sa smaxCreateScalarField()
 * \sa smaxCreate1DField()
 * \sa smaxDestroyField()
 * \sa smaxShareField()
 */
typedef struct XField {
  char *name;               ///< Pointer to a designated local name buffer. It may not contain a separator (see X_SEP).
                            ///< NOTE: it should normally be dynamically allocated, to work with xClearField() / xDestroyField().
  void *value;              ///< Pointer to designated local string content (or structure)...
                            ///< NOTE: it should normally be dynamically allocated, to work with xClearField() / xDestroyField().
  XType type;               ///< The underlyng data type
  char *subtype;            ///< (optional) Descriptive subtype, such a a mime type or encoding (if any). It is
                            ///< entirely up to the user / application to assing meaning to this field.
                            ///< NOTE: it should normally be dynamically allocated, to work with xClearField() / xDestroyField().
  int ndim;                 ///< The dimensionality of the data
  int sizes[X_MAX_DIMS];    ///< The sizes along each dimension
  boolean isSerialized;     ///< Whether the fields is stored in serialized (string) format.
  struct XField *next;      ///< Pointer to the next linked element (if inside an XStructure).
} XField;

/**
 * Static initializer for the XField data structure.
  */
#define X_FIELD_INIT        {NULL, NULL, X_UNKNOWN, NULL, 0, {0}, FALSE, NULL}

/**
 * \brief SMA-X structure object, containing a linked-list of XField elements.
 *
 * \sa smaxCreateStruct()
 * \sa smaxDestroyStruct()
 * \sa smaxShareStruct()
 */
typedef struct XStructure {
  XField *firstField;           ///< Pointer to the first field in this structure or NULL if the structure is empty.
  struct XStructure *parent;    ///< Reference to parent structure (if any)
} XStructure;

/**
 * A fast lookup table for fields in large XStructures.
 *
 * @sa xCreateLookup()
 * @sa xLookupField()
 * @sa xDestroyLookup()
 */
typedef struct {
  void *priv;                   ///< Private data, not exposed to users
} XLookupTable;



/**
 * Static initializer for an XStructure data structure.
 */
#define X_STRUCT_INIT       {NULL}

extern boolean xVerbose;        ///< Switch to enable verbose console output for XChange operations.
extern boolean xDebug;          ///< Switch to enable debugging (very verbose) output for XChange operations.

#define xvprintf if(xVerbose) printf        ///< Use for generating verbose output
#define xdprintf if(xDebug) printf          ///< Use for generating debug output

// In xutil.c ------------------------------------------------>
boolean xIsVerbose();
void xSetVerbose(boolean value);
void xSetDebug(boolean value);
int xError(const char *fn, int code);
const char *xErrorDescription(int code);

// Structure access methods ----------------------------------->
XStructure *xCreateStruct();
XStructure *xCopyOfStruct(const XStructure *s);
void xDestroyStruct(XStructure *s);
void xClearStruct(XStructure *s);
XField *xCreateField(const char *name, XType type, int ndim, const int *sizes, const void *value);
XField *xCreateScalarField(const char *name, XType type, const void *value);
XField *xCopyOfField(const XField *f);
void xClearField(XField *f);
void xDestroyField(XField *f);
XField *xGetField(const XStructure *s, const char *name);
XField *xSetField(XStructure *s, XField *f);
XField *xRemoveField(XStructure *s, const char *name);
boolean xIsFieldValid(const XField *f);
long xGetFieldCount(const XField *f);
void *xGetElementAtIndex(const XField *f, int idx);
long xGetAsLongAtIndex(const XField *f, int idx, long defaultValue);
double xGetAsDoubleAtIndex(const XField *f, int idx);
char *xGetStringAtIndex(const XField *f, int idx);
long xGetAsLong(const XField *f, long defaultValue);
double xGetAsDouble(const XField *f);
char *xGetStringValue(const XField *f);

int xCountFields(const XStructure *s);
long xDeepCountFields(const XStructure *s);
XStructure *xGetSubstruct(const XStructure *s, const char *id);
XField *xSetSubstruct(XStructure *s, const char *name, XStructure *substruct);
int xReduceDims(int *ndim, int *sizes);
int xReduceStruct(XStructure *s);
int xReduceField(XField *f);

// Sorting, ordering
int xSortFields(XStructure *s, int (*cmp)(const XField **f1, const XField **f2), boolean recursive);
int xSortFieldsByName(XStructure *s, boolean recursive);
int xReverseFieldOrder(XStructure *s, boolean recursive);

// Field lookup
XLookupTable *xAllocLookup(unsigned int size);
XLookupTable *xCreateLookup(const XStructure *s, boolean recursive);
XField *xLookupField(const XLookupTable *tab, const char *id);
int xLookupPut(XLookupTable *tab, const char *prefix, const XField *field, XField **oldValue);
XField *xLookupRemove(XLookupTable *tab, const char *id);
int xLookupPutAll(XLookupTable *tab, const char *prefix, const XStructure *s, boolean recursive);
int xLookupRemoveAll(XLookupTable *tab, const char *prefix, const XStructure *s, boolean recursive);
long xLookupCount(const XLookupTable *tab);
void xDestroyLookup(XLookupTable *tab);
void xDestroyLookupAndData(XLookupTable *tab);

// Convenience field creator methods
XField *xCreateDoubleField(const char *name, double value);
XField *xCreateIntField(const char *name, int value);
XField *xCreateLongField(const char *name, long long value);
XField *xCreateBooleanField(const char *name, boolean value);
XField *xCreateStringField(const char *name, const char *value);
XField *xCreate1DField(const char *name, XType type, int count, const void *values);
XField *xCreateMixedArrayField(const char *name, int ndim, const int *sizes, XField *value);
XField *xCreateMixed1DField(const char *name, int size, XField *value);
int xSetSubtype(XField *f, const char *type);

// Parsers / formatters
boolean xParseBoolean(char *str, char **end);
float xParseFloat(const char *str, char **tail);
double xParseDouble(const char *str, char **tail);
int xPrintDouble(char *str, double value);
int xPrintFloat(char *str, float value);
int xParseDims(const char *src, int *sizes);
int xPrintDims(char *dst, int ndim, const int *sizes);

// Low-level utilities ---------------------------------------->
char xTypeChar(XType type);
boolean xIsCharSequence(XType type);
boolean xIsInteger(XType type);
boolean xIsDecimal(XType type);
boolean xIsNumeric(XType type);
char *xLastSeparator(const char *id);
char *xNextIDToken(const char *id);
char *xCopyIDToken(const char *id);
int xMatchNextID(const char *token, const char *id);
char *xGetAggregateID(const char *group, const char *key);
int xSplitID(char *id, char **pKey);
int xElementSizeOf(XType type);
int xStringElementSizeOf(XType type);
long xGetElementCount(int ndim, const int *sizes);
void *xAlloc(XType type, int count);
void xZero(void *buf, XType type, int count);
char *xStringCopyOf(const char *str);


// <================= xchange internals ======================>

/// \cond PRIVATE
#ifdef __XCHANGE_INTERNAL_API__

#  include <stdio.h>
#  include <math.h>

// On some older platform NAN may not be defined, so define it here if need be
#ifndef NAN
#  define NAN               (0.0/0.0)
#endif

int x_error(int ret, int en, const char *from, const char *desc, ...);
int x_warn(const char *from, const char *desc, ...);
int x_trace(const char *loc, const char *op, int n);
void *x_trace_null(const char *loc, const char *op);

/**
 * Propagates an error (if any) with an offset. If the error is non-zero, it returns with the offset
 * error value. Otherwise it keeps going as if it weren't even there...
 *
 * @param n     {int} error code or the call that produces the error code
 *
 * @sa error_return
 */
#  define prop_error(loc, n) { \
  int __ret = x_trace(loc, NULL, n); \
  if (__ret < 0) \
    return __ret; \
}

#  define x_check_alloc(ptr) \
  if (!ptr) { \
    perror("ERROR! alloc error"); \
    exit(errno); \
  }

#endif /* __XCHANGE_INTERNAL_API__ */
/// \endcond

#endif /* XCHANGE_H_ */
