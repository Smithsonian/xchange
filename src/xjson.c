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


#define UNICODE_BYTES   6         ///< '\u####'


#define Error(format, ARGS...)      fprintf(xerr ? xerr : stderr, ERROR_PREFIX format, ##ARGS)
#define Warning(format, ARGS...)    fprintf(xerr ? xerr : stderr, WARNING_PREFIX format, ##ARGS)
/// \endcond

static XStructure *ParseObject(char **pos, int *lineNumber);
static XField *ParseField(char **pos, int *lineNumber);
static void *ParseValue(char **pos, XType *type, int *ndim, int sizes[X_MAX_DIMS], int *lineNumber);
static void *ParseArray(char **pos, XType *type, int *ndim, int sizes[X_MAX_DIMS], int *lineNumber);
static char *ParseString(char **pos, int *lineNumber);
static void *ParsePrimitive(char **pos, XType *type, int *lineNumber);

static int GetObjectStringSize(int prefixSize, const XStructure *s);
static int GetFieldStringSize(int prefixSize, const XField *f, boolean ignoreName);
static int GetArrayStringSize(int prefixSize,char *ptr, XType type, int ndim, const int *sizes);
static int GetJsonStringSize(const char *src, int maxLength);

static int PrintObject(const char *prefix, const XStructure *s, char *str);
static int PrintField(const char *prefix, const XField *f, char *str);
static int PrintArray(const char *prefix, char *ptr, XType type, int ndim, const int *sizes, char *str);
static int PrintPrimitive(const void *ptr, XType type, char *str);
static int PrintString(const char *src, int maxLength, char *json);

static FILE *xerr;     ///< File / stream, which errors are printed to. A NULL will print to stderr

static char *indent;   ///< use xjsonGetIndent() for non-null access.
static int ilen = XJSON_DEFAULT_INDENT;

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
  char *old = indent;

  if(nchars < 0) nchars = 0;

  indent = (char *) realloc(indent, nchars + 1);

  if(indent) {
    if(nchars > 0) memset(indent, ' ', nchars);
    indent[nchars] = '\0';
    ilen = nchars;
  }
  else {
    x_error(0, errno, "xjsonSetIndent", "alloc error (%d bytes)", (nchars + 1));
    indent = old;
    ilen = old ? strlen(old) : 0;
  }
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

static char *GetIndent() {
  if(!indent) xjsonSetIndent(XJSON_DEFAULT_INDENT);
  return indent;
}

/**
 * Converts structured data into its JSON representation. Conversion errors are reported to stderr
 * or the altenate stream set by xSetErrorStream().
 *
 * @param s     Pointer to structured data
 * @return      String JSON representation, or NULL if there was an error (errno set to EINVAL).
 *
 * @sa xjsonFieldToString()
 * @sa xjsonSetIndent()
 * @sa xjsonParseAt()
 * @sa xjsonParseFile()
 * @sa xjsonParseFilename()
 */
char *xjsonToString(const XStructure *s) {
  char *str;
  int n;

  if(!s) return xStringCopyOf(JSON_NULL);
  if(!xerr) xerr = stderr;

  n = GetObjectStringSize(0, s);
  if(n < 0) {
    Error("%s\n", xErrorDescription(n));
    errno = EINVAL;
    return NULL;
  }

  str = (char *) malloc(n + 2);     // + '\n' + '\0'
  if(!str) {
    Error("Out of memory (need %ld bytes).\n", (long) (n+1));
    return NULL;
  }

  n = PrintObject("", s, str);
  if (n < 0) {
    free(str);
    return NULL;
  }

  sprintf(&str[n], "\n");

  return str;
}

/**
 * Converts an XField into its JSON representation, with the specified indentation of white spaces
 * in front of every line. Conversion errors are reported to stderr or the altenate stream set by
 * xSetErrorStream().
 *
 * @param indent  Number of white spaces to insert in front of each line.
 * @param f       Pointer to field
 * @return        String JSON representation, or NULL if there was an error (errno set to EINVAL).
 *
 * @sa xjsonFieldToString()
 */
char *xjsonFieldToIndentedString(int indent, const XField *f) {
  static const char *fn = "xjsonFieldToIndentedString";

  char *prefix, *str;
  int n;

  if(!f) return xStringCopyOf(JSON_NULL);
  if(!xerr) xerr = stderr;
  if(indent < 0) indent = 0;

  n = GetFieldStringSize(indent, f, FALSE);
  if(n < 0) {
    Error("%s\n", xErrorDescription(n));
    errno = EINVAL;
    return NULL;
  }

  prefix = malloc(indent + 1);
  if(!prefix) {
    x_error(0, errno, fn, "alloc error (%d) bytes", (n + 1));
    return NULL;
  }
  if(indent > 0) memset(prefix, ' ', indent);
  prefix[indent] = '\0';

  str = (char *) malloc(n + 1);  // + '\0'
  if(!str) {
    x_error(0, errno, fn, "alloc error (%d) bytes", (n + 1));
    free(prefix);
    return NULL;
  }

  n = PrintField(prefix, f, str);
  free(prefix);

  if (n < 0) {
    free(str);
    return NULL;
  }

  return str;
}

/**
 * Converts an XField into its JSON representation. Conversion errors are reported to stderr
 * or the altenate stream set by xSetErrorStream().
 *
 * @param f     Pointer to field
 * @return      String JSON representation, or NULL if there was an error (errno set to EINVAL).
 *
 * @sa xjsonToString()
 * @sa xjsonSetIndent()
 * @sa xjsonParseAt()
 * @sa xjsonParseFile()
 * @sa xjsonParseFilename()
 */
char *xjsonFieldToString(const XField *f) {
  return xjsonFieldToIndentedString(0, f);
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
 * @sa xjsonParseFieldAt()
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
 * Parses a JSON field from the given parse position, returning the field's data in the xchange
 * format and updating the parse position. Parse errors are reported to stderr or the alternate
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
 * @sa xjsonParseAt()
 * @sa xjsonToString()
 * @sa xjsonParseFile()
 * @sa xjsonParseFileName()
 */
XField *xjsonParseFieldAt(char **pos, int *lineNumber) {
  int n;

  if(!pos) {
    x_error(0, EINVAL, "xjsonParseAt", "'pos' parameter is NULL");
    return NULL;
  }

  if(!xerr) xerr = stderr;

  if(!lineNumber) lineNumber = &n;
  *lineNumber = 1;      // Start at line 1...

  return ParseField(pos, lineNumber);
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

  for(L=0; L < (long) length; L++) {
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
  int l = 0;
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

static XField *ParseField(char **pos, int *lineNumber) {
  XField *f;

  *pos = SkipSpaces(*pos, lineNumber);

  f = calloc(1, sizeof(XField));
  x_check_alloc(f);

  f->name = ParseString(pos, lineNumber);
  *pos = SkipSpaces(*pos, lineNumber);

  if(**pos != ':') {
    char *token = GetToken(*pos);
    Warning("[L.%d] Missing key:value separator ':' near '%s'\n", *lineNumber, token);
    free(token);
    xDestroyField(f);
    return NULL;
  }

  (*pos)++;
  *pos = SkipSpaces(*pos, lineNumber);

  switch(**pos) {
    case '{':
      f->type = X_STRUCT;
      f->value = (void *) ParseObject(pos, lineNumber);
      break;
    case '[':
      f->value = ParseArray(pos, &f->type, &f->ndim, f->sizes, lineNumber);
      break;
    case '"': {
      char **str = malloc(sizeof(char *));
      x_check_alloc(str);

      *str = ParseString(pos, lineNumber);
      f->type = X_STRING;
      f->value = (char *) str;
      break;
    }
    default:
      f->value = ParsePrimitive(pos, &f->type, lineNumber);
  }

  return f;
}

static XStructure *ParseObject(char **pos, int *lineNumber) {
  XStructure *s;

  *pos = SkipSpaces(*pos, lineNumber);

  if(**pos != '{') {
    char *bad = GetToken(*pos);
    Error("[L.%d] Expected '{', got \"%s\"\n", *lineNumber, bad);
    free(bad);
    return NULL;
  }

  (*pos)++; // Opening {

  s = (XStructure *) calloc(1, sizeof(XStructure));
  x_check_alloc(s);

  while(**pos) {
    XField *f;

    *pos = SkipSpaces(*pos, lineNumber);

    if(**pos == '}') {
      (*pos)++;
      break;
    }

    if(**pos == ',') {
      Warning("[L.%d] Empty field.\n", *lineNumber);
      (*pos)++;
      continue;
    }

    f = ParseField(pos, lineNumber);
    if(!f) break;

    f = xSetField(s, f);
    if(f) xDestroyField(f); // If duplicate field, destroy the prior one.

    // Spaces after field...
    *pos = SkipSpaces(*pos, lineNumber);

    // There should be either a comma or a closing bracket after the field...
    if(**pos == ',') (*pos)++;
    else if(**pos == '}') {
      (*pos)++;
      break;
    }
    else Warning("[L.%d] Missing comma or closing bracket after field.\n", *lineNumber);
  }

  return s;
}

static char UnescapedChar(char c) {
  switch(c) {
    case '\\': return '\\';
    case '"': return '\"';
    case '/': return '/';
    case 'a': return '\a';  // not strictly JSON...
    case 'b': return '\b';
    case 'f': return '\f';
    case 'n': return '\n';
    case 't': return '\t';
    case 'r': return '\r';
  }
  return c;
}

static char *json2raw(const char *json, int maxlen, char *dst) {
  int isEscaped = 0;
  int i, l = 0;

  for(i = 0; l < maxlen; i++) {
     char c = json[i];

     if(isEscaped) {
       isEscaped = FALSE;
       if(c == 'u') {
         unsigned short unicode;
         if(sscanf(&json[i+1], "%hx", &unicode) == 1) {
           if(unicode <= 0xFF) dst[l++] = (char) unicode;   // -> ASCII
           else l += sprintf(&dst[l], "\\u%04hx", unicode); // -> keep as is
         }
         else return "Unicode \\u without 4 digit hex";
       }
       else {
         dst[l++] = UnescapedChar(c);
       }
     }
     else if(c == '\0') break;
     else if(c == '\\') isEscaped = TRUE;
     else dst[l++] = c;
   }

   dst[l] = '\0';
   return NULL;
}

static char *ParseString(char **pos, int *lineNumber) {
  int isEscaped = 0;
  int i, l;
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

  next = json2raw(next, l, dst);
  if(next) Error("[L.%d] %s.\n", *lineNumber, next);

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
  errno = 0;
  ll = strtoll(next, &end, 0);
  if(end == *pos && !errno) {
    if(ll == (int) ll) {
      // If we can represent as int, then prefer it.
      int *value = (int *) malloc(sizeof(int));
      x_check_alloc(value);
      *value = (int) ll;
      *type = X_INT;
      return value;
    }
    else {
      long long *value = (long long *) malloc(sizeof(long long));
      x_check_alloc(value);
      *value = ll;
      *type = X_LONG;
      return value;
    }
  }

  // Try parse as double...
  errno = 0;
  d = strtod(next, &end);
  if(end == *pos && !errno) {
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
  if(*next == '"') {
    char **ptr = (char **) calloc(1, sizeof(char *));
    x_check_alloc(ptr);
    *ptr = ParseString(pos, lineNumber);
    *type = X_STRING;
    return ptr;
  }

  return ParsePrimitive(pos, type, lineNumber);
}


static XType GetCommonType(XType t1, XType t2) {
  if(t1 == X_UNKNOWN) return t2;
  if(t2 == X_UNKNOWN) return t1;
  if(t1 == X_FIELD || t2 == X_FIELD) return X_FIELD;
  if(t1 == X_STRUCT || t2 == X_STRUCT) return X_FIELD;
  if(t1 == X_STRING || t2 == X_STRING) return X_FIELD;
  if(t1 == X_DOUBLE || t2 == X_DOUBLE) return X_DOUBLE;
  if(t1 == X_FLOAT || t2 == X_FLOAT) return X_FLOAT;
  if(t1 == X_LONG || t2 == X_LONG) return X_LONG;
  if(t1 == X_INT || t2 == X_INT) return X_INT;
  if(t1 == X_SHORT || t2 == X_SHORT) return X_SHORT;
  if(t1 == X_BYTE || t2 == X_BYTE) return X_BYTE;
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

  next = SkipSpaces(next+1, lineNumber);

  for(n = 0; *next && *next != ']';) {
    XField *e;
    boolean isValid;

    e = (XField *) calloc(1, sizeof(XField));
    x_check_alloc(e);

    e->value = ParseValue(&next, &e->type, &e->ndim, e->sizes, lineNumber);
    isValid = (e->value || errno != EINVAL);

    if(isValid) n++;

    // Add to the tail of the list...
    if(!last) first = e;
    else last->next = e;
    last = e;

    // If types and dimensions all match, then will return an array of that type (e.g. double[], or int[]).
    // Otherwise return an array of the common type...
    if(*type != X_FIELD && *type != X_STRUCT) {
      if(*type == X_UNKNOWN) *type = e->type;
      else if(*type != e->type) *type = GetCommonType(*type, e->type);

      if(*ndim == 0) {
        *ndim = e->ndim;
        memcpy(sizes, e->sizes, e->ndim * sizeof(int));
      }

      // Heterogeneous arrays are treated as an array of fields.
      else if(*ndim != e->ndim) {
        *type = X_FIELD;
      }
      else if(memcmp(sizes, e->sizes, e->ndim * sizeof(int))) {
        *type = X_FIELD;
      }
    }

    // After the value there must be either a comma or a closing bracket
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
      goto cleanup; // @suppress("Goto statement used")
    }
  }

  *pos = ++next;

  // For heterogeneous arrays, return an array of XFields...
  if(*type == X_FIELD) {
    XField *array, *e = first;
    int i;

    array = (XField *) calloc(n, sizeof(XField));
    x_check_alloc(array);

    memset(sizes, 0, X_MAX_DIMS * sizeof(int));
    *ndim = 1;
    sizes[0] = n;

    for(i = 0; i < n; i++) {
      char idx[20];
      XField *nextField = e->next;

      // Name is . + 1-based index, e.g. ".1", ".2"...
      sprintf(idx, ".%d", (i + 1));

      array[i] = *e;
      array[i].name = xStringCopyOf(idx);
      array[i].next = NULL;

      free(e);
      e = nextField;
    }

    // Discard any unassigned elements
    while(e) {
      XField *nextField = e->next;
      xDestroyField(e);
      e = nextField;
    }

    return array;
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
        goto cleanup; // @suppress("Goto statement used")
      }
    }

    for(i = 0; i < n; i++) if(first) {
      XField *e = first;
      first = e->next;

      if(e->value) {
        memcpy(data + i * rowSize, *type == X_FIELD ? (char *) e : e->value, rowSize);
        free(e->value);
        e->value = NULL;
      }
      xDestroyField(e);
    }

    // Discard unused parsed elements
    while(first) {
      XField *nextField = first->next;
      xDestroyField(first);
      first = nextField;
    }

    return data;
  }

  // -------------------------------------------------------------------------
  cleanup:

  while(first) {
    XField *e = first;
    first = e->next;
    xDestroyField(e);
  }

  return NULL;
}


static int GetObjectStringSize(int prefixSize, const XStructure *s) {
  int n;
  XField *f;

  if(!s) return 0;

  n = prefixSize + 4;       // "{\n" + .... + <prefix> + "}\n";

  for(f = s->firstField; f != NULL; f = f->next) {
    int m = GetFieldStringSize(prefixSize + ilen, f, f->type == X_FIELD);
    prop_error("GetObjectStringSize", m);
    n += m;
  }

  return n;
}


static int PrintObject(const char *prefix, const XStructure *s, char *str) {
  static const char *fn = "PrintObject";

  char *fieldPrefix;
  XField *f;
  int n = 0;

  if(!s) return X_SUCCESS;
  if(!str) return x_error(X_NULL, EINVAL, fn, "output string buffer is NULL");
  if(!prefix) return x_error(X_NULL, EINVAL, fn, "prefix is NULL");

  if(!s->firstField) return sprintf(&str[n], "{ }");

  fieldPrefix = (char *) malloc(strlen(prefix) + xjsonGetIndent() + 1);
  x_check_alloc(fieldPrefix);

  sprintf(fieldPrefix, "%s%s", prefix, GetIndent());

  n += sprintf(str, "{\n");

  for(f = s->firstField; f != NULL; f = f->next) {
    int m = PrintField(fieldPrefix, f, &str[n]);
    if(m < 0) {
      free(fieldPrefix);
      return x_trace(fn, NULL, m);     // Error code;
    }
    n += m;
  }

  free(fieldPrefix);
  n += sprintf(&str[n], "%s}", prefix);

  return n;
}


static int GetFieldStringSize(int prefixSize, const XField *f, boolean ignoreName) {
  static const char *fn = "GetFieldStringSize";

  int n = prefixSize + 2, m;      // <value> + `,\n`

  if(f == NULL) return 0;

  if(!ignoreName) {
    if(f->name == NULL) return x_error(X_NAME_INVALID, EINVAL, fn, "field->name is NULL");
    if(*f->name == '\0') return x_error(X_NAME_INVALID, EINVAL, fn, "field->name is empty");
    m = GetJsonStringSize(f->name, TERMINATED_STRING) + 2;   // <"name"> + ': '

    prop_error(fn, m);
    n += m;
  }

  m = GetArrayStringSize(prefixSize, f->value, f->type, f->ndim, f->sizes);
  prop_error(fn, m);

  return n + m; // termination
}


static int PrintField(const char *prefix, const XField *f, char *str) {
  static const char *fn = "PrintField";

  int n = 0, m;

  if(str == NULL) return x_error(X_NULL, EINVAL, fn, "output string buffer is NULL");
  if(f == NULL) return 0;
  if(f->name == NULL) return x_error(X_NAME_INVALID, EINVAL, fn, "field->name is NULL");
  if(*f->name == '\0') return x_error(X_NAME_INVALID, EINVAL, fn, "field->name is empty");
  if(f->isSerialized) return x_error(X_PARSE_ERROR, ENOMSG, fn, "field is serialized (unknown format)");        // We don't know what format, so return an error

  n = sprintf(str, "%s", prefix);
  n += PrintString(f->name, -1, &str[n]);
  n += sprintf(&str[n], ": ");

  m = PrintArray(prefix, f->value, f->type, f->ndim, f->sizes, &str[n]);
  prop_error(fn, m);

  n += m;
  if(f->next) n += sprintf(&str[n], ",");
  n += sprintf(&str[n], "\n");

  return n;
}


static __inline__ int IsNewLine(XType type, int ndim) {
  return (ndim > 1 || type == X_STRUCT || type == X_FIELD);
}


static __inline__ int SizeOf(XType type, int ndim, const int *sizes) {
  return xElementSizeOf(type) * xGetElementCount(ndim, sizes);
}


static int GetArrayStringSize(int prefixSize, char *ptr, XType type, int ndim, const int *sizes) {
  static const char *fn = "GetArrayStringSize";

  if(!ptr) return prefixSize + sizeof(JSON_NULL); // null
  if(ndim < 0) return x_error(X_SIZE_INVALID, EINVAL, fn, "invalid ndim: %d", ndim);

  if(ndim == 0) {
    int m;

    switch(type) {
      case X_UNKNOWN:
        return sizeof(JSON_NULL);

      case X_STRUCT:
        m = GetObjectStringSize(prefixSize + ilen, (XStructure *) ptr);
        prop_error(fn, m);
        return m;

      case X_STRING:
      case X_RAW:
        m = GetJsonStringSize(*(char **) ptr, TERMINATED_STRING);
        prop_error(fn, m);
        return m;

      case X_FIELD: {
        const XField *f = (XField *) ptr;
        m = GetFieldStringSize(prefixSize + ilen, f, TRUE);
        prop_error(fn, m);
        return m;
      }

      default:
        m = xStringElementSizeOf(type);
        prop_error(fn, m);
        return m;
    }
  }
  else {
    const int N = sizes[0];
    const int rowSize = SizeOf(type, ndim-1, &sizes[1]);
    const boolean newLine = IsNewLine(type, ndim);
    int k;

    int n = 4;     // "[ " + .... +  " ]" or "[\n" + ... + "\n]"
    if(newLine) n += prefixSize + 1;            // '\n' + prefix

    for(k = 0; k < N; k++, ptr += rowSize) {
      int m = GetArrayStringSize(prefixSize + ilen, ptr, type, ndim-1, &sizes[1]);
      prop_error(fn, m);

      n += m + 3; // + " , " or " ,\n"
    }

    return n;
  }
}


static int PrintArray(const char *prefix, char *ptr, XType type, int ndim, const int *sizes, char *str) {
  static const char *fn = "PrintArray";

  const char *str0 = str;

  if(!str) return x_error(X_NULL, EINVAL, fn, "output string buffer is NULL");
  if(!prefix) return x_error(X_NULL, EINVAL, fn, "prefix is NULL");

  if(ndim < 0) return x_error(X_SIZE_INVALID, ERANGE, fn, "invalid ndim: %d", ndim);

  if(ndim == 0) {
    int n;

    switch(type) {
      case X_STRUCT:
        n = PrintObject(prefix, (XStructure *) ptr, str);
        break;
      case X_FIELD: {
        XField *f = (XField *) ptr;
        n = PrintArray(prefix, f->value, f->type, f->ndim, f->sizes, str);
        break;
      }
      default:
        n = PrintPrimitive(ptr, type, str);
    }

    prop_error(fn, n);
    return n;
  }
  else {
    const int N = sizes[0];
    const int rowSize = ptr ? SizeOf(type, ndim-1, &sizes[1]) : 0;
    const boolean newLine = ptr ? IsNewLine(type, ndim) : FALSE;

    int k;
    char *rowPrefix;

    // Special case: empty array
    if(N == 0) {
      for(k = ndim; --k >= 0; ) *(str++) = '[';
      *(str++) = ' ';
      for(k = ndim; --k >= 0; ) *(str++) = ']';
      return str - str0;
    }

    // Indentation for elements...
    rowPrefix = (char *) malloc(strlen(prefix) + xjsonGetIndent() + 2);
    x_check_alloc(rowPrefix);

    sprintf(rowPrefix, "%s%s", prefix, GetIndent());

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
        m = PrintObject(rowPrefix, (XStructure *) ptr, str);
      }
      else {
        m = PrintArray(rowPrefix, ptr, type, ndim-1, &sizes[1], str);
      }
      if(m < 0) {
        free(rowPrefix);
        return x_trace(fn, NULL, m);       // Error code
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
  static const char *fn = "PrintPrimitive";

  if(!ptr) return sprintf(str, JSON_NULL);

  if(xIsCharSequence(type)) {
    prop_error(fn, PrintString((char *) ptr, xElementSizeOf(type), str));
    return X_SUCCESS;
  }

  switch(type) {
    case X_UNKNOWN: return sprintf(str, JSON_NULL);
    case X_BOOLEAN: return sprintf(str, (*(boolean *)ptr ? JSON_TRUE : JSON_FALSE));
    case X_BYTE: return sprintf(str, "%hhu", *(unsigned char *) ptr);
    case X_SHORT: return sprintf(str, "%hd", *(short *) ptr);
    case X_INT: return sprintf(str, "%d", *(int *) ptr);
    case X_LONG: return sprintf(str, "%lld", *(long long *) ptr);
    case X_FLOAT: return sprintf(str, "%.8g , ", *(float *) ptr);
    case X_DOUBLE: return xPrintDouble(str, *(double *) ptr);
    case X_STRING:
    case X_RAW: return PrintString(*(char **) ptr, TERMINATED_STRING, str);
    default: return x_error(X_TYPE_INVALID, EINVAL, fn, "invalid type: %d", type);
  }
}


static int GetJsonBytes(char c) {
  if(iscntrl(c)) switch(c) {
    case '\b':
    case '\f':
    case '\n':
    case '\t':
    case '\r': return 2;
    default: return UNICODE_BYTES;
  }

  switch(c) {
    case '\\':
    case '"': return 2;
  }

  return 1;
}


static int GetJsonStringSize(const char *src, int maxLength) {
  int i, n = 2; // ""

  if(maxLength < 0) {
    for(i = 0; src[i]; i++)
      n += GetJsonBytes(src[i]);
  }
  else for(i = 0; i < maxLength && src[i]; i++)
    n += GetJsonBytes(src[i]);

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


static int raw2json(const char *src, int maxlen, char *json) {
  char *next = json;
  int i;

  for(i = 0; i < maxlen && src[i]; i++) switch(GetJsonBytes(src[i])) {
    case UNICODE_BYTES:
      *(next++) = '\\';
      *(next++) = 'u';
      next += sprintf(next, "00%02hhx", (unsigned char) src[i]);
      break;
    case 2:
      *(next++) = '\\';
      *(next++) = GetEscapedChar(src[i]);
       break;
    case 1:
      *(next++) = src[i];
      break;
  }

  *(next++) = '\0';

  return next - json - 1;
}

static int PrintString(const char *src, int maxLength, char *json) {
  char *next = json;

  if(maxLength < 0) maxLength = INT_MAX;

  *(next++) = '"';

  next += raw2json(src, maxLength, next);

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
 * @sa xjsonUnescape()
 */
char *xjsonEscape(const char *src, int maxLength) {
  static const char *fn = "xjsonEscape";

  int size;
  char *json;

  if(!src) {
    x_error(0, EINVAL, fn, "input string is NULL");
    return NULL;
  }

  if(maxLength <= 0) maxLength = INT_MAX;

  size = GetJsonStringSize(src, maxLength);

  json = malloc(size + 1);
  if(!json) {
    x_error(0, errno, fn, "malloc() error (%d bytes)", (size + 1));
    return NULL;
  }

  raw2json(src, maxLength, json);

  return json;
}

/**
 * Converts a an escaped string in JSON representation to a native string
 *
 * @param str     The JSON representation of the string, in which special characters
 *                appear in escaped form (without the surrounding double quotes).
 * @return        The native string, which may contain special characters.
 *
 * @sa xjsonEscape()
 */
char *xjsonUnescape(const char *str) {
  static const char *fn = "xjsonUnescape";

  char *raw;
  int l;

  if(!str) {
    x_error(0, EINVAL, fn, "input string is NULL");
    return NULL;
  }

  l = strlen(str);
  raw = (char *) malloc(l + 1);
  if(!raw) {
    x_error(0, errno, fn, "alloc error (%d bytes)", (l + 1));
    return NULL;
  }

  str = json2raw(str, l, raw);
  if(str) x_error(0, EBADMSG, fn, str);

  return raw;
}
