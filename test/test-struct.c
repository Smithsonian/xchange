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

int main(int argc, char *argv[]) {
  XStructure *s, *sys, *sub;

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

  return 0;
}
