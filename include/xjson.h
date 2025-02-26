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

#define XJSON_DEFAULT_INDENT         2  ///< Number of characters to indent.

#ifndef NULLDEV
#  define NULLDEV "/dev/null"           ///< null device on system
#endif

void xjsonSetIndent(int nchars);
int xjsonGetIndent();

char *xjsonToString(const XStructure *s);
char *xjsonFieldToString(const XField *f);
char *xjsonFieldToIndentedString(int indent, const XField *f);
XStructure *xjsonParsePath(const char *fileName);
XStructure *xjsonParseFile(FILE *file, size_t length);
XStructure *xjsonParseAt(char **src);
XField *xjsonParseFieldAt(char **src);
void xjsonSetErrorStream(FILE *fp);

char *xjsonEscape(const char *src, int maxLength);
char *xjsonUnescape(const char *json);

#endif /* XJSON_H_ */
