![Build Status](https://github.com/Smithsonian/xchange/actions/workflows/build.yml/badge.svg)
![Tests](https://github.com/Smithsonian/xchange/actions/workflows/test.yml/badge.svg)
![Static Analysis](https://github.com/Smithsonian/xchange/actions/workflows/analyze.yml/badge.svg)
<a href="https://smithsonian.github.io/xchange/apidoc/html/files.html">
 ![API documentation](https://github.com/Smithsonian/xchange/actions/workflows/dox.yml/badge.svg)
</a>
<a href="https://smithsonian.github.io/xchange/index.html">
 ![Project page](https://github.com/Smithsonian/xchange/actions/workflows/pages/pages-build-deployment/badge.svg)
</a>

<picture>
  <source srcset="resources/CfA-logo-dark.png" alt="CfA logo" media="(prefers-color-scheme: dark)"/>
  <source srcset="resources/CfA-logo.png" alt="CfA logo" media="(prefers-color-scheme: light)"/>
  <img src="resources/CfA-logo.png" alt="CfA logo" width="400" height="67" align="right"/>
</picture>
<br clear="all">


# xchange 

[![DOI](resources/796202092.svg)](https://doi.org/10.5281/zenodo.14634824)

Structured data representation and JSON support for C/C++.

 - [API documentation](https://smithsonian.github.io/xchange/apidoc/html/files.html)
 - [Project page](https://smithsonian.github.io/xchange) on github.io

Author: Attila Kovacs

Updated for 1.0 and later releases.

## Table of Contents

 - [Introduction](#xchange-introduction)
 - [Building](#building-xchange)
 - [Linking your application against `xchange`](#xchange-linking)
 - [Structured data](#structured-data)
 - [JSON parser and emitter](#json-interchange)
 - [Error handling](#xchange-error-handling)
 - [Debugging support](#xchange-debugging-support)
 - [Future plans](#xchange-future-plans) 


-----------------------------------------------------------------------------

<a name="xchange-introduction"></a>
## Introduction

The __xchange__ library provides structured data representation and exchange in C/C++, and includes support for 
JSON parsing and generation. It is free to use, in any way you like, without licensing restrictions.

For JSON parsing end emitting, __xchange__ provides a higher-level data model than __cjson__, with high-level 
functions for accessing and manipulating data both with less code and with cleaner code.

The __xchange__ library was created, and is maintained, by Attila Kovács at the Center for Astrophysics \| Harvard 
&amp; Smithsonian, and it is available through the [Smithsonian/xchange](https://github.com/Smithsonian/xchange) 
repository on GitHub. 

-----------------------------------------------------------------------------

<a name="building-xchange"></a>
## Building

The __xchange__ library can be built either as a shared (`libxchange.so[.1]`) and as a static (`libxchange.a`) library, 
depending on what suits your needs best.

You can configure the build, either by editing `config.mk` or else by defining the relevant environment variables 
prior to invoking `make`. The following build variables can be configured:

 - `CC`: The C compiler to use (default: `gcc`).

 - `CPPFLAGS`: C preprocessor flags, such as externally defined compiler constants.
 
 - `CFLAGS`: Flags to pass onto the C compiler (default: `-g -Os -Wall`). Note, `-Iinclude` will be added 
   automatically.
 
 - `CSTANDARD`: Optionally, specify the C standard to compile for, e.g. `c99` to compile for the C99 standard. If
   defined then `-std=$(CSTANDARD)` is added to `CFLAGS` automatically.
   
 - `WEXTRA`: If set to 1, `-Wextra` is added to `CFLAGS` automatically.

 - `FORTIFY`: If set it will set the `_FORTIFY_SOURCE` macro to the specified value (`gcc` supports values 1 
   through 3). It affords varying levels of extra compile time / runtime checks.
   
 - `LDFLAGS`: Extra linker flags (default: _not set_). Note, `-lm -lpthread` will be added automatically.

 - `CHECKEXTRA`: Extra options to pass to `cppcheck` for the `make check` target

 - `DOXYGEN`: Specify the `doxygen` executable to use for generating documentation. If not set (default), `make` will
   use `doxygen` in your `PATH` (if any). You can also set it to `none` to disable document generation and the
   checking for a usable `doxygen` version entirely.
 
 
After configuring, you can simply run `make`, which will build the `shared` (`lib/libxchange.so[.1]`) and `static` 
(`lib/libxchange.a`) libraries, local HTML documentation (provided `doxygen` is available), and performs static
analysis via the `check` target. Or, you may build just the components you are interested in, by specifying the
desired `make` target(s). (You can use `make help` to get a summary of the available `make` targets). 

After building the library you can install the above components to the desired locations on your system. For a 
system-wide install you may simply run:

```bash
  $ sudo make install
```

Or, to install in some other locations, you may set a prefix and/or `DESTDIR`. For example, to install under `/opt` 
instead, you can:

```bash
  $ sudo make prefix="/opt" install
```

Or, to stage the installation (to `/usr`) under a 'build root':

```bash
  $ make DESTDIR="/tmp/stage" install
```



-----------------------------------------------------------------------------

<a name="xchange-linking"></a>
## Linking your application against `xchange`

Provided you have installed the shared (`libxchange.so`) or static (`libxchange.a`) library in a location that is
in your `LD_LIBRARY_PATH` (e.g. in `/usr/lib` or `/usr/local/lib`) you can simply link your program using the 
`-lxchange` flag. Your `Makefile` may look like: 

```make
myprog: ...
	$(CC) -o $@ $^ $(LDFLAGS) -lxchange 
```

(Or, you might simply add `-lxchange` to `LDFLAGS` and use a more standard recipe.) And, in if you installed the 
__xchange__ library elsewhere, you can simply add the location to `LD_LIBRARY_PATH` prior to linking.


-----------------------------------------------------------------------------

<a name="structured-data"></a>
## Structured data

 - [Basic data types](#xchange-data-types)
 - [Scalars](#xchange-scalars)
 - [Arrays](#xchange-arrays)
 - [Creating structure](#xchange-creating-structure)
 - [Aggregate IDs](#xchange-aggregate-ids)
 - [Accessing substructures and elements](#accessing-data)
 - [Sorting fields](#sorting-fields)


The __xchange__ library defines the `XStructure` type to represent structured data. It is defined in `xchange.h`, but 
as a user you really do not need to know much about its layout, as you probably want to avoid low-level direct access 
to its elements. Rather, you should be using the functions of the __xchange__ API to create, modify, or access data 
within.

Under the hood, the `XStructure` contains a linked list of fields, each an `XField` data type to represent a single 
element, or an array of elements, of the above mentioned types, including embedded `Xstructure`s. In this way, an 
`Xstructure` can easily represent a multi-level hierarchy of a composite data object. Each `XField` has a name/ID, an 
associated data type, a dimensionality, a shape (for multidimensional arrays).

<a name="xchange-data-types"></a>
### Basic data types

The __xchange__ library supports most basic (primitive) data types used across programming languages. The table below 
shows the unique __xchange__ types recognized by the library and the corresponding pointer/array type values:

 | `XType`       | element type             | Comment / example                                               |
 |:--------------|:------------------------:|:----------------------------------------------------------------|
 | `X_BOOLEAN`   | `boolean`                | `TRUE` (1 or non-zero) or `FALSE` (0)                           |
 | `X_BYTE`      | `char`                   | '`-128`' to  '`127`'                                            |
 | `X_INT16`     | `int16_t`                | '`-32768`' to '`32767`'                                         |
 | `X_INT32`     | `int32_t`                | '`-2,147,483,648`' to '`2,147,483,647`'                         |
 | `X_INT64`     | `int64_t`                | '`-9,223,372,036,854,775,808`' to '`9,223,372,036,854,775,807`' |
 | `X_FLOAT`     | `float`                  | `1`, `1.0`, `-1.234567e-28`                                     |
 | `X_DOUBLE`    | `double`                 | `1`, `1.0`, `-1.2345678901234567e-111`                          |
 | `X_STRING`    | `char *`    	            | `Hello world!`, `line1\nline2\n` (0-terminated)                 |
 | `X_CHARS(n) ` | `char[n]`                | Fixed-length character arrays (also w/o termination)            |
 | `X_FIELD`     | `XField`                 | For irregular and/or heterogeneous arrays                       |
 | `X_STRUCT`    | `XStructure`             | substructure                                                    |

The `boolean` type is defined in `xchange.h`. The `XField.value` is a pointer / array of the given element type. So,
an `XField` of type `X_DOUBLE` will have a `value` field that should be cast a `(double *)`, while for type `X_STRING`
the value field shall be cast as `(char **)`.

Additionally, the __xchange__ also defines derivative `XType` values for native integer storage types, whose widths 
depend on the particular CPU architecture. Hence, these are aliased to matching unique types (above) by the C 
preprocessor during compilation:

 | `XType`       | element type             | width            | alias of                                   |
 |:--------------|:------------------------:|:----------------:|:-------------------------------------------|
 | `X_SHORT`     | `short`                  |   &gt;= 16-bits  | typically `X_INT16`                        |
 | `X_INT`       | `int`                    |   &gt;= 16-bits  | often `X_INT32`                            |
 | `X_LONG`      | `long`                   |   &gt;= 32-bits  | typically `X_INT32` or `X_INT64`           |
 | `X_LLONG`     | `long long`              |   &gt;= 64-bits  | typically `X_INT64`                        |


<a name="xchange-strings"></a>
#### Strings

Strings can be either fixed-width or else a 0-terminated sequence of ASCII characters. At its basic level the library 
does not impose any restriction of what ASCII characters may be used. However, we recommend that users stick to the 
JSON convention, and represent special characters in escaped form. E.g. carriage return (`0xd`) as `\` followed by 
`n`, tabs as `\` followed by `t`, etc. As a result a single backslash should also be escaped as two consecutive `\` 
characters. You might use `xjsonEscapeString()` or `xjsonUnescapeString()` to perform the conversion to/from standard
JSON representation.

Fixed-width strings of up to _n_ characters are represented internally as the `XCHAR(n)` type. They may be 
0-terminated as appropriate, or else represent exactly _n_ ASCII characters without explicit termination. 
Alternatively, the `X_STRING` type represents ASCII strings of arbitrary length, up to the 0-termination character.

<a name="xchange-scalars"></a>
### Scalars

You can create scalar fields easily, e.g.:

```c
  // Create "is_ok" as a boolean field with TRUE
  XField *fb = xCreateBooleanField("is_ok", TRUE);

  // Create "serial-number" field with an integer value
  XField *fi = xCreateIntField("serial-number", 1001);

  // Create "my measurement" as a double-precision value 1.04
  XField *fd = xCreateDoubleField("my-measurement", 1.04);
  
  // Create "description" as a string
  XField *fs = xCreateStringField("description", "blah-blah-blah");
```

Under the hood, scalar values are a special case of arrays containing a single element. Scalars have dimension zero 
i.e., a shape defined by an empty integer array, e.g. `int shape[0]` in a corresponding `XField` element. 

In this way scalars are distinguished from true arrays containing just a single elements, which have dimensionality 
&lt;=1 and shapes e.g., `int shape[1] = {1}` or `int shape[2] = {1, 1}`. The difference, while subtle, becomes more 
obvious when serializing the array, e.g. to JSON. A scalar floating point value of 1.04, for example, will appear as 
`1.04` in JSON, whereas the 1D and 2D single-element arrays will be serialized as `{ 1.04 }` or `{{ 1.04 }}`, 
respectively.


<a name="xchange-arrays"></a>
### Arrays

The __xchange__ library supports array data types in one or more dimensions (up to 20 dimensions). For example, to
create a field for 2&times;3&times;4 array of `double`s, you may have something along:

```c
  double data[2][3][4] = ...;           // The native array in C
  int sizes[] = { 2, 3, 4 };            // An array containing the dimensions for xchange
  
  // Create a field for the 3-dimensional array with the specified shape.
  XField *f = xCreateField("my-array", X_DOUBLE, 3, sizes, data);
```

Note, that there is no requirement that the native array has the same dimensionality as it's nominal format in the
field. We could have declared `data` as a 1D array `double data[2 * 3 * 4] = ...`, or really any array (pointer)
containing doubles with storage for at least 24 elements. It is the `sizes` array, along with the dimensionality,
which together define the number of elements used from it, and the shape of the array for __xchange__.

Arrays of irregular shape or mixed element types can be represented by fields containing an array of `XField`
entries:

```c
  XField *row1, *row2, ...                 // Heterogeneous entries, each wrapped in an `XField`
  XField data[N] = { *row1, *row2, ... };  // The irregular / mixed-type array. 

  XField *f = xCreateMixed1DField("my_array", N);
```

Or, use `xCreateMixedArrayField()` to create a multi-dimensional array of heterogeneous elements the same way.


<a name="xchange-creating-structure"></a>
### Creating structure

Structures should always be created by calling `xCreateStruct()` (or else by an appropriate de-serialization 
function such as `xjsonParseAt()`, or as a copy via `xCopyStruct()`). Once the structure is no longer used it should be 
explicitly destroyed (freed) by calling `xDestroyStruct()`. Named substructures can be added to any structure with 
`xSetSubstruct()`, and named fields via `xSetField()`. That is the gist of it. So for example, the skeleton structure 
from the example above can be created programatically as:


```c
  XStructure *s, *sys, *sub;
  
  // Create the top-level structure
  s = xCreateStruct();
  
  // Create and add the "system" sub-structure
  sys = xCreateStruct();
  xSetSubstruct(s, "system", sys);
  
  // Create and add the "subsystem" sub-structure
  sub = xCreateStruct();
  xSetSubstruct(sys, "subsystem", sub);
  
  // Set the "property" field in "subsystem".
  xSetField(sub, "property", xCreateStringField("some value here"));
```

and then eventually destroyed after use as:

```c
  // Free up all resources used by the structure 's'
  xDestroyStruct(s);
```

<a name="xchange-aggregate-ids"></a>
### Aggregate IDs

Since the `XStructure` data type can represent hierarchies of arbitrary depth, and named at every level of the 
hierarchy, we can uniquely identify any particular field, at any level, with an aggregate ID, which concatenates the 
field names each every level, top-down, with a separator. The convention of __xchange__ is to use colon (':') as the
separator. Consider an example structure (in JSON notation):

```json
  {
    "system": {
      "subsystem": {
        "property": "some value here"
      }
    }
  }
```

Then, the leaf "property" entry can be 'addressed' with the aggregate ID of `system:subsystem:property` from the top
level. The `xGetAggregateID()` function is provided to construct such aggregate IDs by gluing together a leading and 
trailing component.


<a name="accessing-data"></a>
### Accessing substructures and elements

Once a structure is populated -- either by having constructed it programatically, or e.g. by parsing a JSON definition
of it from a string or file -- you can access its content and/or modify it.

E.g., to retrieve the "property" field from the above example structure:

```c
  XField *f = xGetField(s, "system:subsystem:property"); 
```

or to retrieve the "subsystem" structure from within:

```c
  XStructure *sub = xGetSubstruct(s, "system:subsystem");
```

Conversely you can set / update fields in a structure using `xSetField()` / `xSetSubstruct()`, e.g.:

```c
  XStructrure *newsub = ...     // The new substructure
  XField *newfield = ...        // A new field to set
  XField *oldfield, *oldsub;    // prior entries by the same field name/location (if any)
  
  oldfield = xSetField(s, newfield);        // Sets the a field in 's'
  oldsub = xSetSubstruct(s, "field", sub);  // Set a substructure named "bar" in 's'
```

The above calls return the old values (if any) for the "foo" and "bar" field in the structure, e.g. so we may dispose 
of them if appropriate:

```c
  // Destroy the replaced fields if they are no longer needed.
  xDestroyField(oldfield);
  xDestroyField(oldsub);
```

You can also remove existing fields from structures using `xRemoveField()`, e.g.

```c
  // Remove and then destroy the field named "blah" in structure 's'.
  xDestroyField(xRemoveField(s, "blah"));
```

#### Large structures

The normal `xGetField()` and `xGetSubstruct()` functions have computational costs that scale linearly with the number 
of direct fields in the structure. It is not much of an issue for structures that contain dozens of, or even a couple 
hundred, fields (per layer). For much larger structures, which have a fixed layout, there is an option for a 
potentially much more efficient hash-based lookup also. E.g. instead of `xGetField()` you may use `xLookupField()`:

```c
  XStructure *s = ...
  
  // Create a lookup table for all fields of 's' and all its substructures.
  XLookupTable *l = xCreateLookupTable(s, TRUE);
  
  // Now use a hash-based lookup to locate the field by name
  XField *f = xLookupField(l, "subsystem:property");
 
  ...
  
  // Once done with the lookup, destroy it.
  xDestroyLookup(l);
```

Note however, that preparing the lookup table has significant _O(N)_ computational cost also. Therefore, a lookup 
table is practical only if you are going to use it repeatedly, many times over. As a rule of thumb, lookups may have 
the advantage if accessing fields in a structure by name hundreds of times, or more.

The same performance limitation also applies to building large structures, since the `xSetField()` and 
`xSetSubstruct()` functions iterate over the existing fields to check if a prior field by the same name was already 
present, and which should be removed before the new field is set (hence the time to build up a structure with _N_
fields will scale as _O(N<sup>2</sup>)_ in general). The user may consider using `xInsertField()` instead, which is 
much more scalable for building large structures, since it does not check for duplicates (hence scales as _O(N)_ 
overall). However, `xInsertField()` also makes the ordering of fields less intuitive, and it is left up to the caller 
to ensure that field names added this way are never duplicated. (Tip: if you used `InsertField()` consistently, you 
may call `xReverseFieldOrder()` at the end, so the fields will appear in the same order in which they were inserted.)


#### Iterating over elements

You can easily iterate over the elements also. This is one application where you may want to know the internal layout
of `XStructure`, namely that it contains a simple linked-list of `XField` fields. One way to iterate over a structures
elements is with a `for` loop, e.g.:

```c
  XStructure *s = ...
  XField *f;
  
  for (f = s->firstField; f != NULL; f = f->next) {
    // Process each field 'f' here...
    ...
  }
```

<a name="sorting-fields"></a>
### Sorting fields

You can easily sort fields by name using `xSortFieldsByName()`, or with using a custom comparator function with 
`xSortFields()`. You can also reverse the order with `xReverseFieldOrder()`. For example to sort fields in a 
structure (and its substructures) in descending alphabetical order:

```c
  XStructure *s = ...
  
  // Sort in by names in ascending order, recursively
  xSortFieldsByName(s, TRUE);

  // Reverse the order, recursively
  xReverseFieldOrder(s, TRUE);
```

-----------------------------------------------------------------------------

<a name="json-interchange"></a>
## JSON parser and emitter

Once you have an `XStructure` data object, you can easily convert it to JSON string representation, as:

```c
  #include <xjson.h>

  XStructure *s = ...

  // Obtain a JSON string representation of the structure 's'.
  char *json = xjsonToString(s);
```

The above produces a proper JSON document. Or, you can do the reverse and create an `XStructure` from its JSON 
representation, either from a string (a 0-terminated `char` array):

```c
  #include <xjson.h>

  char *tail;    // for return parse position
  XStructure *s = xjsonParseString(json, &tail);
  if (s == NULL) {
     // Oops, there was some problem...
  }
```

or parse it from a file, which contains a JSON definition of the structured data:

```c
  #include <xjson.h>

  XStructure *s = xjsonParsePath("my-data.json");
  if (s == NULL) {
     // Oops, there was some problem...
  }
```

### JSON fragments

Alternatively, you can also create partial JSON fragments for individual fields, e.g.:
  
```c
  XField *f = ...
  
  // Obtain a JSON fragment for the field 'f'.
  char *json = xjsonFieldToString(f);
```

For example, for a numerical array field with 4 elements the above might generate something like:

```json
  "my-numbers": [ 1, 2, 3, 4 ]
```


### Escaped string representations

You might just want to use JSON-style escaping for strings, and `xjsonEscape()` / `xjsonUnescape()` can help with that 
too. Suppose you have a C string that you want to escape...

```c
  char *string = "\"This has some\n\t special characters\"";
  
  // Escape the special character, e.g. replace `\n` with `\` + `n` etc...
  char *escaped = xjsonEscape(string);
```

If you print `string` to a file or the standard output, it will show up as 2 lines:

```txt
  "This has some
          special characters"
```

But if you now print `escaped` instead, that will show up as:

```txt
  \"This has some\n\t special characters\"
```

And the reverse, suppose you read back the above line from an input, containing the escaped form, and want to 
reconstruct from it the original C string with the special characters in it:

```c
   // And reverse, from escaped form to ASCII (e.g. `\` + `n` --> `\n`)
   char *string = xjsonUnescape(escaped);
```


-----------------------------------------------------------------------------

<a name="xchange-error-handling"></a>
## Error handling

The functions that can encounter an error will return either one of the error codes defined in `xchange.h`, or 
`NULL` pointers. String descriptions for the error codes can be produced by `xErrorDescription(int)`. For 
example,

```c
  char *text = ...
  int status = xParseDouble(text, NULL);
  if (status != X_SUCCESS) {
    // Ooops, something went wrong...
    fprintf(stderr, "WARNING! %s", xErrorDescription(status));
    ...
  }
```

The JSON parser can also sink its error messages to a designated file or stream, which can be set by 
`xjsonSetErrorStream(FILE *)`.
 
 -----------------------------------------------------------------------------
 
<a name="xchange-debugging-support"></a>
## Debugging support

You can enable verbose output of the __xchange__ library with `xSetVerbose(boolean)`. When enabled, it will produce 
status messages to `stderr`so you can follow what's going on. In addition (or alternatively), you can enable debug 
messages with `xSetDebug(boolean)`. When enabled, all errors encountered by the library (such as invalid arguments 
passed) will be printed to `stderr`, including call traces, so you can walk back to see where the error may have 
originated from. (You can also enable debug messages by default by defining the `DEBUG` constant for the compiler, 
e.g. by adding `-DDEBUG` to `CFLAGS` prior to calling `make`). 

For helping to debug your application, the __xchange__ library provides two macros: `xvprintf()` and `xdprintf()`, 
for printing verbose and debug messages to `stderr`. Both work just like `printf()`, but they are conditional on 
verbosity being enabled via `xSetVerbose(boolean)` and `xSetDebug(boolean)`, respectively. Applications using 
__xchange__ may use these macros to produce their own verbose and/or debugging outputs conditional on the same global 
settings. 


<a name="xchange-future-plans"></a>
## Future plans

There are a number of ways this little library can evolve and grow in the not too distant future. Some of the obvious
paths forward are:

 - Add regression testing and code coverage tracking (high priority)
 - Add support for [BSON](https://bsonspec.org/spec.html) -- MongoDB's binary exchange format.
 - Add support for 128-bit floating point types (`X_FLOAT128`).
 
If you have an idea for a must have feature, please let me (Attila) know. Pull requests, for new features or fixes to
existing ones are especially welcome! 
 

-----------------------------------------------------------------------------
Copyright (C) 2025 Attila Kovács
