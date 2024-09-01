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
  int status;

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

  f = xCreateDoubleField("other", 1.0);
  xLookupPut(l, NULL, f, NULL);
  if(f != xLookupField(l, "other")) {
    fprintf(stderr, "ERROR! put other\n");
    return 1;
  }

  xLookupRemove(l, "other");
  if(xLookupField(l, "other") != NULL) {
    fprintf(stderr, "ERROR! remove other\n");
    return 1;
  }
  xDestroyLookup(l);

  l = xAllocLookup(16);
  status = xLookupPutAll(l, NULL, s, TRUE);
  if(status < 0) {
    fprintf(stderr, "ERROR! putAll: %d: %s\n", status, xErrorDescription(status));
    return 1;
  }

  f = xLookupField(l, "system:subsystem");
  if(!f) {
    fprintf(stderr, "ERROR! putAll: subsystem not found\n");
    return 1;
  }
  else if(f->value != (void *) sub) {
    fprintf(stderr, "ERROR! putAll: l1: %p != %p\n", f->value, sub);
    return 1;
  }

  xLookupRemoveAll(l, NULL, s, TRUE);
  if(xLookupField(l, "system") != NULL) {
    fprintf(stderr, "ERROR! removeAll: system\n");
    return 1;
  }

  xDestroyLookup(l);

  return 0;
}
