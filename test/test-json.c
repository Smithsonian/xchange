/**
 * @file
 *
 * @date Created  on Apr 23, 2024
 * @author Attila Kovacs
 */

#include <stdio.h>
#include <stdlib.h>

#include "xchange.h"
#include "xjson.h"


static XStructure *createStruct() {
  XStructure *s = xCreateStruct();
  XStructure *sub = xCreateStruct();
  XStructure *esub = xCreateStruct();

  int array[3][10] = {{1, 2, 3}, {4, 5, 6}, {7, 8, 9}};
  int sizes[] = {3, 10};
  int empty[] = {0, 0};

  xSetField(s, xCreateBooleanField("bool", 1));
  xSetField(s, xCreateStringField("string", "Hello world!"));
  xSetField(s, xCreateIntField("int", -10));
  xSetField(s, xCreateLongField("long", 12345678901L));
  xSetField(s, xCreateDoubleField("double", 1.2345e-11));
  xSetField(s, xCreateField("array", X_INT, 2, sizes, array));
  xSetField(s, xCreateField("empty-array", X_INT, 2, empty, array));

  xSetField(sub, xCreateIntField("int", 1154));
  xSetField(sub, xCreateStringField("string", "Here go the \n\t special chars."));
  xSetSubstruct(s, "sub", sub);
  xSetSubstruct(s, "empty", esub);

  return s;
}



int main(int argc, char *argv[]) {
  XStructure *s = createStruct(), *s1;
  char *str = xjsonToString(s), *next;

  printf("%s\n\n", str);

  next = str;
  s1 = xjsonParseAt(&next, NULL);

  printf("%s\n\n", xjsonToString(s1));


  return 0;
}
