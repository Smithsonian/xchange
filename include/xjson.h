/**
 * @file
 *
 * @date Nov 29, 2020
 * @author Attila Kovacs
 *
 *  A set of functions for parsing and creating JSON description of data.
 */

#ifndef XJSON_H_
#define XJSON_H_

#include <stdio.h>
#include <xchange.h>

#define XJSON_INDENT         "  "       ///< Indentation for nested elements.

#ifndef NULLDEV
#  define NULLDEV "/dev/null"           ///< null device on system
#endif

void xjsonSetIndent(int nchars);
int xjsonGetIndent();

char *xjsonToString(const XStructure *s);
char *xjsonFieldToString(const XField *f);
char *xjsonFieldToIndentedString(int indent, const XField *f);
XStructure *xjsonParseFilename(const char *fileName, int *lineNumber);
XStructure *xjsonParseFile(FILE *file, size_t length, int *lineNumber);
XStructure *xjsonParseAt(char **src, int *lineNumber);
XField *xjsonParseFieldAt(char **src, int *lineNumber);
void xjsonSetErrorStream(FILE *fp);

char *xjsonEscape(const char *src, int maxLength);
char *xjsonUnescape(const char *json);

#endif /* XJSON_H_ */
