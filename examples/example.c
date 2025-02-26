/**
 *  Example file demonstrating the basics of creating structured data
 *  programatically, accessing values therein, and outputing to JSON
 *  or parsing from JSON representation.
 *
 *  Link with:
 *  ```
 *    -lxchange
 *  ```
 *
 * @date Created  on Apr 23, 2024
 * @author Attila Kovacs
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "xchange.h"
#include "xjson.h"

/**
 * Creates structured data programatically, including substructures,
 * homogeneous and heterogeneous arrays, and various scalar fields. The
 * returned structure, in JSON notation is:
 *
 * ```json
 * {
 *   "bool": true,
 *   "string": "Hello world!",
 *   "int": -10,
 *   "long": 12345678901,
 *   "double": 1.2345e-11,
 *   "array": [
 *     [ 1, 2, 3, 0, 0, 0, 0, 0, 0, 0 ],
 *     [ 4, 5, 6, 0, 0, 0, 0, 0, 0, 0 ],
 *     [ 7, 8, 9, 0, 0, 0, 0, 0, 0, 0 ]
 *   ],
 *   "empty-array": [[ ]],
 *   "hetero-array": [
 *     [ 1, 2 ],
 *     [ true ],
 *     [ "aa", "bb", "cc" ]
 *   ],
 *   "sub": {
 *     "int": 1154,
 *     "string": "Here go the \n\t special chars."
 *   },
 *   "empty": { }
 * }
 *
 * ```
 *
 * @return    The C representation of the above structure.
 */
static XStructure *create_my_struct() {
  XStructure *s = xCreateStruct();      // The top level structure
  XStructure *sub = xCreateStruct();    // A substructure
  XStructure *esub = xCreateStruct();   // an empty substructure (we won't populate).

  int array[3][10] = {{1, 2, 3}, {4, 5, 6}, {7, 8, 9}}; // 3x10 integer array
  int sizes[] = {3, 10};                // The size of the 3x10 2D array
  int empty[] = {0, 0};                 // The size of an empty 2D array

  int i1[] = {1, 2};                    // 1D array of integers
  char *s2[] = {"aa", "bb", "cc"};      // 1D array of strings
  boolean b3[] = {TRUE};                // boolean array

  // -------------------------------------------------------------------------
  // Heterogeneous array...
  // It is composed of different rows. First we define each row as a field.
  XField *f1 = xCreate1DField(".1", X_INT, 2, i1);
  XField *f2 = xCreate1DField(".3", X_BOOLEAN, 1, b3);
  XField *f3 = xCreate1DField(".2", X_STRING, 3, s2);

  // Create the heterogeneous array from the row contents.
  XField fa[] = { *f1, *f2, *f3 };

  // Discard the original row containers (we copied their content...)
  free(f1);
  free(f2);
  free(f3);
  // -------------------------------------------------------------------------

  // Set the fields of the top-level structure
  xSetField(s, xCreateBooleanField("bool", 1));
  xSetField(s, xCreateStringField("string", "Hello world!"));
  xSetField(s, xCreateIntField("int", -10));
  xSetField(s, xCreateLongField("long", 12345678901L));
  xSetField(s, xCreateDoubleField("double", 1.2345e-11));
  xSetField(s, xCreateField("array", X_INT, 2, sizes, array));
  xSetField(s, xCreateField("empty-array", X_INT, 2, empty, array));
  xSetField(s, xCreate1DField("hetero-array", X_FIELD, 3, fa));

  // Set the fields of the substructure
  xSetField(sub, xCreateIntField("int", 1154));
  xSetField(sub, xCreateStringField("string", "Here go the \n\t special chars."));
  // Add the substructure into the top-level structure
  xSetSubstruct(s, "sub", sub);

  // Add the empty substructure into the top-level structure.
  xSetSubstruct(s, "empty", esub);

  return s;
}


int main() {
  XStructure *s;      // Structured data in C
  XStructure *sub;    // A substructure;
  XStructure *parsed; // Parsed back data structure.
  XField *f;          // A field
  char *json;         // JSON string representation
  char *pos;          // String parse position

  int ival;           // an integer value
  double dval;        // a double-precision value

  long n;             // element count
  int *pi;            // pointer to an integer

  // Print runtime errors and traces to the console.
  xSetDebug(TRUE);

  // Print JSON parse errors to stderr.
  xjsonSetErrorStream(stderr);



  // -------------------------------------------------------------------------
  // Create structured data programatically (see above).
  s = create_my_struct();



  // -------------------------------------------------------------------------
  // Access a field

  // Access a field, using an aggregate ID...
  f = xGetField(s, "sub:int");        // The field named 'int' in the substructure named 'sub'

  // ... or via the substructure.
  sub = xGetSubstruct(s, "sub");      // Retrieve the substructure named 'sub'
  f = xGetField(sub, "int");          // Get the field named 'int' from the substructure

  // Get the value of the field as a scalar integer (or default to -1)...
  ival = (int) xGetAsLong(f, -1);

  // ... or as a floating point value
  dval = xGetAsDouble(f);

  printf("Value is %d (int), %f (double)\n", ival, dval);



  // -------------------------------------------------------------------------
  // Access arrays elements

  // Get an array field named "array"
  f = xGetField(s, "array");

  n = xGetFieldCount(f);
  printf("Array has %ld elements\n", n);

  // Get a pointer to the 3rd element (i.e. 0-based index 2)...
  pi = (int *) xGetElementAtIndex(f, 2);

  // ... or, get the value at index 2 as an integer (or default to -1)...
  ival = (int) xGetAsLongAtIndex(f, 2, -1);

  // ... or, as a double...
  dval = xGetAsDoubleAtIndex(f, 2);

  printf("3rd element is %d (pointer), %d (int), %f (double)\n", *pi, ival, dval);



  // -------------------------------------------------------------------------
  // Convert the structured data to JSON representation, and print...
  json = xjsonToString(s);
  printf("%s", json);



  // -------------------------------------------------------------------------
  // Parse the JSON representation back into a structure
  pos = json;                           // Start parsing at the head of the JSON string
  parsed = xjsonParseAt(&pos, NULL);    // Parse the JSON, and update the parse position
  printf(" Parsed %d bytes\n", (int) (pos - json));

  // Alternatively, you can parse JSON from a file, e.g.:
  //parsed = xjsonParsePath("/path/to/my.json", NULL);



  // -------------------------------------------------------------------------
  // Clean up allocated data we no longer need
  free(json);                           // Discard JSON string representation

  xDestroyStruct(parsed);               // Discard the parsed-back data structure
  xDestroyStruct(s);                    // Discard the original C structured data representation



  return 0;
}
