/**
 * @file
 *
 * @date Nov 25, 2020
 * @author Attila Kovacs
 *
 * \brief
 *      A collection of commonly used functions for generic structured data exchange.
 *
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "xchange.h"


/**
 * Creates a new empty XStructure.
 *
 * \sa smaxDestroyStruct()
 *
 */
__inline__ XStructure *xCreateStruct() {
  return (XStructure *) calloc(1, sizeof(XStructure));
}

/**
 * Returns a deep copy of the supplied structure. Note that this only works with vanilla xchange structures
 * with native storage fields. For example, SMA-X structures store data in serialized forms, and therefore it
 * needs its own implementation for making deep copies of structs!
 *
 * @param s     Pointer to the original structure or NULL.
 * @return      A fully independent (deep) copy of the argument or NULL if the input structure is NULL.
 *
 * @sa xCopyOfField()
 */
XStructure *xCopyOfStruct(const XStructure *s) {
  XStructure *copy;
  XField *f, *last = NULL;

  if(!s) return NULL;

  copy = xCreateStruct();
  if(!copy) {
    fprintf(stderr, "xCopyOfStruct(): %s\n", strerror(errno));
    return NULL;
  }

  for(f=s->firstField; f != NULL; f=f->next) {
    XField *cf = xCopyOfField(f);

    if(f->type == X_STRUCT) if(f->value) {
      // Set the copy as the parent structure for any copied substructures.
      XStructure *sub = (XStructure *) f->value;
      int k;
      for(k = xGetFieldCount(f); --k >= 0; ) sub[k].parent = copy;
    }

    if(last) last->next = cf;
    else copy->firstField = cf;

    last = cf;
  }

  return copy;
}

/**
 * Returns a deep copy of the supplied field. The returned copy is a standalone field, unlinked
 * to another to avoid corrupting any structure in which the original field may reside in.
 * Note, that this only works with vanilla xchange structures with native storage fields. For example,
 * SMA-X structures store data in serialized forms, and therefore it needs its own implementation for
 * making deep copies of fields!
 *
 * @param f     Pointer to the original field or NULL.
 * @return      A fully independent (deep) copy of the argument or NULL if the input field is NULL.
 *
 * @sa xCopyOfStruct()
 */
XField *xCopyOfField(const XField *f) {
  XField *copy;
  int k, n, eCount;

  if(!f) return NULL;

  copy = (XField *) malloc(sizeof(XField));
  if(!copy) {
    perror("xCopyOfField(): malloc(XField)");
    return NULL;
  }

  // Start with a clone...
  memcpy(copy, f, sizeof(XField));

  copy->value = NULL;       // To be assigned below...
  copy->next = NULL;        // Clear the link of the copy to avoid corrupted structures.

  if(f->name) copy->name = xStringCopyOf(f->name);
  if(!f->name) {
    perror("xCopyOfField(): xStringCopyOf(f->name)");
    free(copy);
    return NULL;
  }

  if(!f->value) return copy;


  // Copy data
  // -----------------------------------------------------------------------------------

  if(f->type == X_STRUCT) {
    XStructure *s;
    eCount = xGetFieldCount(f);

    s = calloc(eCount, sizeof(XStructure));
    copy->value = (char *) s;

    for(k = 0; k < eCount; k++) {
      XStructure *e = xCopyOfStruct((XStructure *) f->value);
      if(!e) continue;
      s[k].firstField = e->firstField;
      free(e);
    }

    return copy;
  }

  // -----------------------------------------------------------------------------------

  if(f->isSerialized) {
    copy->value = xStringCopyOf(f->value);
    return copy;
  }

  // -----------------------------------------------------------------------------------

  eCount = xGetFieldCount(f);
  if(eCount <= 0) return copy;

  n = eCount * xElementSizeOf(f->type);
  if(n <= 0) return copy;

  // Allocate the copy value storage.
  copy->value = malloc(n);
  if(!copy->value) {
    fprintf(stderr, "xCopyOfField(%c[%d]): %s\n", xTypeChar(f->type), eCount, strerror(errno));
    return copy;
  }

  if(f->type == X_STRING || f->type == X_RAW) {
    char **src = (char **) f->value;
    char **dst = (char **) copy->value;
    for(k = 0; k < eCount; k++) dst[k] = xStringCopyOf(src[k]);
  }
  else memcpy(copy->value, f->value, n);

  return copy;
}


/**
 * Return the field by the specified name, or NULL if no such field exists.
 *
 * \param s     Structure from which to retrieve a given field.
 * \param id    Name or aggregate ID of the field to retrieve
 *
 * \return      Matching field from the structure or NULL if there is no match or one of the arguments is NULL.
 */
XField *xGetField(const XStructure *s, const char *id) {
  XField *e;

  if(s == NULL) return NULL;
  if(id == NULL) return NULL;

  for(e = s->firstField; e != NULL; e = e->next) if(e->name) if(xMatchNextID(e->name, id) == X_SUCCESS) {
    const char *next = xNextIDToken(id);
    if(!next) return e;

    if(e->type != X_STRUCT) return NULL;
    return xGetField((XStructure *) e, next);
  }

  return NULL;
}

/**
 * Creates a generic field of a given name and type and dimensions using a copy of the specified native array,
 * unless type is X_STRUCT in which case the value is referenced directly inside the field. For X_STRING
 * and X_RAW only the array references to the underlying string/byte buffers are copied into the field.
 *
 * \param name      Field name
 * \param type      Storage type, e.g. X_INT.
 * \param ndim      Number of dimensionas (1:20). If ndim < 1, it will be reinterpreted as ndim=1, size[0]=1;
 * \param sizes     Array of sizes along each dimensions, with at least ndim elements, or NULL with ndim<1.
 * \param value     Pointer to the native data location in memory, or NULL to leave unassigned for now.
 *
 * \return          A newly created field with the copy of the supplied data, or NULL if there was an error.
 */
XField *xCreateField(const char *name, XType type, int ndim, const int *sizes, const void *value) {
  const char *funcName = "xCreateField()";
  XField *f;
  int n;

  if(!name) {
    xError(funcName, X_NAME_INVALID);
    return NULL;
  }

  if(ndim > X_MAX_DIMS) {
    xError(funcName, X_SIZE_INVALID);
    return NULL;
  }

  f = calloc(1, sizeof(XField));
  f->name = xStringCopyOf(name);
  f->type = type;

  if(ndim < 1) {
    f->ndim = 0;
    f->sizes[0] = 1;
  }
  else {
    f->ndim = ndim;
    memcpy(f->sizes, sizes, ndim * sizeof(int));
  }

  if(!value) {
    f->value = NULL;
    return f;
  }

  n = xGetElementCount(ndim, sizes) * xElementSizeOf(type);
  f->value = malloc(n);

  if(!f->value) {
    xDestroyField(f);
    xError(funcName, X_NULL);
    return NULL;
  }

  memcpy(f->value, value, n);

  return f;
}


/**
 * Creates a generic scalar field of a given name and native value. The structure will hold a copy
 * of the value that is pointed at.
 *
 * \param name      Field name
 * \param type      Storage type, e.g. X_INT.
 * \param value     Pointer to the native data location in memory.
 *
 * \return          A newly created field with the supplied data, or NULL if there was an error.
 */
XField *xCreateScalarField(const char *name, XType type, const void *value) {
  return xCreateField(name, type, 0, NULL, value);
}

/**
 * Creates a field holding a single double-precision value value.
 *
 * \param name      Field name
 * \param value     Associated value
 *
 * \return          A newly created field with the supplied data, or NULL if there was an error.
 *
 */
XField *xCreateDoubleField(const char *name, double value) {
  return xCreateScalarField(name, X_DOUBLE, &value);
}

/**
 * Creates a field holding a single ineger value value.
 *
 * \param name      Field name
 * \param value     Associated value
 *
 * \return          A newly created field with the supplied data, or NULL if there was an error.
 *
 * @sa xCreateLongField()
 */
XField *xCreateIntField(const char *name, int value) {
  return xCreateScalarField(name, X_INT, &value);
}

/**
 * Creates a field holding a single ineger value value.
 *
 * \param name      Field name
 * \param value     Associated value
 *
 * \return          A newly created field with the supplied data, or NULL if there was an error.
 *
 * @sa xCreateIntField()
 */
XField *xCreateLongField(const char *name, long long value) {
  return xCreateScalarField(name, X_LONG, &value);
}


/**
 * Creates a field holding a single boolean value value.
 *
 * \param name      Field name
 * \param value     Associated value
 *
 * \return          A newly created field with the supplied data, or NULL if there was an error.
 */
XField *xCreateBooleanField(const char *name, boolean value) {
  return xCreateScalarField(name, X_BOOLEAN, &value);
}


/**
 * Creates a field holding a single string value.
 *
 * \param name      Field name
 * \param value     Associated value
 *
 * \return          A newly created field referencing the supplied string, or NULL if there was an error.
 */
XField *xCreateStringField(const char *name, const char *value) {
  return xCreateScalarField(name, X_STRING, &value);
}


/**
 * Checks if a given field has valid data.
 *
 * \param f         Pointer to the field to check.
 *
 * \return          TRUE is the field seems to contain valid data, otherwise FALSE.
 *
 */
boolean xIsFieldValid(const XField *f) {
  if(f->name == NULL) return FALSE;
  if(f->value == NULL) return FALSE;
  if(f->type == X_STRUCT) return TRUE;
  if(xElementSizeOf(f->type) <= 0) return FALSE;
  if(f->ndim <= 0) return FALSE;
  if(f->sizes[0] <= 0) return FALSE;
  return TRUE;
}

/**
 * Returns the total number of primitive elements in a field.
 *
 * @param f     The field
 * @return      The total number of primitive elements contained in the field.
 */
int xGetFieldCount(const XField *f) {
  if(!f) return 0;
  return xGetElementCount(f->ndim, f->sizes);
}

/**
 * Inserts a structure within a parent structure, returning the old field that may have existed under
 * the requested name before.
 *
 * \param s         Pointer to the parent structure
 * \param name      Name of the sub-structure
 * \param substruct Pointer to the sub-structure
 *
 * return           The prior field stored under the same name or NULL. If there is an error
 *                  then NULL is returned and errno is set to indicate the nature of the issue.
 *                  (a message is also printed to stderr if xDebug is enabled.)
 */
XField *xSetSubstruct(XStructure *s, const char *name, XStructure *substruct) {
  const char *funcName = "smaxSetSubstruct()";
  XField *f;

  if(!s) {
    xError(funcName, X_STRUCT_INVALID);
    return NULL;
  }

  if(!name) {
    xError(funcName, X_NAME_INVALID);
    return NULL;
  }

  if(!name[0]) {
    xError(funcName, X_NAME_INVALID);
    return NULL;
  }

  if(!substruct) {
    xError(funcName, X_NULL);
    return NULL;
  }

  if(substruct->parent) {
    xError(funcName, X_INCOMPLETE);
    return NULL;
  }

  f = xCreateScalarField(name, X_STRUCT, substruct);
  if(!f) return NULL;

  substruct->parent = s;

  return xSetField(s, f);
}


/**
 * Removes as field from the structure, returning it if found.
 *
 * \param s     Pointer to structure
 * \param name  Name of field to remove
 *
 * \return      Pointer to the removed field or else NULL if the was an error or if no
 *              matching field existed in the structure.
 *
 */
XField *xRemoveField(XStructure *s, const char *name) {
  const char *funcName = "smaxSetField()";

  XField *e, *last = NULL;

  if(!s) {
    xError(funcName, X_STRUCT_INVALID);
    return NULL;
  }

  if(!name) {
    xError(funcName, X_NAME_INVALID);
    return NULL;
  }

  if(!name[0]) {
    xError(funcName, X_NAME_INVALID);
    return NULL;
  }

  for(e = s->firstField; e != NULL; e=e->next) {
    if(!strcmp(name, e->name)) {
      if(last) last->next = e->next;
      else s->firstField = e->next;
      e->next = NULL;
      if(e->type == X_STRUCT) if(e->value) {
        XStructure *sf = (XStructure *) e->value;
        int k;
        for(k = xGetFieldCount(e); --k >= 0; ) sf[k].parent = NULL;
      }
      return e;
    }
    last = e;
  }

  return NULL;
}


/**
 * Adds or replaces a field in the structure with the specified field value, returning the previous value for the same field.
 * It is up to the caller whether or not the old value should be destoyed or kept. Note though that you should check first to
 * see if the replaced field is the same as the new one before attempting to destroy...
 *
 * A note of caution: There is no safeguard against adding the same field to more than one structure, which will result in
 * a corruption of the affected structures, since both structures would link to the field, but the field links to only
 * one specific successive element. Therefore, the user is responsible to ensure that fields are assigned to structures
 * uniquely, and if necessary remove the field from one structure before assigning it to another.
 *
 * \param s     Structure to which to add the field
 * \param f     Field to be added.
 *
 * \return      Previous field by the same name, or NULL if the field is new...
 */
XField *xSetField(XStructure *s, XField *f) {
  const char *funcName = "smaxSetField()";

  XField *e, *last = NULL;

  if(!s) {
    xError(funcName, X_STRUCT_INVALID);
    return NULL;
  }

  if(!f) {
    xError(funcName, X_NULL);
    return NULL;
  }

  f->next = NULL;

  for(e = s->firstField; e != NULL; e = e->next) {
    if(!strcmp(f->name, e->name)) {

      f->next = e->next;                    // Inherit the link to the successive field
      e->next = NULL;                       // Unlink the prior field by the same name.

      if(last == NULL) s->firstField = f;   // If it's right at the top, then replace the top.
      else last->next = f;                  // Otherwise, link to the prior field.

      return e;
    }

    last = e;
  }

  // Add the new field at the end of the list...
  if(last == NULL) s->firstField = f;
  else last->next = f;

  return NULL;
}

/**
 * Returns the number of fields contained inside the structure. It is not recursive.
 *
 * @param s     Pointer to the structure to investigate
 * @return      the number of fields cotnained in the structure (but not counting fields in sub-structures).
 */
int xCountFields(const XStructure *s) {
  XField *f;
  int n = 0;

  if(!s) return 0;

  for(f = s->firstField; f != NULL; f = f->next) n++;

  return n;
}

/**
 * Destroys an X structure, freeing up resources used by name and value.
 *
 * \param s     Pointer to the structure to be destroyed.
 *
 */
__inline__ void xDestroyStruct(XStructure *s) {
  if(s == NULL) return;
  xClearStruct(s);
  free(s);
}

/**
 * Destroys an X structure field, freeing up resources used.
 *
 * \param f     Pointer to the field to be destroyed.
 *
 */
void xDestroyField(XField *f) {
  if(!f) return;

  if(f->value) {
    if(f->type == X_STRUCT) {
      XStructure *s = (XStructure *) f->value;
      int i;
      for(i=xGetFieldCount(f); --i >= 0; ) xClearStruct(&s[i]);
    }
    else if(!f->isSerialized && (f->type == X_STRING || f->type == X_RAW)) {
      char **str = (char **) f->value;
      int i;
      for(i=xGetFieldCount(f); --i >= 0; ) if(str[i]) free(str[i]);
    }
    free(f->value);
  }

  if(f->name != NULL) free(f->name);

  free(f);
}


/**
 * Destroys the contents of an X structure, leaving the structure empty.
 *
 * @param s    Pointer to the structure to be cleared.
 *
 * @sa smaDestroyStruct()
 */
void xClearStruct(XStructure *s) {
  XField *f;

  if(s == NULL) return;

  for(f = s->firstField; f != NULL; ) {
    XField *next = f->next;
    xDestroyField(f);
    f = next;
  }

  s->firstField = NULL;
}

/**
 * Reduces the dimensions by eliminating axes that contain a singular elements. Thus a size of {1, 3, 1, 5} will reduce to
 * {3, 5} containing the same number of elements, in fewer dimensions. If any of the dimensions are zero then it
 * reduces to { 0 }.
 *
 *
 * @param[in, out] ndim     Pointer to the dimensions (will be updated in situ)
 * @param[in, out] sizes    Array of sizes along the dimensions (will be updated in situ)
 * @return                  X_SUCCESS (0) if successful or else X_SIZE_INVALID if the ndim argument is NULL, or if it is greater than zero but
 *                          the sizes argument is NULL (errno set to EINVAL in both cases)
 *
 * @see xReduceAllDims()
 */
int xReduceDims(int *ndim, int *sizes) {
  int i;

  if(!ndim) {
    errno = EINVAL;
    return X_SIZE_INVALID;
  }

  if(*ndim <= 0) return X_SUCCESS;

  if(sizes == NULL) {
    errno = EINVAL;
    return -1;
  }

  for(i = *ndim; --i >= 0; ) if (sizes[i] == 0) {
    *ndim = 1;
    sizes[0] = 0;
    return X_SUCCESS;
  }

  for(i = *ndim; --i >= 0; ) if (sizes[i] == 1) {
    (*ndim)--;
    if(i < *ndim) memmove(&sizes[i], &sizes[i+1], (*ndim - i - 1) * sizeof(int));
    else sizes[i] = 0;
  }

  return X_SUCCESS;
}

/**
 * Recursively eliminates unneccessary embedding of singular structures inside a structure as well as reduces the
 * dimension of all array fields with xReduceDims().
 *
 * @param s     Pointer to a structure.
 * @return      X_SUCCESS (0) if successful or else X_STRUCT_INVALID if the argument is NULL (errno is
 *              also set to EINVAL)
 *
 * @see xReduceDims()
 */
int xReduceAllDims(XStructure *s) {
  XField *f;

  if(!s) {
    errno = EINVAL;
    return X_STRUCT_INVALID;
  }

  f = s->firstField;
  if(f->next == NULL && f->type == X_STRUCT) {
    XStructure *sub = (XStructure *) f;
    XField *sf;

    s->firstField = sub->firstField;
    for(sf = s->firstField; sf; sf = sf->next) if(sf->type == X_STRUCT) {
      XStructure *ss = (XStructure *) f;
      int i;
      for(i = xGetFieldCount(f); --i >= 0; ) ss[i].parent = s;
    }

    free(f);
    return xReduceAllDims(s);
  }

  for(f = s->firstField; f; f = f->next) {
    xReduceDims(&f->ndim, f->sizes);

    if(f->type == X_STRUCT) {
      XStructure *ss = (XStructure *) f;
      int i;
      for(i = xGetFieldCount(f); --i >= 0; ) xReduceAllDims(&ss[i]);
    }
  }

  return 0;
}

