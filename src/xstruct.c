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
#include <search.h>

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
 * \return      Matching field from the structure or NULL if there is no match or one of
 *              the arguments is NULL.
 *
 * \sa xLookupField()
 * \sa xSetField()
 * \sa xGetSubstruct()
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
 * Returns a substructure by the specified name, or NULL if no such sub-structure exists.
 *
 * \param s   Structure from which to retrieve a given sub-structure.
 * \param id  Name or aggregate ID of the substructure to retrieve
 * \return    Matching sub-structure from the structure or NULL if there is no match or one of
 *            the arguments is NULL.
 *
 * \sa xSetSubstruct()
 * \sa xGetField()
 */
XStructure *xGetSubstruct(const XStructure *s, const char *id) {
  const XField *f = xGetField(s, id);

  if(f && f->type == X_STRUCT)
    return (XStructure *) f->value;

  errno = ENOENT;
  return NULL;
}

/**
 * Creates a generic field of a given name and type and dimensions using a copy of the specified native array,
 * unless type is X_STRUCT in which case the value is referenced directly inside the field. For X_STRING
 * and X_RAW only the array references to the underlying string/byte buffers are copied into the field.
 *
 * \param name      Field name (it may not contain a separator X_SEP)
 * \param type      Storage type, e.g. X_INT.
 * \param ndim      Number of dimensionas (1:20). If ndim < 1, it will be reinterpreted as ndim=1, size[0]=1;
 * \param sizes     Array of sizes along each dimensions, with at least ndim elements, or NULL with ndim<1.
 * \param value     Pointer to the native data location in memory, or NULL to leave unassigned for now.
 *
 * \return          A newly created field with the copy of the supplied data, or NULL if there was an error.
 */
XField *xCreateField(const char *name, XType type, int ndim, const int *sizes, const void *value) {
  static const char *funcName = "xCreateField()";
  XField *f;
  int n;

  if(!name) {
    xError(funcName, X_NAME_INVALID);
    return NULL;
  }

  if(xLastSeparator(name)) {
    // name contains a separator
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

  if(type == X_STRUCT) {
    f->value = (char *) value;
  }
  else {
    f->value = malloc(n);

    if(!f->value) {
      xDestroyField(f);
      xError(funcName, X_NULL);
      return NULL;
    }

    memcpy(f->value, value, n);
  }

  return f;
}


/**
 * Creates a generic scalar field of a given name and native value. The structure will hold a copy
 * of the value that is pointed at.
 *
 * \param name      Field name (it may not contain a separator X_SEP)
 * \param type      Storage type, e.g. X_INT.
 * \param value     Pointer to the native data location in memory.
 *
 * \return          A newly created field with the supplied data, or NULL if there was an error.
 */
XField *xCreateScalarField(const char *name, XType type, const void *value) {
  return xCreateField(name, type, 0, NULL, value);
}

/**
 * Creates a generic field for a 1D array of a given name and native data. The structure will hold a
 * copy of the value that is pointed at.
 *
 * \param name      Field name (it may not contain a separator X_SEP)
 * \param type      Storage type, e.g. X_INT.
 * \param count     Number of elements in array
 * \param values    Pointer to an array of native values.
 *
 * \return          A newly created field with the supplied data, or NULL if there was an error.
 */
XField *xCreate1DField(const char *name, XType type, int count, const void *values) {
  const int sizes[] = { count };
  return xCreateField(name, type, 1, sizes, values);
}

/**
 * Creates a field holding a single double-precision value value.
 *
 * \param name      Field name (it may not contain a separator X_SEP)
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
 * \param name      Field name (it may not contain a separator X_SEP)
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
 * \param name      Field name (it may not contain a separator X_SEP)
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
 * \param name      Field name (it may not contain a separator X_SEP)
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
 * \param name      Field name (it may not contain a separator X_SEP)
 * \param value     Associated value. NULL values will be treated as empty strings.
 *
 * \return          A newly created field referencing the supplied string, or NULL if there was an error.
 */
XField *xCreateStringField(const char *name, const char *value) {
  const char *empty = "";
  return xCreateScalarField(name, X_STRING, value ? &value : &empty);
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
  if(xLastSeparator(f->name)) return FALSE;
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
 * The name may not contain a compound ID. To add the structure to embedded sub-structures, you may want to
 * use xGetSubstruct() first to add the new structure directly to the relevant embedded component.
 *
 * \param s         Pointer to the parent structure
 * \param name      Name of the sub-structure
 * \param substruct Pointer to the sub-structure. It is added directly as a reference, without making a copy.
 *
 * return           The prior field stored under the same name or NULL. If there is an error
 *                  then NULL is returned and errno is set to indicate the nature of the issue.
 *                  (a message is also printed to stderr if xDebug is enabled.)
 *
 * \sa xGetSubstruct()
 */
XField *xSetSubstruct(XStructure *s, const char *name, XStructure *substruct) {
  static const char *funcName = "xSetSubstruct()";
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
  static const char *funcName = "xRemoveField()";

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

  for(e = s->firstField; e != NULL; e = e->next) {
    if(!strcmp(name, e->name)) {
      if(last) last->next = e->next;
      else s->firstField = e->next;
      e->next = NULL;
      if(e->type == X_STRUCT) if(e->value) {
        XStructure *sub = (XStructure *) e->value;
        int k = xGetFieldCount(e);
        while(--k >= 0) sub[k].parent = NULL;
      }
      return e;
    }
    last = e;
  }

  return NULL;
}

/**
 * (<i>expert</i>) Inserts a field into the structure at its head position. That is, the specified field will become
 * the first field in the structure. And, unlike xSetField(), this function does not check for (nor remove) previously
 * present fields by the same name. Thus, it is left up to the caller to ensure that there are no duplicate field
 * names added to the structure.
 *
 * A note of caution: There is no safeguard against adding the same field to more than one structure, which will
 * result in a corruption of the affected structures, since both structures would link to the field, but the field
 * links to only one specific successive element. Therefore, the user is responsible to ensure that fields are
 * assigned to structures uniquely, and if necessary remove the field from one structure before assigning it to
 * another.
 *
 * \param s     Structure to which to add the field
 * \param f     Field to be added.
 *
 *
 * \sa xSetField()
 * \sa xReverseFieldOrder()
 */
int xInsertField(XStructure *s, XField *f) {
  static const char *funcName = "xInsertField()";

  if(!s) return xError(funcName, X_STRUCT_INVALID);
  if(!f) return xError(funcName, X_NULL);
  if(!f->name) return xError(funcName, X_NAME_INVALID);

  // The field name contains a separator?
  if(xLastSeparator(f->name)) return xError(funcName, X_NAME_INVALID);

  // Add the new field at the head of the list...
  f->next = s->firstField;
  s->firstField = f;

  return X_SUCCESS;
}

/**
 * Adds or replaces a field in the structure with the specified field value, returning the previous value for the same field.
 * It is up to the caller whether or not the old value should be destoyed or kept. Note though that you should check first to
 * see if the replaced field is the same as the new one before attempting to destroy...
 *
 * The field's name may not contain a compound ID. To add fields to embedded sub-structures, you may want to use
 * xGetSubstruct() first to add the field directly to the relevant embedded component.
 *
 * A note of caution: There is no safeguard against adding the same field to more than one structure, which will result in
 * a corruption of the affected structures, since both structures would link to the field, but the field links to only
 * one specific successive element. Therefore, the user is responsible to ensure that fields are assigned to structures
 * uniquely, and if necessary remove the field from one structure before assigning it to another.
 *
 * \param s     Structure to which to add the field
 * \param f     Field to be added.
 *
 * \return      Previous field by the same name, or NULL if the field is new or if there was an error
 *              (errno will be set to EINVAL)
 *
 * \sa xInsertField()
 * \sa xSetSubstruct()
 * \sa xGetSubstruct()
 */
XField *xSetField(XStructure *s, XField *f) {
  static const char *funcName = "xSetField()";

  XField *e, *last = NULL;

  if(!s) {
    xError(funcName, X_STRUCT_INVALID);
    errno = EINVAL;
    return NULL;
  }

  if(!f) {
    xError(funcName, X_NULL);
    errno = EINVAL;
    return NULL;
  }

  if(!f->name) {
    xError(funcName, X_NAME_INVALID);
    errno = EINVAL;
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
 *
 * @sa xDeepCountFields()
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
      XStructure *sub = (XStructure *) f->value;
      int i = xGetFieldCount(f);
      while(--i >= 0) xClearStruct(&sub[i]);
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
 * Reduces the dimensions by eliminating axes that contain a singular elements. Thus a size of {1, 3, 1, 5}
 * will reduce to {3, 5} containing the same number of elements, in fewer dimensions. If any of the dimensions
 * are zero then it reduces to { 0 }.
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
      XStructure *sub = (XStructure *) f;
      int i = xGetFieldCount(f);
      while(--i >= 0) sub[i].parent = s;
    }

    free(f);
    return xReduceAllDims(s);
  }

  for(f = s->firstField; f; f = f->next) {
    xReduceDims(&f->ndim, f->sizes);

    if(f->type == X_STRUCT) {
      XStructure *sub = (XStructure *) f;
      int i = xGetFieldCount(f);
      while(--i >= 0) xReduceAllDims(&sub[i]);
    }
  }

  return 0;
}



/**
 * Returns a pointer to the beginning of the next component in a compound ID. Leading ID separators are ignored.
 *
 * @param id    Aggregate X ID.
 * @return      Pointer to the start of the next compound ID token, or NULL if there is no more components in the ID.
 */
char *xNextIDToken(const char *id) {
  char *next;

  if(!id) return NULL;

  // Ignore leading separator.
  if(!strncmp(id, X_SEP, X_SEP_LENGTH)) id += X_SEP_LENGTH;

  next = strstr(id, X_SEP);
  return next ? next + X_SEP_LENGTH : NULL;
}

/**
 * Returns a copy of the next next component in a compound ID. Leading ID separators are ignored.
 *
 * @param id    Aggregate X ID.
 * @return      Pointer to the start of the next compound ID token, or NULL if there is no more components in the ID.
 */
char *xCopyIDToken(const char *id) {
  const char *next;
  char *token;
  int l;

  if(!id) return NULL;

  // Ignore leading separator.
  if(!strncmp(id, X_SEP, X_SEP_LENGTH)) id += X_SEP_LENGTH;

  next = xNextIDToken(id);
  if(next) l = next - id - X_SEP_LENGTH;
  else l = strlen(id);

  token = malloc(l+1);
  if(!token) return NULL;

  memcpy(token, id, l);
  token[l] = '\0';

  return token;
}

/**
 * Checks if the next component in a compound id matches a given token.
 *
 * @param token     Full token to check for
 * @param id        Compount X ID.
 * @return          X_SUCCESS if it's a match. Otherwise X_FAILURE or another X error if the
 *                  arguments are invalid.
 */
int xMatchNextID(const char *token, const char *id) {
  int L;

  if(!token) return X_NULL;
  if(!id) return X_NULL;
  if(*token == '\0') return X_NAME_INVALID;
  if(*id == '\0') return X_GROUP_INVALID;


  L = strlen(token);
  if(strncmp(id, token, L)) return X_FAILURE;

  if(id[L] == '\0') return X_SUCCESS;

  if(strlen(id) < L + X_SEP_LENGTH) return X_FAILURE;
  if(strncmp(id + L, X_SEP, X_SEP_LENGTH)) return X_FAILURE;

  return X_SUCCESS;
}


/**
 * Returns the aggregated (hierarchical) &lt;table&gt;:&lt;key&gt; ID. The caller is responsible for calling free()
 * on the returned string after use.
 *
 * \param table     SMA-X hastable name
 * \param key       The lower-level id to concatenate.
 *
 * \return          The aggregated ID, or NULL if both arguments were NULL themselves.
 *
 * \sa xSplitID()
 */
char *xGetAggregateID(const char *table, const char *key) {
  char *id;

  if(table == NULL && key == NULL) return NULL;

  if(table == NULL) return xStringCopyOf(key);
  if(key == NULL) return xStringCopyOf(table);

  id = (char *) malloc(strlen(table) + strlen(key) + X_SEP_LENGTH + 1); // <group>:<key>
  if(!id) return NULL;
  sprintf(id, "%s" X_SEP "%s", table, key);

  return id;
}

/**
 * Returns the string pointer to the begining of the last separator in the ID.
 *
 * @param id    Compound SMA-X ID.
 * @return      Pointer to the beginning of the last separator in the ID, or NULL if the ID does not contain a separator.
 *
 * @sa xSplitID()
 */
char *xLastSeparator(const char *id) {
  int l = strlen(id);

  while(--l >= 0) if(id[l] == X_SEP[0]) if(!strncmp(&id[l], X_SEP, X_SEP_LENGTH)) return (char *) &id[l];
  return NULL;
}

/**
 * Splits the id into two strings (sharing the same input buffer): (1) the id of the embedding structure, and
 * (2) the embedded field name. The original input id is string terminated after the table name. And the pointer to the key
 * part that follows after the last separator is returned in the second (optional argument).
 *
 * \param[in,out] id        String containing an aggregate ID, which will be terminated after the last substructure.
 * \param[out] pKey         Returned pointer to the second component after the separator within the same buffer. This is
 *                          not an independent pointer. Use xStringCopyOf() if you need an idependent string
 *                          on which free() can be called! The returned value pointed to may be NULL if the ID
 *                          could not be split. The argument may also be null, in which case the input string is
 *                          just terminated at the stem, without returning the second part.
 *
 * \return      X_SUCCESS (0)       if the ID was successfully split into two components.
 *              X_NULL              if the id argument is NULL.
 *              X_NAME_INVALID      if no separator was found
 *
 * \sa xGetAggregateID()
 * \sa xLastSeparator()
 */
int xSplitID(char *id, char **pKey) {
  if(id == NULL) return X_NULL;

  // Default NULL return for the second component.
  if(pKey) *pKey = NULL;

  id = xLastSeparator(id);
  if(id) *id = '\0';
  else return X_NAME_INVALID;

  if(pKey) *pKey = id + X_SEP_LENGTH;

  return X_SUCCESS;
}

/**
 * Counts the number of fields in a structure, including the field count for all embedded
 * substructures also recursively.
 *
 * @param s           Pointer to a structure
 * @return            The total number of fields present in the structure and all its sub-structures.
 *
 * @sa xCountFields()
 */
long xDeepCountFields(const XStructure *s) {
  XField *f;
  long n = 0;

  if(!s) return 0;

  for(f = s->firstField; f != NULL; f = f->next) {
    n++;

    if(f->type == X_STRUCT) {
      XStructure *sub = (XStructure *) f->value;
      int i = xGetFieldCount(f);
      while(--i >= 0) {
        long m = xDeepCountFields(&sub[i]);
        if(m < 0) return -1;
        n += m;
      }
    }
  }

  return n;
}

/**
 * Sort the fields in a structure using a specific comparator function.
 *
 * @param s           The structure, whose fields to sort
 * @param cmp         The comparator function. It takes two pointers to XField locations as arguments.
 * @param recursive   Whether to apply the sorting to all ebmbedded substructures also
 * @return            X_SUCCESS (0) if successful, or else X_NULL if the structure or the comparator
 *                    function is NULL.
 *
 * @sa xSortFieldsByName()
 * @sa xReverseFieldOrder()
 */
int xSortFields(XStructure *s, int (*cmp)(const XField **f1, const XField **f2), boolean recursive) {
  XField **array, *f;
  int i, n;

  if(s == NULL || cmp == NULL) return xError("xSortFields()", X_NULL);

  for(n = 0, f = s->firstField; f != NULL; f = f->next, n++)
    if(f->type == X_STRUCT && recursive) {
      XStructure *sub = (XStructure *) f->value;
      int i = xGetFieldCount(f);
      while (--i >= 0) xSortFields(&sub[i], cmp, TRUE);
    }

  if(n < 2) return n;

  array = (XField **) malloc(n * sizeof(XField *));
  for(n = 0, f = s->firstField; f != NULL;) {
    XField *next = f->next;
    f->next = NULL;
    array[n++] = f;
    f = next;
  }
  s->firstField = NULL;

  qsort(array, n, sizeof(XField *), (int (*)(const void *, const void *)) cmp);

  s->firstField = array[0];
  for(i = 1, f = s->firstField; i < n; i++, f = f->next) f->next = array[i];

  return X_SUCCESS;
}


static int XFieldNameCmp(const XField **f1, const XField **f2) {
  return strcmp((*f1)->name, (*f2)->name);
}

/**
 * Sorts the fields of a structure by field name, in ascending alphabetical order.
 *
 * @param s           The structure, whose fields to sort
 * @param recursive   Whether to apply the sorting to all ebmbedded substructures also
 * @return            X_SUCCESS (0) if successful, or else X_NULL if the structure is NULL.
 *
 * @sa xReverseFieldOrder()
 */
int xSortFieldsByName(XStructure *s, boolean recursive) {
  return xSortFields(s, XFieldNameCmp, recursive);
}

/**
 * Reverse the order of fields in a structure.
 *
 * @param s           The structure, whose field order to reverse.
 * @param recursive   Whether to apply the reversal to all ebmbedded substructures also
 * @return            X_SUCCESS (0) if successful, or else X_NULL if the structure is NULL.
 *
 * @sa xSortFields()
 * @sa xSortFieldsByName()
 * @sa xInsertField()
 */
int xReverseFieldOrder(XStructure *s, boolean recursive) {
  XField *f, *rev = NULL;

  if(s == NULL) return xError("xReverseFieldOrder()", X_NULL);

  f = s->firstField;
  s->firstField = NULL;

  while(f != NULL) {
    XField *next = f->next;
    f->next = rev;
    rev = f;

    if(f->type == X_STRUCT && recursive) {
      XStructure *sub = (XStructure *) f->value;
      int i = xGetFieldCount(f);
      while(--i >= 0) xReverseFieldOrder(&sub[i], TRUE);
    }

    f = next;
  }

  s->firstField = rev;
  return X_SUCCESS;
}
