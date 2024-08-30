/**
 * @file
 *
 * @date Created  on Aug 30, 2024
 * @author Attila Kovacs
 *
 *   Lookup table for faster field access in large fixed-layout structures.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "xchange.h"

/// \cond PRIVATE

typedef struct LookupEntry {
  long hash;
  char *key;                ///< dynamically allocated.
  XField *value;
  struct LookupEntry *next;
} XLookupEntry;

typedef struct {
  XLookupEntry **table;
  int nBins;
  int nEntries;
} XLookupPrivate;

/// \endcond

static long xGetHash(const char *str) {
  int i;
  long hash = 0;
  if(!str) return 0;
  for(i=0; str[i]; i++) hash += str[i] ^ i;
  return hash;
}


static XLookupEntry *xGetLookupEntry(const XLookupTable *tab, const char *key, long hash) {
  const XLookupPrivate *p;
  XLookupEntry *e;

  p = (XLookupPrivate *) tab->priv;

  for(e = p->table[(int) (hash % p->nBins)]; e != NULL; e=e->next) if(strcmp(e->key, key) == 0) return e;

  return NULL;
}

static int xLookupAddAsync(XLookupTable *tab, const char *prefix, const XField *field, void **oldValue) {
  XLookupPrivate *p = (XLookupPrivate *) tab->priv;
  const char *id = prefix ? xGetAggregateID(prefix, field->name) : strdup(field->name);
  long hash = xGetHash(id);
  XLookupEntry *e = xGetLookupEntry(tab, id, hash);
  int idx;

  if(e) {
    if(oldValue) *oldValue = e->value;
    e->value = (XField *) field;
    return 1;
  }

  e = (XLookupEntry *) calloc(1, sizeof(XLookupEntry));
  e->hash = hash;
  e->key = (char *) id;
  e->value = (XField *) field;

  idx = hash % p->nBins;

  e->next = p->table[idx];
  p->table[idx] = e;
  p->nEntries++;

  return 0;
}

static void xLookupAddAllAsync(XLookupTable *tab, const char *prefix, const XStructure *s, boolean recursive) {
  XField *f;
  int lp = prefix ? strlen(prefix) : 0;

  for(f = s->firstField; f != NULL; f = f->next) {
    xLookupAddAsync(tab, prefix, f, NULL);

    if(f->type == X_STRUCT && recursive) {
      XStructure *sub = (XStructure *) f->value;
      char *p1 = (char *) malloc(lp + strlen(f->value) + 2 * sizeof(X_SEP) + 12);

      int count = xGetFieldCount(f);
      while(--count >= 0) {
        int n = 0;
        if(prefix) n = sprintf(p1, "%s" X_SEP, prefix);

        if(f->ndim == 0) sprintf(&p1[n], "%s", f->name);
        else sprintf(&p1[n], "%s" X_SEP "%d", f->name, (count + 1));

        xLookupAddAllAsync(tab, p1, &sub[count], TRUE);
      }
    }
  }
}

static XLookupTable *xAllocLookup(int size) {
  XLookupTable *tab;
  XLookupPrivate *p;

  unsigned int n = 1;
  while(n < size) n <<= 1;

  p = (XLookupPrivate *) calloc(1, sizeof(XLookupPrivate));
  p->table = (XLookupEntry **) calloc(n, sizeof(XLookupEntry *));
  if(!p->table) {
    perror("ERROR! xAllocLookup");
    free(p);
    return NULL;
  }

  p->nBins = p->table ? n : 0;

  tab = (XLookupTable *) calloc(1, sizeof(XLookupTable));
  tab->priv = p;


  return tab;
}

/**
 * Creates a fast name lookup table for the fields of structure, with or without including fields of embedded
 * substructures also. For structures with a large number of fields, such a lookup can significantly
 * improve access times for retrieving specific fields from a structure. However, the lookup will not
 * track fields added or removed after its creation, and so it is suited for accessing structures with a
 * fixed layout only.
 *
 * Since the lookup table contains references to the structure, you should not destroy the structure as long
 * as the lookup table is used.
 *
 * Once the lookup table is no longer used, the caller should explicitly destroy it with `xDestroyLookup()`
 *
 * @param s           Pointer to a structure, for which to create a field lookup.
 * @param recursive   Whether to include fields from substructures recursively in the lookup.
 * @return            The lookup table, or NULL if there was an error (errno will inform about the type of
 *                    error).
 *
 * @sa xLookupField()
 * @sa xDestroyLookup()
 */
XLookupTable *xCreateLookup(const XStructure *s, boolean recursive) {
  XLookupTable *l;

  if(s == NULL) {
    errno = EINVAL;
    return NULL;
  }

  l = xAllocLookup(recursive ? xDeepCountFields(s) : xCountFields(s));
  if(!l) return NULL;

  xLookupAddAllAsync(l, NULL, s, recursive);

  return l;
}

/**
 * Returns a named field from a lookup table.
 *
 * @param tab       Pointer to the lookup table
 * @param id        The aggregate ID of the field to find.
 * @return          The corresponding field or NULL if no such field exists or tab was NULL.
 *
 * @sa xCreateLookup()
 * @sa xGetField()
 */
XField *xLookupField(const XLookupTable *tab, const char *id) {
  XLookupEntry *e;

  if(!tab || !id) {
    errno = EINVAL;
    return NULL;
  }

  e = xGetLookupEntry(tab, id, xGetHash(id));
  return e ? e->value : NULL;
}

/**
 * Destroys a lookup table, freeing up it's in-memory resources.
 *
 * @param tab     Pointer to the lookup table to destroy.
 *
 * @sa xCreateLookup()
 */
void xDestroyLookup(XLookupTable *tab) {
  XLookupPrivate *p;

  if(!tab) return;

  p = (XLookupPrivate *) tab->priv;
  p->nEntries = 0;
  p->nBins = 0;

  if(p->table) {
    int i;
    for (i = 0; i < p->nBins; i++) {
      XLookupEntry *e = p->table[i];

      while(e != NULL) {
        XLookupEntry *next = e->next;
        if(e->key) free(e->key);
        free(e);
        e = next;
      }
    }

    free(p->table);
  }

  free(tab);
}
