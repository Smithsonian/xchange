/**
 * @file
 *
 * @date Oct 25, 2020
 * @author Attila Kovacs
 *
 * @brief   A set of functions for parsing and creating JSON description of data.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <sys/stat.h>

#define __XCHANGE_INTERNAL_API__        ///< Use internal definitions
#include "xjson.h"

#ifndef TRUE
#define TRUE 1          ///< Boolean 'true' in case it isn't already defined
#endif

#ifndef FALSE
#define FALSE 0         ///< Boolean 'false' in case it isn't already defined
#endif

/// \cond PRIVATE
#define JSON_TRUE       "true"
#define JSON_TRUE_LEN   (sizeof(JSON_TRUE) - 1)

#define JSON_FALSE      "false"
#define JSON_FALSE_LEN  (sizeof(JSON_FALSE) - 1)

#define JSON_NULL       "null"
#define JSON_NULL_LEN   (sizeof(JSON_NULL) - 1)

#define TERMINATED_STRING       (-1)

#define ERROR_PREFIX    "ERROR! XJSON "
#define WARNING_PREFIX  "WARNING! XJSON "


#define Error(format, ARGS...)      fprintf(xerr ? xerr : stderr, ERROR_PREFIX format, ##ARGS)
#define Warning(format, ARGS...)    fprintf(xerr ? xerr : stderr, WARNING_PREFIX format, ##ARGS)
/// \endcond

static XStructure *ParseObject(char **pos, int *lineNumber);
static void *ParseValue(char **pos, XType *type, int *ndim, int sizes[X_MAX_DIMS], int *lineNumber);
static void *ParseArray(char **pos, XType *type, int *ndim, int sizes[X_MAX_DIMS], int *lineNumber);
static char *ParseString(char **pos, int *lineNumber);
static void *ParsePrimitive(char **pos, XType *type, int *lineNumber);

static int GetObjectStringSize(int prefixSize, const XStructure *s);
static int GetFieldStringSize(int prefixSize, const XField *f);
static int GetArrayStringSize(int prefixSize,char *ptr, XType type, int ndim, const int *sizes);
static int GetJsonStringSize(const char *src, int maxLength);

static int PrintObject(const char *prefix, const XStructure *s, char *str, boolean asArray);
static int PrintField(const char *prefix, const XField *f, char *str);
static int PrintArray(const char *prefix, char *ptr, XType type, int ndim, const int *sizes, char *str);
static int PrintPrimitive(const void *ptr, XType type, char *str);
static int PrintString(const char *src, int maxLength, char *json);

static FILE *xerr;     ///< File / stream, which errors are printed to. A NULL will print to stderr

static char *indent = XJSON_INDENT;
static int ilen = sizeof(XJSON_INDENT) - 1;

/**
 * Sets the number of spaces per indentation when emitting JSON formatted output.
 *
 * @param nchars    (bytes) the new number of white space character of indentation to use. Negative values
 *                  map to 0.
 *
 * @sa xjsonGetIndent()
 * @sa xjsonToString()
 */
void xjsonSetIndent(int nchars) {
  if(nchars < 0) nchars = 0;

  indent = (char *) malloc(nchars + 1);
  x_check_alloc(indent);

  memset(indent, ' ', nchars);
  indent[nchars] = '\0';

  ilen = nchars;
}

/**
 * Returns the number of spaces per indentation when emitting JSON formatted output.
 *
 * @return    (bytes) the number of white space character of indentation used.
 *
 * @sa xjsonSetIndent()
 * @sa xjsonToString()
 */
int xjsonGetIndent() {
  return ilen;
}

/**
 * Converts structured data into its JSON representation. Conversion errors are reported to stderr
 * or the altenate stream set by xSetErrorStream().
 *
 * @param s     Pointer to structured data
 * @return      String JSON representation, or NULL if there was an error (errno set to EINVAL).
 *
 * @sa xjsonSetIndent()
 * @sa xjsonParseAt()
 * @sa xjsonParseFile()
 * @sa xjsonParseFilename()
 */
char *xjsonToString(const XStructure *s) {
  char *str;
  int n;

  if(!s) {
    x_error(0, X_NULL, "xjsonToString", "input structure is NULL");
    return NULL;
  }

  if(!xerr) xerr = stderr;

  n = GetObjectStringSize(0, s);
  if(n < 0) {
    Error("%s\n", xErrorDescription(n));
    errno = EINVAL;
    return NULL;
  }

  str = (char *) malloc(n + 1);
  if(!str) {
    Error("Out of memory (need %ld bytes).\n", (long) (n+1));
    return NULL;
  }

  n = PrintObject("", s, str, FALSE);
  if (n < 0) {
    free(str);
    return NULL;
  }

  sprintf(&str[n], "\n");

  return str;
}


/**
 * Parses a JSON object from the given parse position, returning the structured data
 * and updating the parse position. Parse errors are reported to stderr or the alternate
 * stream set by xSetErrorStream().
 *
 *
 * @param[in,out] pos           Pointer to current parse position, which will be updated to point to after the last character consumed
 *                              by the JSON parser.
 * @param[out]    lineNumber    Optional pointer that holds a line number of the parse position, or NULL if not required.
 *                              Line numbers may be useful to report where the parser run into an error if the parsing failed.
 *                              Line numbers start at 1, and are counted from the initial parse position.
 *
 * @return        Structured data created from the JSON description, or NULL if there was an error parsing the data (errno is
 *                set to EINVAL).
 *                The lineNumber argument can be used to determine where the error occurred.
 *
 * @sa xjsonToString()
 * @sa xjsonParseFile()
 * @sa xjsonParseFileName()
 */
XStructure *xjsonParseAt(char **pos, int *lineNumber) {
  int n;

  if(!pos) {
    x_error(0, EINVAL, "xjsonParseAt", "'pos' parameter is NULL");
    return NULL;
  }

  if(!xerr) xerr = stderr;

  if(!lineNumber) lineNumber = &n;
  *lineNumber = 1;      // Start at line 1...

  return ParseObject(pos, lineNumber);
}

/**
 * Parses a JSON object from the beginning of a file, returning the described structured data.
 * Parse errors are reported to stderr or the alternate stream set by xSetErrorStream().
 *
 *
 * @param[in]  fileName     File name/path to parse.
 * @param[out] lineNumber   Optional pointer that holds a line number of the parse position, or NULL if not required.
 *                          Line numbers may be useful to report where the parser run into an error if the parsing failed.
 *                          Line numbers start at 1, and are counted from the initial parse position.
 *
 * @return     Structured data created from the JSON description, or NULL if there was an error parsing the data
 *             (errno is set to EINVAL). The lineNumber argument can be used to determine where the error occurred).
 *
 * @sa xjsonParseFile()
 * @sa xjsonParseAt()
 * @sa xjsonToString()
 */
XStructure *xjsonParseFilename(const char *fileName, int *lineNumber) {
  FILE *fp;
  struct stat st;
  XStructure *s;

  if(!fileName) {
    x_error(0, EINVAL, "xjsonParseFilename", "fileName is NULL");
    return NULL;
  }

  if(xIsVerbose()) fprintf(stderr, "XJSON: Parsing %s.\n", fileName);

  fp = fopen(fileName, "r");
  if(!fp) {
    Error("Cannot open file (%s).\n", strerror(errno));
    *lineNumber = -1;
    return NULL;
  }

  stat(fileName, &st);
  s = xjsonParseFile(fp, st.st_size, lineNumber);

  fclose(fp);

  return s;
}


/**
 * Parses a JSON object from the current position in a file, returning the described structured data.
 * Parse errors are reported to stderr or the alternate stream set by xSetErrorStream().
 *
 *
 * @param[in]  fp           File pointer, opened with read permission ("r").
 * @param[in]  length       [bytes] The number of bytes to parse / available, or 0 to read to the end
 *                          of the file. (In the latter case the file must support fseek with SEEK_END to automatically
 *                          determine the length, or else this function will return NULL).
 * @param[out] lineNumber   Optional pointer that holds a line number of the parse position, or NULL if not required.
 *                          Line numbers may be useful to report where the parser run into an error if the parsing failed.
 *                          Line numbers start at 1, and are counted from the initial parse position.
 *
 * @return     Structured data created from the JSON description, or NULL if there was an error parsing the data
 *             (errno is set to EINVAL). The lineNumber argument can be used to determine where the error occurred).
 *
 * @sa xjsonParseFilename()
 * @sa xjsonParseAt()
 * @sa xjsonToString()
 */
XStructure *xjsonParseFile(FILE *fp, size_t length, int *lineNumber) {
  static const char *fn = "xjsonParseFile";

  XStructure *s;
  int n;
  long L;
  char *str;
  volatile char *pos;

  if(fp == NULL) {
    x_error(0, EINVAL, fn, "file is NULL");
    return NULL;
  }

  if(!xerr) xerr = stderr;

  if(length == 0) {
    long p = ftell(fp);
    if(fseek(fp, 0, SEEK_END) != 0) {
      x_error(0, errno, fn, "fseek() error");
      return NULL;
    }
    length = ftell(fp) - p;
    if(fseek(fp, p, SEEK_SET) != 0) {
      x_error(0, errno, fn, "ftell() error");
      return NULL;
    }
  }

  if(!lineNumber) lineNumber = &n;
  *lineNumber = 1;      // Start at line 1...

  pos = str = malloc(length + 1);
  if(!str){
    Error("Out of memory (read %ld bytes).\n", (long) (length + 1));
    fclose(fp);
    *lineNumber = -1;
    return NULL;
  }

  for(L=0; L < length; L++) {
    int m = fread(str, L, 1, fp);

    if(m < 0) {
      Error("Read error: %s (pos = %ld).\n", strerror(errno), L);
      L = -1;
      break;
    }

    if(!m) {
      Error("Incomplete read (%ld of %ld bytes).\n", L, (long) length);
      L = -1;
      break;
    }

    L += m;
  }

  if(L < 0) {
    free(str);
    *lineNumber = -1;
    return NULL;
  }

  str[L] = '\0';

  s = ParseObject((char **) &pos, lineNumber);

  free(str);

  return s;
}


/**
 * Change the file to which XJSON reports errors. By default it will use stderr.
 *
 * @param fp    File to which to write errors or NULL to suppress errors.
 */
void xjsonSetErrorStream(FILE *fp) {
  static int local;     // If using a file opened by this call...

  if(local) {
    if(!fp) return;
    fclose(xerr);
    local = FALSE;
  }

  if(!fp) {
    fp = fopen(NULLDEV, "w");
    if(fp) local = TRUE;
  }

  if(fp) xerr = fp;
}

/**
 * Returns a pointer to the start of the next non-space character, while counting lines..
 *
 * @param str           Starting parse position.
 * @param lineNumber    Pointer to the line number counter.
 * @return              Pointer to the next non-space character.
 */
static char *SkipSpaces(char *str, int *lineNumber) {
  for(; *str; str++) {
    if(!isspace(*str)) {
      // Comments aren't part of standard JSON, but Microsoft uses them, so we'll deal with it...
      if(*str != '#') break;                            // Start of next (non comment) token...
      else for(++str; *str; str++) if(*str == '\n') {   // Skip comments
        str++;                                          // Consume EOL (end of comment)
        (*lineNumber)++;
        break;
      }
    }
    else if(*str == '\n') (*lineNumber)++;
  }
  return str;
}

static char *GetToken(char *from) {
  int l;
  char *token;

  from = SkipSpaces(from, &l);

  for(l=0; from[l]; l++) if(isspace(from[l])) break;

  token = malloc(l + 1);
  if(!token) {
    Error("Out of memory (need %ld bytes).\n", (long) (l + 1));
    return NULL;
  }

  memcpy(token, from, l);
  token[l] = '\0';

  return token;
}

static XStructure *ParseObject(char **pos, int *lineNumber) {
  char *next;
  XStructure *s;

  next = *pos = SkipSpaces(*pos, lineNumber);

  if(*next != '{') {
    char *bad = GetToken(next);
    Error("[L.%d] Expected '{', got \"%s\"\n", *lineNumber, bad);
    free(bad);
    return NULL;
  }

  next++; // Opening {

  s = (XStructure *) calloc(1, sizeof(XStructure));
  x_check_alloc(s);

  while(*next) {
    int l;
    XField *f;

    next = SkipSpaces(next, lineNumber);
    if(*next == '}') {
      next++;
      break;
    }

    f = calloc(1, sizeof(XField));
    x_check_alloc(f);

    for(l=0; next[l]; l++) if(isspace(next[l])) break;

    f->name = malloc(l+1);
    if(!f->name) {
      Error("[L.%d] Out of memory (field->name).\n", *lineNumber);
      xDestroyStruct(s);
      return NULL;
    }

    memcpy(f->name, next, l);
    f->name[l] = '\0';

    next += l;
    next = SkipSpaces(next, lineNumber);

    if(*next != ':') {
      char *token = GetToken(next);
      Warning("[L.%d] Missing key:value separator ':' near '%s'\n", *lineNumber, token);
      free(token);
      continue;
    }
    next++;
    next = SkipSpaces(next, lineNumber);

    switch(*next) {
      case '{':
        f->type = X_STRUCT;
        f->value = (void *) ParseObject(&next, lineNumber);
        break;
      case '[':
        f->value = ParseArray(&next, &f->type, &f->ndim, f->sizes, lineNumber);
        break;
      case '"': {
        char **str = malloc(sizeof(char *));
        x_check_alloc(str);

        *str = ParseString(&next, lineNumber);
        f->type = X_STRING;
        f->value = (char *) str;
        break;
      }
      default:
        f->value = ParsePrimitive(&next, &f->type, lineNumber);
    }

    f = xSetField(s, f);
    if(f) xDestroyField(f); // If duplicate field, destroy the prior one.
  }

  *pos = next;

  return s;
}

static char UnescapedChar(char c) {
  switch(c) {
    case '\\': return '\\';
    case '"': return '\"';
    case '/': return '/';
    case 'b': return '\b';
    case 'f': return '\f';
    case 'n': return '\n';
    case 't': return '\t';
    case 'r': return '\r';
  }
  return c;
}


static char *ParseString(char **pos, int *lineNumber) {
  int isEscaped = 0;
  int i, k, l;
  char *next, *dst;

  next = *pos = SkipSpaces(*pos, lineNumber);

  if(*next != '"') {
    char *bad = GetToken(next);
    Error("[L.%d] Expected '\"', found \"%s\".\n", *lineNumber, bad);
    free(bad);
    return NULL;
  }

  next++;

  // Figure out how many characters we need to store...
  for(i = 0, l = 0; next[i]; i++) {
    const char c = next[i];

    // Count line numbers...
    if(c == '\n') (*lineNumber)++;

    if(isEscaped) {
      isEscaped = FALSE;
      l++;
      if(c == 'u') i++;
    }
    else if(c == '\\') isEscaped = TRUE;
    else if(c == '"') break;
    else l++;
  }

  *pos = next + i + 1;    // Update the parse location....

  dst = malloc(l + 1);
  if(!dst) {
    Error("[L.%d] Out of memory.\n", *lineNumber);
    return NULL;
  }

  for(i = k = 0; k < l; i++) {
    char c = next[i];

    if(isEscaped) {
      isEscaped = FALSE;
      if(c == 'u') {
        dst[k++] = '\\';     // Keep unicode as is...
        dst[k++] = 'u';
      }
      else {
        dst[k++] = UnescapedChar(c);
      }
    }
    else if(c == '\\') isEscaped = TRUE;
    else dst[k++] = c;
  }

  dst[l] = '\0';

  return dst;
}




static void *ParsePrimitive(char **pos, XType *type, int *lineNumber) {
  int l;
  long long ll;
  char *next, *end;
  double d;

  next = *pos = SkipSpaces(*pos, lineNumber);

  for(l = 0; next[l]; l++) if(isspace(next[l]) || next[l] == ',')  break;
  *pos = &next[l];      // Consume a token not matter what...

  // Check for null
  if(l == JSON_NULL_LEN) if(!strncmp(next, JSON_NULL, JSON_NULL_LEN)) {
    *type = X_UNKNOWN;
    return NULL;
  }

  // Check if boolean
  if(l == JSON_TRUE_LEN) if(!strncmp(next, JSON_TRUE, JSON_TRUE_LEN)) {
    boolean *value = malloc(sizeof(boolean));
    x_check_alloc(value);
    *value = TRUE;
    *type = X_BOOLEAN;
    return value;
  }

  if(l == JSON_FALSE_LEN) if(!strncmp(next, JSON_FALSE, JSON_FALSE_LEN)) {
    boolean *value = malloc(sizeof(boolean));
    x_check_alloc(value);
    *value = FALSE;
    *type = X_BOOLEAN;
    return value;
  }

  // Try parse as int / long
  ll = strtoll(next, &end, 0);
  if(end == *pos) if(errno != ERANGE) {
    long long *value = (long long *) malloc(sizeof(long));
    x_check_alloc(value);
    *value = ll;
    *type = (ll == (int) ll) ? X_INT : X_LONG;
    return value;
  }

  // Try parse as double...
  d = strtod(next, &end);
  if(end == *pos) {
    double *value = (double *) malloc(sizeof(double));
    x_check_alloc(value);
    *value = d;
    *type = X_DOUBLE;
    return value;
  }

  // Bad token, take note of it...
  next = GetToken(next);
  Warning("[L.%d] Skipping invalid token \"%s\".\n", *lineNumber, next);
  free(next);

  errno = EINVAL;
  return NULL;
}


static void *ParseValue(char **pos, XType *type, int *ndim, int sizes[X_MAX_DIMS], int *lineNumber) {
  const char *next;

  next = *pos = SkipSpaces(*pos, lineNumber);

  memset(sizes, 0, X_MAX_DIMS * sizeof(int));
  *ndim = 0;

  // Is value an object?
  if(*next == '{') {
    *type = X_STRUCT;
    return ParseObject(pos, lineNumber);
  }

  // Is value an array?
  if(*next == '[') return ParseArray(pos, type, ndim, sizes, lineNumber);

  // Is value a string?
  if(*next == '"') return ParseString(pos, lineNumber);

  return ParsePrimitive(pos, type, lineNumber);
}


static XType GetCommonType(XType t1, XType t2) {
  if(t1 == X_STRING && t2 == X_STRING) return X_STRING;
  if(t1 == X_STRING || t2 == X_STRING) return X_STRUCT;
  if(t1 == X_STRUCT || t2 == X_STRUCT) return X_STRUCT;
  if(t1 == X_DOUBLE || t2 == X_DOUBLE) return X_DOUBLE;
  if(t1 == X_FLOAT || t2 == X_FLOAT) return X_FLOAT;
  if(t1 == X_LONG || t2 == X_LONG) return X_LONG;
  if(t1 == X_LONG_HEX || t2 == X_LONG_HEX) return X_LONG;
  if(t1 == X_INT || t2 == X_INT) return X_INT;
  if(t1 == X_INT_HEX || t2 == X_INT_HEX) return X_INT;
  if(t1 == X_SHORT || t2 == X_SHORT) return X_SHORT;
  if(t1 == X_SHORT_HEX || t2 == X_SHORT_HEX) return X_SHORT;
  if(t1 == X_BYTE || t2 == X_BYTE) return X_BYTE;
  if(t1 == X_BYTE_HEX || t2 == X_BYTE_HEX) return X_BYTE;
  if(t1 == X_BOOLEAN || t2 == X_BOOLEAN) return X_BOOLEAN;
  return X_UNKNOWN;
}


static void *ParseArray(char **pos, XType *type, int *ndim, int sizes[X_MAX_DIMS], int *lineNumber) {
  int n = 0;
  char *next;
  XField *first = NULL, *last = NULL;

  *type = X_UNKNOWN;
  *ndim = 0;
  memset(sizes, 0, X_MAX_DIMS * sizeof(int));

  next = *pos = SkipSpaces(*pos, lineNumber);

  if(*next != '[') {
    char *bad = GetToken(next);
    Error("[L.%d] Expected '[', got \"%s\".", *lineNumber, bad);
    free(bad);
    return NULL;
  }

  next = SkipSpaces(++next, lineNumber);

  for(n = 0; *next && *next != ']';) {
    XField *e;
    boolean isValid;

    e = (XField *) calloc(1, sizeof(XField));
    x_check_alloc(e);

    e->value = ParseValue(&next, &e->type, &e->ndim, e->sizes, lineNumber);
    isValid = (e->value || errno != EINVAL);

    if(isValid) n++;

    if(!last) first = e;
    else last->next = e;
    last = e;

    // If types and dimensions all match, then will return an array of that type (e.g. double[], or int[]).
    // Otherwise return an array of the common type...
    if(*type != X_STRUCT) {
      if(*type == X_UNKNOWN) *type = e->type;
      else if(*type != e->type) *type = GetCommonType(*type, e->type);

      if(*ndim == 0) {
        *ndim = e->ndim;
        memcpy(sizes, e->sizes, e->ndim * sizeof(int));
      }

      // Heterogeneous arrays are treated as array of structures.
      else if(*ndim != e->ndim) {
        *type = X_STRUCT;
      }
      else if(memcmp(sizes, e->sizes, e->ndim * sizeof(int))) {
        *type = X_STRUCT;
      }
    }

    // After the value there must be either a comma or or closing ]
    next = SkipSpaces(next, lineNumber);
    if(*next == ',') {
      if(!isValid) n++;  // Treat empty intermediate entries as NULL
      next++;
      continue;
    }

    if(*next != ']') {
      char *token = GetToken(next);
      Warning("[L.%d] Expected ',' or ']', got \"%s\".\n", *lineNumber, token);
      free(token);

      *pos = next;
      return NULL;
    }
  }

  *pos = ++next;

  // For heterogeneous arrays, return an array of XStructures...
  if(*type == X_STRUCT) {
    int i;
    XStructure *s;

    s = calloc(n, sizeof(XStructure));

    *ndim = 1;
    memset(sizes, 0, X_MAX_DIMS * sizeof(int));
    sizes[0] = n;

    (*pos)++;

    for(i = 0; i < n; i++) if(first) {
      s[i].firstField = first;
      if (first->type == X_STRUCT) ((XStructure *) first)->parent = &s[i];

      first = first->next;
      s[i].firstField->next = NULL;
    }

    return s;
  }

  // Otherwise return a primitive array...
  else {
    int i, eCount, eSize;
    int rowSize;
    char *data = NULL;

    if(*ndim >= X_MAX_DIMS) {
      Warning("[L.%d] Too many array dimensions.\n", *lineNumber);
      return NULL;
    }

    // Thus far suzes is the size of top-level entries
    eCount = xGetElementCount(*ndim, sizes);
    eSize = xElementSizeOf(*type);
    rowSize = eSize * eCount;

    // If only contains an empty array then treat as n = 0...
    if(eCount == 0 && n == 1) n = 0;

    // Add in the top-level dimension
    for(i = *ndim; --i >= 0; ) sizes[i+1] = sizes[i];
    sizes[0] = n;
    (*ndim)++;

    // create combo array and copy rows into it...
    eCount *= n;
    if(eCount > 0) {
      data = calloc(eCount, eSize);
      if(!data) {
        Error("[L.%d] Out of memory (array data).\n", *lineNumber);
        return NULL;
      }
    }

    for(i = 0; i < n; i++) if(first) {
      XField *e = first;
      if(e->value) {
        memcpy(data + i * rowSize, e->value, rowSize);
        first = e->next;
        free(e->value);
      }
      free(e);
    }

    return data;
  }
}


static int GetObjectStringSize(int prefixSize, const XStructure *s) {
  int n;
  XField *f;

  if(!s) return 0;

  n = prefixSize + 4;       // "{\n" + .... + <prefix> + "}\n";

  for(f = s->firstField; f != NULL; f = f->next) {
    int m = GetFieldStringSize(prefixSize + ilen, f);
    if(m < 0) return m;
    n += m;
  }

  return n;
}


static int PrintObject(const char *prefix, const XStructure *s, char *str, boolean asArray) {
  char *fieldPrefix;
  XField *f;
  int n = 0;

  if(!s) return X_SUCCESS;
  if(!str) return X_NULL;
  if(!prefix) return X_NULL;

  if(!s->firstField) return sprintf(&str[n], "{ }");

  fieldPrefix = malloc(strlen(prefix) + ilen + 1);
  if(!fieldPrefix) {
    Error("Out of memory (field prefix).\n");
    return X_INCOMPLETE;
  }

  sprintf(fieldPrefix, "%s%s", prefix, indent);

  n += sprintf(str, asArray ? "[\n" : "{\n");

  for(f = s->firstField; f != NULL; f = f->next) {
    int m = PrintField(fieldPrefix, f, &str[n]);
    if(m < 0) {
      free(fieldPrefix);
      return m;     // Error code;
    }
    n += m;
  }

  free(fieldPrefix);
  n += sprintf(&str[n], asArray ? "%s]" : "%s}", prefix);

  return n;
}


static int GetFieldStringSize(int prefixSize, const XField *f) {
  int m;

  if(f == NULL) return 0;
  if(f->name == NULL) {
    Error("Field @0x%p name has no name.\n", f);
    return X_NAME_INVALID;
  }
  if(*f->name == '\0') {
    Error("Empty @0x%p field name.\n", f);
    return X_NAME_INVALID;
  }

  if(!f->value) m = 4; // "null"
  else if(f->ndim > 1) m = GetArrayStringSize(prefixSize, f->value, f->type, f->ndim, f->sizes);
  else switch(f->type) {
    case X_STRUCT: m = GetObjectStringSize(prefixSize, (XStructure *) f->value); break;
    case X_STRING:
    case X_RAW:
      m = GetJsonStringSize((char *) f->value, TERMINATED_STRING);
      break;
    default: {
      if (xGetElementCount(f->ndim, f->sizes) == 0) return 0;
      m = xStringElementSizeOf(f->type);
      if(m < 0) Error("Unrecognised type 0x%02x in field @0x%p.\n", f->type, f);
    }
  }

  if(m < 0) return m;   // Error code

  return m + strlen(f->name) + 4;   // name + " = " + value + "\n"
}


static int PrintField(const char *prefix, const XField *f, char *str) {
  int n = 0, m;

  if(str == NULL) return X_NULL;
  if(f == NULL) return 0;
  if(f->name == NULL) return X_NAME_INVALID;
  if(*f->name == '\0') return X_NAME_INVALID;
  if(f->isSerialized) return X_PARSE_ERROR;        // We don't know what format, so return an error

  n = sprintf(str, "%s%s : ", prefix, f->name);

  m = PrintArray(prefix, f->value, f->type, f->ndim, f->sizes, &str[n]);
  if(m < 0) return m;

  n += m;
  n += sprintf(&str[n], "\n");


  return n;
}


static __inline__ int IsNewLine(XType type, int ndim) {
  return (ndim > 1 || type == X_STRUCT);
}


static __inline__ int SizeOf(XType type, int ndim, const int *sizes) {
  return xElementSizeOf(type) * xGetElementCount(ndim, sizes);
}


static int GetArrayStringSize(int prefixSize, char *ptr, XType type, int ndim, const int *sizes) {
  if(!ptr) return JSON_NULL_LEN;

  if(ndim < 0) {
    Error("Invalid array dimension %d.\n", ndim);
    return X_SIZE_INVALID;
  }
  if(ndim == 0) {
    int m;

    if(type == X_STRUCT) m = GetObjectStringSize(prefixSize, (XStructure *) ptr);
    else if(type == X_STRING || type == X_RAW) m = GetJsonStringSize((char *) ptr, TERMINATED_STRING);
    else return m = xStringElementSizeOf(type);

    if(m < 0) {
      Error("Invalid type 0x%02x\n.", type);
      return m;     // Error code
    }
    return m;
  }
  else {
    const int N = sizes[0];
    const int rowSize = SizeOf(type, ndim-1, &sizes[1]);
    const boolean newLine = IsNewLine(type, ndim);
    int k;

    int n = 4;     // "[ " + .... +  " ]" or "[\n" + ... + "\n]"
    if(newLine) n += prefixSize << 1;    // Indentations if on new lines

    for(k = 0; k < N; k++, ptr += rowSize) {
      int m = GetArrayStringSize(prefixSize + ilen, ptr, type, ndim-1, &sizes[1]);
      if(m < 0) return m;   // Error code
      if(newLine) n += prefixSize + ilen;
      n += m + 3; // + " , " or " ,\n"
    }
    return n;
  }
}


static int PrintArray(const char *prefix, char *ptr, XType type, int ndim, const int *sizes, char *str) {
  const char *str0 = str;

  if(!str) return X_NULL;
  if(!prefix) return X_NULL;

  if(ndim < 0) return X_SIZE_INVALID;

  if(ndim == 0) return (type == X_STRUCT) ? PrintObject(prefix, (XStructure *) ptr, str, FALSE) : PrintPrimitive(ptr, type, str);

  else {
    const int N = sizes[0];
    const int rowSize = SizeOf(type, ndim-1, &sizes[1]);
    const boolean newLine = IsNewLine(type, ndim);

    int k;
    char *rowPrefix;

    // Indentation for elements...
    rowPrefix = malloc(strlen(prefix) + ilen + 1);
    sprintf(rowPrefix, "%s%s", prefix, indent);

    // Special case: empty array
    if(N == 0) {
      for(k = ndim; --k >= 0; ) *(str++) = '[';
      *(str++) = ' ';
      for(k = ndim; --k >= 0; ) *(str++) = ']';
      return str - str0;
    }


    *(str++) = '[';                                     // Opening bracket at current position...

    // Print elements as required.
    for(k = 0; k < N; k++, ptr += rowSize) {
      int m;

      // " ,"
      if(k) str += sprintf(str, ",");

      // " ", or row indented new line
      if(newLine) str += sprintf(str, "\n%s", rowPrefix);
      else *(str++) = ' ';

      // The next element...
      if (type == X_STRUCT) {
        XStructure *s = (XStructure *) ptr;

        // Check if we should print the structure element as just an array. It is possible
        // only if the structure contains a single unnamed field only.
        boolean asObject = s->firstField && (s->firstField->name || s->firstField->next);
        m = PrintObject(rowPrefix, (XStructure *) ptr, str, !asObject);
      }
      else {
        m = PrintArray(rowPrefix, ptr, type, ndim-1, &sizes[1], str);
      }

      if(m < 0) {
        free(rowPrefix);
        return m;       // Error code
      }
      str += m;
    }

    // " ", or indented new line
    if(newLine) str += sprintf(str, "\n%s", prefix);    // For newLine type elemments, close on an indented new line....
    else *(str++) = ' ';                                // Otherwise, just add a space...

    *(str++) = ']';                                     // Close bracket.

    free(rowPrefix);

    return str - str0;
  }
}


static int PrintPrimitive(const void *ptr, XType type, char *str) {
  if(!ptr) return sprintf(str, JSON_NULL);

  if(xIsCharSequence(type)) return PrintString((char *) ptr, xElementSizeOf(type), str);

  switch(type) {
    case X_BOOLEAN: return sprintf(str, (*(boolean *)ptr ? JSON_TRUE : JSON_FALSE));
    case X_BYTE: return sprintf(str, "%hhu", *(unsigned char *) ptr);
    case X_BYTE_HEX: return sprintf(str, "0x%hhx", *(unsigned char *) ptr);
    case X_SHORT: return sprintf(str, "%hd", *(short *) ptr);
    case X_SHORT_HEX: return sprintf(str, "0x%hx", *(unsigned short *) ptr);
    case X_INT: return sprintf(str, "%d", *(int *) ptr);
    case X_INT_HEX: return sprintf(str, "0x%x", *(int *) ptr);
    case X_LONG: return sprintf(str, "%lld", *(long long *) ptr);
    case X_LONG_HEX: return sprintf(str, "0x%llx", *(long long *) ptr);
    case X_FLOAT: return sprintf(str, "%.8g , ", *(float *) ptr);
    case X_DOUBLE: return xPrintDouble(str, *(double *) ptr);
    case X_STRING:
    case X_RAW: return PrintString(*(char **) ptr, TERMINATED_STRING, str);
    default: return X_TYPE_INVALID;
  }
}


static int GetJsonBytes(char c) {
  if(iscntrl(c)) switch(c) {
    case '\b':
    case '\f':
    case '\n':
    case '\t':
    case '\r': return 2;
    default: return 0;
  }

  switch(c) {
    case '\\':
    case '"': return 2;
  }

  return 1;
}


static int GetJsonStringSize(const char *src, int maxLength) {
  int i, n = 2; // ""
  for(i=0; i < maxLength && src[i]; i++) n += GetJsonBytes(src[i]);
  return n;
}


static char GetEscapedChar(char c) {
  switch(c) {
    case '\\': return '\\';
    case '\"': return '"';
    case '/': return '/';
    case '\b': return 'b';
    case '\f': return 'f';
    case '\n': return 'n';
    case '\t': return 't';
    case '\r': return 'r';
  }
  return c;
}


static int PrintString(const char *src, int maxLength, char *json) {
  int i;
  char *next = json;

  if(maxLength < 0) maxLength = INT_MAX;

  *(next++) = '"';

  for(i = 0; i < maxLength && src[i]; i++) switch(GetJsonBytes(src[i])) {
    case 2:
      *(next++) = '\\';
      *(next++) = GetEscapedChar(src[i]);
      break;
    case 1:
      *(next++) = src[i];
      break;
  }

  *(next++) = '"';
  *(next++) = '\0';

  return next - json - 1;
}


/**
 * Converts a native string to its JSON representation.
 *
 * @param src           Pointer to the native (unescaped) string, which may contain special characters.
 * @param maxLength     The number of characters in the input string if not terminated, or &lt;=0
 *                      if always teminated arbitrary length string.
 * @return              The JSON representation of the original string, in which special characters
 *                      appear in escaped form (without the surrounding double quotes).
 *
 * @sa xjsonUnescapeString()
 */
char *xjsonEscapeString(const char *src, int maxLength) {
  int size;
  char *json;

  if(!src) {
    x_error(0, EINVAL, "xjsonEscapeString", "input string is NULL");
    return NULL;
  }

  size = GetJsonStringSize(src, maxLength);
  json = malloc(size + 1);
  if(!json) return NULL;

  if(PrintString(src, maxLength, json) != X_SUCCESS) {
    free(json);
    return NULL;
  }

  return json;
}

/**
 * Converts a an escaped string in JSON representation to a native string
 *
 * @param str     The JSON representation of the string, in which special characters
 *                appear in escaped form (without the surrounding double quotes).
 * @return        The native string, which may contain special characters.
 *
 * @sa xjsonEscapeString()
 */
char *xjsonUnescapeString(const char *str) {
  int lineNumber = 0;

  if(!str) {
    x_error(0, EINVAL, "xjsonUnescapeString", "input string is NULL");
    return NULL;
  }

  return ParseString((char **) &str, &lineNumber);
}
