/**
 * -file
 *
 * @date Nov 29, 2020
 * @author Attila Kovacs
 *
 */

#ifndef XJSON_H_
#define XJSON_H_

#include "xchange.h"

#define JSON_INDENT         "  "                        ///< Indentation for nested elements.
#define JSON_INDENT_LEN     (sizeof(JSON_INDENT) - 1)   ///< String length of indentation.

#define NULLDEV "/dev/null"                             ///< null device on system

char *xjsonToString(const XStructure *s);
XStructure *xjsonParseFile(char *fileName, int *lineNumber);
XStructure *xjsonParseAt(char **src, int *lineNumber);
void xSetErrorStream(FILE *fp);

#endif /* XJSON_H_ */
