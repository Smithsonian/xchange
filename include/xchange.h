/**
 * \file
 *
 * \date Mar 14, 2019
 * \author Attila Kovacs
 *
 *   A Basic set of utilities to allow platform-independent structured data exchange from C/C++.
 *   It also includes a JSON parser and emitter implementation.
 *
 * @version 0.9.0
 */

#ifndef XCHANGE_H_
#define XCHANGE_H_


/// API major version
#define XCHANGE_MAJOR_VERSION  0

/// API minor version
#define XCHANGE_MINOR_VERSION  9

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

#define X_CHARS(length)      (-(length))    ///< \hideinitializer A fixed-size character sequence of 'length' bytes.
#define X_UNKNOWN           0       ///< Unknown XType (default)
#define X_BOOLEAN           '?'     ///< \hideinitializer boolean XType
#define X_BYTE              'B'     ///< \hideinitializer single byte XType
#define X_BYTE_HEX          '#'     ///< \hideinitializer single byte XType in dexadecimal representation
#define X_SHORT             'S'     ///< \hideinitializer native short XType (usually 16-bits)
#define X_SHORT_HEX         'h'     ///< \hideinitializer native short XType in hexadecimal representation
#define X_INT               'L'     ///< \hideinitializer native int XType (usually 16-bits)
#define X_INT_HEX           'H'     ///< \hideinitializer native int XType in hexadecimal representation
#define X_LONG              'Y'     ///< \hideinitializer 64-bit int XType
#define X_LONG_HEX          'Z'     ///< \hideinitializer 64-bit int XType in hexadecimal representation
#define X_FLOAT             'F'     ///< \hideinitializer 32-bit floating point XType
#define X_DOUBLE            'D'     ///< \hideinitializer double-precision (64) bit floating point XType
#define X_STRING            '$'     ///< \hideinitializer a terminated string XType
#define X_RAW               'R'     ///< \hideinitializer raw Redis (string) value XType, as stored in database
#define X_STRUCT            'X'     ///< \hideinitializer An XStructure XType

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
#    define NAN _NAN
#  else
#    define NAN (0.0/0.0)           ///< Not-a-number in case it's not already defined in math.h
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
  char *value;              ///< Pointer to designated local string content (or structure)...
  XType type;               ///< The underlyng data type
  int ndim;                 ///< The dimensionality of the data
  int sizes[X_MAX_DIMS];    ///< The sizes along each dimension
  boolean isSerialized;     ///< Whether the fields is stored in serialized (string) format.
  struct XField *next;      ///< Pointer to the next linked element (if inside an XStructure).
} XField;

/**
 * Static initializer for the XField data structure.
  */
#define X_FIELD_INIT        {NULL}

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
int xError(const char *func, int errorCode);    // TODO...
const char *xErrorDescription(int code);

// Structure access methods ----------------------------------->
XStructure *xCreateStruct();
XStructure *xCopyOfStruct(const XStructure *s);
void xDestroyStruct(XStructure *s);
void xClearStruct(XStructure *s);
XField *xCreateField(const char *name, XType type, int ndim, const int *sizes, const void *value);
XField *xCreateScalarField(const char *name, XType type, const void *value);
XField *xCopyOfField(const XField *f);
void xDestroyField(XField *f);
XField *xGetField(const XStructure *s, const char *name);
XField *xSetField(XStructure *s, XField *f);
XField *xRemoveField(XStructure *s, const char *name);
boolean xIsFieldValid(const XField *f);
int xGetFieldCount(const XField *f);
int xCountFields(const XStructure *s);
XStructure *xGetSubstruct(const XStructure *s, const char *id);
XField *xSetSubstruct(XStructure *s, const char *name, XStructure *substruct);
int xReduceDims(int *ndim, int *sizes);
int xReduceAllDims(XStructure *s);

// Convenience field creator methods
XField *xCreateDoubleField(const char *name, double value);
XField *xCreateIntField(const char *name, int value);
XField *xCreateLongField(const char *name, long long value);
XField *xCreateBooleanField(const char *name, boolean value);
XField *xCreateStringField(const char *name, const char *value);
XField *xCreate1DField(const char *name, XType type, int count, const void *values);

// Parsers / formatters
boolean xParseBoolean(char *str, char **end);
double xParseDouble(const char *str, char **tail);
int xPrintDouble(char *str, double value);
int xPrintFloat(char *str, float value);
int xParseDims(const char *src, int *sizes);
int xPrintDims(char *dst, int ndim, const int *sizes);

// Low-level utilities ---------------------------------------->
char xTypeChar(XType type);
boolean xIsCharSequence(XType type);
char *xLastSeparator(const char *id);
char *xNextIDToken(const char *id);
char *xCopyIDToken(const char *id);
int xMatchNextID(const char *token, const char *id);
char *xGetAggregateID(const char *group, const char *key);
int xSplitID(char *id, char **pKey);
int xElementSizeOf(XType type);
int xStringElementSizeOf(XType type);
int xGetElementCount(int ndim, const int *sizes);
void *xAlloc(XType type, int count);
void xZero(void *buf, XType type, int count);
char *xStringCopyOf(const char *str);


#endif /* XCHANGE_H_ */