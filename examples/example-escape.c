/**
 *  Example file demosntrating the basics of escaping C strings JSON-style, or
 *  converting JSON string representation back to regular C strings.
 *
 *  Link with:
 *  ```
 *    -lxchange
 *  ```
 *
 * @date Created  on Feb 25, 2025
 * @author Attila Kovacs
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "xchange.h"
#include "xjson.h"

int main() {
  const char *cstring = "\\Hello \n\tWorld!";    // A C string contained special characters

  char *escaped, *escaped5;   // JSON escaped strings
  char *unescaped;            // Unescaped C string

  printf(" Original  : '%s'\n", cstring);

  // Obtaine an escaped representation of the C string (same as the string literal
  // that was used to define it in the source code).
  escaped = xjsonEscape(cstring, 0);
  printf(" Escaped   : '%s'\n", escaped);

  // Alternatively, convert an JSON-escaped string representation back to a normal C string
  unescaped = xjsonUnescape(escaped);
  printf(" Unescaped : '%s'\n", unescaped);

  // Discard created strings when no longer needed
  free(unescaped);
  free(escaped);

  // Obtained an escaped representation of the first 5 characters of the C-string
  escaped5 = xjsonEscape(cstring, 5);
  printf(" Escaped5  : '%s'\n", escaped5);

  // Discard created strings when no longer needed
  free(escaped5);

  return 0;
}
