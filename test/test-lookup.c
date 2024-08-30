/**
 * @file
 *
 * @date Created  on Aug 30, 2024
 * @author Attila Kovacs
 */

#include <stdio.h>
#include <stdlib.h>

#include "xchange.h"

int main(int argc, char *argv[]) {
  XStructure *s, *sys, *sub;
  XLookupTable *l;
  XField *f;

  s = xCreateStruct();

  sys = xCreateStruct();
  xSetSubstruct(s, "system", sys);

  sub = xCreateStruct();
  xSetSubstruct(sys, "subsystem", sub);

  xSetField(sub, xCreateIntField("property", 1));

  l = xCreateLookup(s, FALSE);
  f = xLookupField(l, "system");
  if(!f) {
    fprintf(stderr, "ERROR! system not found\n");
    return 1;
  }
  else if(f->value != (void *) sys) {
    fprintf(stderr, "ERROR! l1: %p != %p\n", f->value, sys);
    return 1; 
  }
  xDestroyLookup(l);

  l = xCreateLookup(s, TRUE);
  f = xLookupField(l, "system:subsystem");
  if(!f) {
    fprintf(stderr, "ERROR! subsystem not found\n");
    return 1;
  }
  else if(f->value != (void *) sub) {
    fprintf(stderr, "ERROR! l1: %p != %p\n", f->value, sub);
    return 1;
  }
  xDestroyLookup(l);

  return 0;
}
