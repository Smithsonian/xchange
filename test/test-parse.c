/**
 * @file
 *
 * @date Created  on Jul 17, 2025
 * @author Attila Kovacs
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

#include "xchange.h"

static int parse_float() {
  int i;

  for(i = 0; i < 100; i++) {
    char str[40] = {'\0'}, str2[40] = {'\0'};
    float f, exp;

    sprintf(str, "3.%02d", i);
    exp = (float) (3.0 + 0.01 * i);

    f = xParseFloat(str, NULL);
    if(f != exp) {
      printf("ERROR! parse float: %f != %s\n", f, str);
      return -1;
    }

    xPrintFloat(str2, f);
    if(strncmp(str2, str, strlen(str2))) {
      printf("ERROR! print float: %s != %s\n", str2, str);
      return -1;
    }
  }

  return 0;
}

static int parse_double() {
  int i;

  for(i = 0; i < 100; i++) {
    char str[40] = {'\0'}, str2[40] = {'\0'};
    double d, exp;

    sprintf(str, "3.%02d", i);
    exp = 3.0 + 0.01 * i;

    d = xParseDouble(str, NULL);
    if(fabs(d - exp) > 1e-15) {
      printf("ERROR! parse double: %f != %s\n", d, str);
      return -1;
    }

    xPrintDouble(str2, d);
    if(strncmp(str2, str, strlen(str2))) {
      printf("ERROR! print double: %s != %s (%f)\n", str2, str, d);
      return -1;
    }
  }

  return 0;
}


int main() {
    if(parse_float()) return 1;
    if(parse_double()) return 2;

    printf("OK.\n");
    return 0;
}
