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
#include <semaphore.h>

#define __XCHANGE_INTERNAL_API__      ///< Use internal definitions
#include "xchange.h"

/// \cond PRIVATE

typedef struct LookupEntry {
  long hash;
  char *key;                ///< dynamically allocated.
  XField *field;
  struct LookupEntry *next;
} XLookupEntry;

typedef struct {
  XLookupEntry **table;
  int nBins;
  int nEntries;
  sem_t sem;
} XLookupPrivate;

/// \endcond

static long xGetHash(const char *str) {
  int i;
  long hash = 0;
  if(!str) return 0;
  for(i=0; str[i]; i++) hash += str[i] ^ i;
  return hash;
}

static XLookupEntry *xGetLookupEntryAsync(const XLookupTable *tab, const char *key, long hash) {
  const XLookupPrivate *p;
  XLookupEntry *e;

  p = (XLookupPrivate *) tab->priv;

  for(e = p->table[(int) (hash % p->nBins)]; e != NULL; e=e->next) if(strcmp(e->key, key) == 0) return e;

  return NULL;
}

static int xLookupPutAsync(XLookupTable *tab, const char *prefix, const XField *field, XField **oldValue) {
  XLookupPrivate *p = (XLookupPrivate *) tab->priv;
  const char *id = prefix ? xGetAggregateID(prefix, field->name) : strdup(field->name);
  long hash = xGetHash(id);
  XLookupEntry *e = xGetLookupEntryAsync(tab, id, hash);
  int idx;

  if(e) {
    if(oldValue) *oldValue = e->field;
    e->field = (XField *) field;
    return 1;
  }

  e = (XLookupEntry *) calloc(1, sizeof(XLookupEntry));
  e->hash = hash;
  e->key = (char *) id;
  e->field = (XField *) field;

  idx = hash % p->nBins;

  e->next = p->table[idx];
  p->table[idx] = e;
  p->nEntries++;

  return 0;
}

static XField *xLookupRemoveAsync(XLookupTable *tab, const char *id) {
  XLookupPrivate *p = (XLookupPrivate *) tab->priv;
  XLookupEntry *e, *last = NULL;
  long hash = xGetHash(id);
  int idx = (int) (hash % p->nBins);

  for(e = p->table[idx]; e != NULL; e=e->next) {
    if(strcmp(e->key, id) == 0) {
      p->nEntries--;
      if(last) last->next = e->next;
      else p->table[idx] = e->next;
      return e->field;
    }
    last = e;
  }

  return NULL;
}

/**
 * Returns the number of fields in the lookup table.
 *
 * @param tab   Pointer to the lookup table
 * @return      the number of fields stored.
 */
long xLookupCount(const XLookupTable *tab) {
  const XLookupPrivate *p;

  if(!tab) return 0;
  p = (XLookupPrivate *) tab->priv;
  return p->nEntries;
}

/**
 * Puts a field into the lookup table with the specified aggregate ID prefix. For example, if the prefix
 * is "system:subsystem", and the field's name is "property", then the field will be available as
 * "system:subsystem:property" in the lookup.
 *
 * @param tab         Pointer to a lookup table
 * @param prefix      The aggregate ID prefix before the field's name, not including a separator
 * @param field       The field
 * @param oldValue    (optional) pointer to a buffer in which to return the old field value (if any) stored
 *                    under the same name. It may be NULL if not needed.
 * @return            0 if successfully added a new field, 1 if updated an existing fields, or else
 *                    X_NULL if either of the arguments were NULL, or X_FAILURE if some other error.
 *
 * @sa xLookupPutAll()
 * @sa xLookupRemove()
 */
int xLookupPut(XLookupTable *tab, const char *prefix, const XField *field, XField **oldValue) {
  static const char *fn = "xLookupPutField";

  XLookupPrivate *p;
  int res;

  if(!tab) return x_error(X_NULL, EINVAL, fn, "input table is NULL");
  if(!field) return x_error(X_NULL, EINVAL, fn, "input field is NULL");

  p = (XLookupPrivate *) tab->priv;
  if(sem_wait(&p->sem) != 0) return x_error(X_FAILURE, errno, fn, "sem_wait() error");

  res = xLookupPutAsync(tab, prefix, field, oldValue);
  sem_post(&p->sem);

  return res;
}

/**
 * Removes a field from a lookup table.
 *
 * @param tab   Pointer to the lookup table
 * @param id    The aggregate ID of the field as stored in the lookup
 * @return      The field that was removed, or else NULL if not found.
 *
 * @sa xLookupRemoveAll()
 * @sa xLookupPut()
 */
XField *xLookupRemove(XLookupTable *tab, const char *id) {
  static const char *fn = "xLookupRemoveField";

  XLookupPrivate *p;
  XField *f;

  if(!tab || !id) {
    x_error(X_NULL, EINVAL, fn, "NULL input: tab = %p, id = %p", tab, id);
    return NULL;
  }

  p = (XLookupPrivate *) tab->priv;
  if(sem_wait(&p->sem) != 0) {
    x_error(0, errno, fn, "sem_wait() error");
    return NULL;
  }

  f = xLookupRemoveAsync(tab, id);
  sem_post(&p->sem);

  return f;
}



static int xLookupPutAllAsync(XLookupTable *tab, const char *prefix, const XStructure *s, boolean recursive) {
  XField *f;
  int lp = prefix ? strlen(prefix) : 0;
  int N = 0;

  for(f = s->firstField; f != NULL; f = f->next) {
    xLookupPutAsync(tab, prefix, f, NULL);
    N++;

    if(f->type == X_STRUCT && recursive) {
      XStructure *sub = (XStructure *) f->value;
      char *p1 = (char *) malloc(lp + strlen(f->value) + 2 * sizeof(X_SEP) + 12);

      int count = xGetFieldCount(f);
      while(--count >= 0) {
        int n = 0;
        if(prefix) n = sprintf(p1, "%s" X_SEP, prefix);

        if(f->ndim == 0) sprintf(&p1[n], "%s", f->name);
        else sprintf(&p1[n], "%s" X_SEP "%d", f->name, (count + 1));

        N += xLookupPutAllAsync(tab, p1, &sub[count], TRUE);
      }
    }
  }

  return N;
}

static int xLookupRemoveAllAsync(XLookupTable *tab, const char *prefix, const XStructure *s, boolean recursive) {
  XField *f;
  int lp = prefix ? strlen(prefix) : 0;
  int N = 0;

  for(f = s->firstField; f != NULL; f = f->next) {
    char *id = xGetAggregateID(prefix, f->name);
    xLookupRemoveAsync(tab, id);
    free(id);
    N++;

    if(f->type == X_STRUCT && recursive) {
      XStructure *sub = (XStructure *) f->value;
      char *p1 = (char *) malloc(lp + strlen(f->value) + 2 * sizeof(X_SEP) + 12);

      int count = xGetFieldCount(f);
      while(--count >= 0) {
        int n = 0;
        if(prefix) n = sprintf(p1, "%s" X_SEP, prefix);

        if(f->ndim == 0) sprintf(&p1[n], "%s", f->name);
        else sprintf(&p1[n], "%s" X_SEP "%d", f->name, (count + 1));

        N += xLookupRemoveAllAsync(tab, p1, &sub[count], TRUE);
      }
    }
  }

  return N;
}


/**
 * Puts all fields from a structure into the lookup table with the specified aggregate ID prefix, with or without
 * including embedded substructures. For example,  if the prefix is "system:subsystem", and a field's name is
 * "property", then that field will be available as "system:subsystem:property" in the lookup.
 *
 * @param tab         Pointer to a lookup table
 * @param prefix      The aggregate ID prefix before the field's name, not including a separator
 * @param s           The structure
 * @param recursive   Whether to include fields in all substructures recursively also.
 * @return            the number of fields added (&lt;=0), or else X_NULL if either of the arguments were NULL,
 *                    or X_FAILURE if some other error.
 *
 * @sa xLookupRemoveAll()
 */
int xLookupPutAll(XLookupTable *tab, const char *prefix, const XStructure *s, boolean recursive) {
  static const char *fn = "xLookupPutAll";

  XLookupPrivate *p;
  int n;

  if(!tab) return x_error(X_NULL, EINVAL, fn, "input table is NULL");
  if(!s) return x_error(X_NULL, EINVAL, fn, "input structure is NULL");

  p = (XLookupPrivate *) tab->priv;
  if(sem_wait(&p->sem) != 0) return x_error(X_FAILURE, errno, fn, "sem_wait() error");

  n = xLookupPutAllAsync(tab, prefix, s, recursive);
  sem_post(&p->sem);

  prop_error(fn, n);
  return n;
}

/**
 * Removes all fields of a structure from the lookup table with the specified aggregate ID prefix, with or without
 * including embedded substructures. For example, if the prefix is "system:subsystem", and a field's name is
 * "property", then the field referred to as "system:subsystem:property" in the lookup is affected.
 *
 * @param tab         Pointer to a lookup table
 * @param prefix      The aggregate ID prefix before the field's name, not including a separator
 * @param s           The structure
 * @param recursive   Whether to include fields in all substructures recursively also.
 * @return            the number of fields removed (&lt;=0), or else X_NULL if either of the arguments were NULL,
 *                    or X_FAILURE if some other error.
 *
 * @sa xLookupRemoveAll()
 */
int xLookupRemoveAll(XLookupTable *tab, const char *prefix, const XStructure *s, boolean recursive) {
  static const char *fn = "xLookupRemoveAll";

  XLookupPrivate *p;
  int n;

  if(!tab) return x_error(X_NULL, EINVAL, fn, "input table is NULL");
  if(!s) return x_error(X_NULL, EINVAL, fn, "input structure is NULL");

  p = (XLookupPrivate *) tab->priv;
  if(sem_wait(&p->sem) != 0) return x_error(X_FAILURE, errno, fn, "sem_wait() error");

  n = xLookupRemoveAllAsync(tab, prefix, s, recursive);
  sem_post(&p->sem);

  prop_error(fn, n);
  return n;
}

/**
 * Allocates a new lookup with the specified hash size. The hash size should correspond to the number of elements
 * stored in the lookup. If it's larger or roughtly equal to the number of elements to be stored, then the lookup
 * time will stay approximately constant with the number of elements. If the size is much smaller than the number
 * of elements _N_ stored, then the lookup time will scale as _O(N/size)_ typically.
 *
 * @param size    The number of hash bins to allocate
 * @return        The new lookup table, or else NULL if there was an error.
 *
 * @sa xCreateLookup()
 * @sa xDestroyLookup()
 */
XLookupTable *xAllocLookup(unsigned int size) {
  XLookupTable *tab;
  XLookupPrivate *p;

  unsigned int n = 2;

  if(size < 1) {
    x_error(0, errno, "xAllocLookup", "invalid size: %d", size);
    return NULL;
  }

  while(n < size) n <<= 1;

  p = (XLookupPrivate *) calloc(1, sizeof(XLookupPrivate));
  x_check_alloc(p)

  p->table = (XLookupEntry **) calloc(n, sizeof(XLookupEntry *));
  x_check_alloc(p->table);

  p->nBins = n;
  sem_init(&p->sem, FALSE, 1);

  tab = (XLookupTable *) calloc(1, sizeof(XLookupTable));
  x_check_alloc(tab);

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
  static const char *fn = "xCreateLookup";

  XLookupTable *l;

  if(s == NULL) {
    x_error(0, EINVAL, fn, "input structure is NULL");
    return NULL;
  }

  l = xAllocLookup(recursive ? xDeepCountFields(s) : xCountFields(s));
  if(!l) return x_trace_null(fn, NULL);

  xLookupPutAllAsync(l, NULL, s, recursive);

  return l;
}

/**
 * Returns a named field from a lookup table. When retriving a large number (hundreds or more) fields by name
 * from very large structures, this methods of locating the relevant data can be significantly faster than the
 * xGetField() / xGetSubstruct() approach.
 *
 * Note however, that preparing the lookup table has significant _O(N)_ computational cost also, whereas
 * retrieving _M_ fields with xGetField() / xGetSubstruct() have costs that scale _O(N&times;M)_. Therefore, a
 * lookup table is practical only if you are going to use it repeatedly, many times over. As a rule of thumb,
 * lookups may have the advantage if accessing fields in a structure by name hundreds of times, or more.
 *
 * @param tab       Pointer to the lookup table
 * @param id        The aggregate ID of the field to find.
 * @return          The corresponding field or NULL if no such field exists or tab was NULL.
 *
 * @sa xCreateLookup()
 * @sa xGetField()
 */
XField *xLookupField(const XLookupTable *tab, const char *id) {
  static const char *fn = "xLookupField";

  XLookupEntry *e;
  XLookupPrivate *p;

  if(!tab || !id) {
    x_error(0, EINVAL, fn, "null parameter: tab=%p, id=%p", tab, id);
    return NULL;
  }

  p = (XLookupPrivate *) tab->priv;
  if(sem_wait(&p->sem) != 0) {
    x_error(0, errno, fn, "sem_wait() error");
    return NULL;
  }

  e = xGetLookupEntryAsync(tab, id, xGetHash(id));
  sem_post(&p->sem);

  return e ? e->field : NULL;
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
    sem_destroy(&p->sem);
  }

  free(tab);
}
