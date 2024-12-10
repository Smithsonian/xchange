/**
 * @file
 *
 * @date Created  on Sep 1, 2024
 * @author Attila Kovacs
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "xchange.h"

int main() {
  XStructure *s, *sys, *sub;
  XField *f;

  s = xCreateStruct();

  sys = xCreateStruct();
  xSetSubstruct(s, "system", sys);

  sub = xCreateStruct();
  xSetSubstruct(sys, "subsystem", sub);
  xSetField(sub, xCreateIntField("aaa", 0));
  xSetField(sub, xCreateIntField("bbb", 1));


  xSortFieldsByName(s, TRUE);
  if(strcmp(sub->firstField->name, "aaa") != 0) {
    fprintf(stderr, "ERROR! sort names: expected 'aaa', got '%s'\n", sub->firstField->name);
    return 1;
  }

  xReverseFieldOrder(s, TRUE);
  if(strcmp(sub->firstField->name, "bbb") != 0) {
    fprintf(stderr, "ERROR! sort names: expected 'bbb', got '%s'\n", sub->firstField->name);
    return 1;
  }

  // -------------------------------------------------------------

  xSetField(s, xCreateStringField("string", "blah-blah"));

  f = xGetField(s, "string");
  if(strcmp(f->name, "string") != 0) {
    fprintf(stderr, "ERROR! string name: expected 'string', got '%s'\n", f->name);
    return 1;
  }
  if(strcmp(*(char **) f->value, "blah-blah") != 0) {
    fprintf(stderr, "ERROR! string name: expected 'blah-blah', got '%s'\n", *(char **) f->value);
    return 1;
  }

  xDestroyStruct(s);

  printf("OK\n");

  return 0;
}
