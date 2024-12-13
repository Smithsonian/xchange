/**
 * @file
 *
 * @date Created  on Apr 23, 2024
 * @author Attila Kovacs
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "xchange.h"
#include "xjson.h"


static XStructure *createStruct() {
  XStructure *s = xCreateStruct();
  XStructure *sub = xCreateStruct();
  XStructure *esub = xCreateStruct();

  int array[3][10] = {{1, 2, 3}, {4, 5, 6}, {7, 8, 9}};
  int sizes[] = {3, 10};
  int empty[] = {0, 0};

  int i1[] = {1, 2};
  char *s2[] = {"aa", "bb", "cc"};
  boolean b3[] = {TRUE};

  // Heterogeneous array...
  XField *f1 = xCreate1DField(".1", X_INT, 2, i1);
  XField *f2 = xCreate1DField(".3", X_BOOLEAN, 1, b3);
  XField *f3 = xCreate1DField(".2", X_STRING, 3, s2);
  XField fa[] = {*f1, *f2, *f3 };

  xSetField(s, xCreateBooleanField("bool", 1));
  xSetField(s, xCreateStringField("string", "Hello world!"));
  xSetField(s, xCreateIntField("int", -10));
  xSetField(s, xCreateLongField("long", 12345678901L));
  xSetField(s, xCreateDoubleField("double", 1.2345e-11));
  xSetField(s, xCreateField("array", X_INT, 2, sizes, array));
  xSetField(s, xCreateField("empty-array", X_INT, 2, empty, array));
  xSetField(s, xCreate1DField("hetero-array", X_FIELD, 3, fa));

  xSetField(sub, xCreateIntField("int", 1154));
  xSetField(sub, xCreateStringField("string", "Here go the \n\t special chars."));
  xSetSubstruct(s, "sub", sub);
  xSetSubstruct(s, "empty", esub);

  return s;
}



int main() {
  XStructure *s = createStruct(), *s1;
  char *str, *next, *str1;

  char *specials = "\\\"\r\n\t\b\f";

  xSetDebug(TRUE);

  str = xjsonEscape(specials, 0);
  if(strlen(str) != 2 * strlen(specials)) {
    fprintf(stderr, "ERROR: '%s' has length %d, expected %d\n", str, (int) strlen(str), (int) strlen(specials));
    return 1;
  }

  str = xjsonUnescape(str);
  if(strcmp(specials, str) != 0) {
    fprintf(stderr, "ERROR: unescaped string differs from original\n");
    return 1;
  }

  str = xjsonToString(s),
  printf("%s", str);

  next = str;
  s1 = xjsonParseAt(&next, NULL);

  str1 = xjsonToString(s1);
  if(strcmp(str1, str) != 0) {
    fprintf(stderr, "ERROR! str1 != str:\n\n");
    printf(" str1: %s\n\n", str1);
    return 1;
  }

  printf("OK\n");

  return 0;
}
