<img src="/xchange/resources/CfA-logo.png" alt="CfA logo" width="400" height="67" align="right">
<br clear="all">
Platform-agnostic data exchange framework for C/C++ with built-in JSON parser/emitter support.

## Table of Contents

 - [Introduction](#introduction)
 - [Structured data](#structured-data)
 - [JSON parser and emitter](#json-interchange)
 - [Error handling](#error-handling)
 - [Debugging support](#debugging-support)
 - [Future plans](#future-plans) 


-----------------------------------------------------------------------------

<a name="introduction"></a>
## Introduction

The __xchange__ library provides a framework for platform independent data exchange for structured data in C/C++, and
includes a JSON parser and emitter. 

While there are many excellent libraries out there that offer such capabilities for C++ and/or other object-oriented 
languages, support for structured data exchange is notably rare for standard C. The __xchange__ library aims to fill 
that niche, by providing a data exchange framework with a C90-compatible API that supports the interchange of 
arbitrary structured data between different platforms and different serialization formats with ease. The __xchange__ 
library also provides support for JSON formatting and parsing using the C90 standard out of the box. All that in a 
light-weight and fast package.

The __xchange__ library was created, and is maintained, by Attila Kovács at the Center for Astrophysics \| Harvard 
&amp; Smithsonian, and it is available through the [Smithsonian/xchange](https://github.com/Smithsonian/xchange) 
repository on GitHub. 

There are no official releases of __xchange__ yet. An initial 1.0.0 release is expected in late 2024. Before then the 
API may undergo slight changes and tweaks. Use the repository as is at your own risk for now.

Some related links:

 - [API documentation](https://smithsonian.github.io/xchange/apidoc/html/files.html)
 - [Project page](https://smithsonian.github.io/xchange) on github.io

-----------------------------------------------------------------------------

<a name="structured-data"></a>
## Structured data

 - [Basic data types](#data-types)
 - [Scalars](#scalars)
 - [Arrays](#arrays)
 - [Creating structure](#creating-structure)
 - [Aggregate IDs](#aggregate-ids)
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

<a name="data-types"></a>
### Basic data types

 - [Strings](#strings)

The __xchange__ library supports most basic (primitive) data types used across programming languages. The table below 
shows the (`XType`) types recognised by the library and their C equivalents etc.:

 | __xchange__ type | C type                   | Comment / example                                        |
 |------------------|--------------------------|----------------------------------------------------------|
 | `X_BOOLEAN`      | `boolean`<sup>*</sup>    | '`true`' or '`false`'                                    |
 | `X_BYTE`         | `char` or `int8_t`       | '`-128`' to  '`127`'                                     |
 | `X_BYTE_HEX`     | `char` or `[u]int8_t`    | '`0x0`' to  '`0xff`' (hexadeximal representation)        |
 | `X_SHORT`        | `short` or `int16_t`     | '`-32768`' to '`32767`'                                  |
 | `X_SHORT_HEX`    | `short` or `[u]int16_t`  | '`0x0`' to  '`0xffff`' (hexadeximal representation)      |
 | `X_INT`          | `int32_t`                | '`-2,147,483,648`' to '`2,147,483,647`'                  |
 | `X_INT_HEX`      | `[u]int32_t`             | '`0x0`' to  '`0xffffffff`' (hexadeximal representation)  |
 | `X_LONG`         | `long long` or `int64_t` | '`-9,223,372,036,854,775,808`' to '`9,223,372,036,854,775,807`' |
 | `X_LONG_HEX`     | `[u]int64_t`             | '`0x0`' to  '`0xffffffffffffffff`' (hex. representation) |
 | `X_FLOAT`        | `float`                  | `1`, `1.0`, `-1.234567e-33`                              |
 | `X_DOUBLE`       | `double`                 | `1`, `1.0`, `-1.2345678901234567e-111`                   |
 | `X_STRING`       | `char *`    	       | `Hello world!`, `line1\nline2\n` (0-terminated)          |
 | `X_CHARS(n) `    | `char[n]`                | Fixed-length character arrays (also w/o termination)     |
 | `X_STRUCT`       | `XStructure`             | substructure                                             |

<sup>*</sup> The `boolean` type is defined in `xchange.h`.

The `[...]_HEX` types are meaningful only for ASCII representations, and are otherwise equivalent to the corresponding 
regular integer-types of the same width. They are meant only as a way to explicitly define whether or not an integer 
value is to be represented in hexadecimal format rather than the default decimal format.

<a name="strings"></a>
#### Strings

Strings can be either fixed-length or else a 0-terminated sequence of ASCII characters. At its basic level the library 
does not impose any restriction of what ASCII characters may be used. However, we recommend that users stick to the 
JSON convention, and represent special characters in escaped form. E.g. carriagle return (`0xd`) as `\` followed by 
`n`, tabs as `\` followed by `t`, etc. As a result a single backslash should also be escaped as two consecutive `\` 
characters. You might use `xjsonEscapeString()` or `xjsonUnescapeString()` to perform the conversion to/from standard
JSON representation.

Fixed-length strings of up to _n_ characters are represented internally as the `XCHAR(n)` type. They may be 
0-terminated as appropriate, or else represent exactly _n_ ASCII characters without explicit termination. 
Alternatively, the `X_STRING` type represents ASCII strings of arbitrary length, up to the 0-termination character.

<a name="scalars"></a>
### Scalar values

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

In this way scalars are distinsguished from true arrays containing just a single elements, which have dimensionality 
&lt;=1 and shapes e.g., `int shape[1] = {1}` or `shape[2] = {1, 1}`. The difference, while subtle, becomes more 
obvious when serializing the array, e.g. to JSON. A scalar floating point value of 1.04, for example, will appear as 
`1.04` in JSON, whereas the 1D and 2D single-element arrays will be serialized as `{ 1.04 }` or `{{ 1.04 }}`, 
respectively.


<a name="arrays"></a>
### Arrays

The __xchange__ library supports array data types in one or more dimensions (up to 20 dimensions). For example, to
create a field for 2&times;3&times;4 array of `double`s, you may have something along:

```c
  double data[2][3][4] = ...;		// The native array in C
  int sizes[] = { 2, 3, 4 };		// An array containing the dimensions for xchange
  
  // Create a field for the 3-dimensional array with the specified shape.
  XField *f = xCreateField("my-array", X_DOUBLE, 3, sizes, data);
```

Note, that there is no requirement that the native array has the same dimensionality as it's nominal format in the
field. We could have declared `data` as a 1D array `double data[2 * 3 * 4] = ...`, or really any array (pointer)
containing doubles with storage for at least 24 elements. It is the `sizes` array, along with the dimensionality,
which together define the shape of the field for __xchange__.



<a name="creating-structure"></a>
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

<a name="aggregate-ids"></a>
### Aggregate IDs

Since the `XStructure` data type can represent hierarchies of arbitrary depth, with names defined at every level,
we can uniquely identify any particular field, at any level, with an aggregate ID, which concatenates the field
names eatch every level, top-down, with a separator. The convention of __xchange__ is to use colon (':') as the
separator. Consider an example structure (in JSON notation):

```
  {
    ...
    system = {
      ...
      subsystem = {
        ...
        property = "some value here";
      }
    }
  }
```

Then, the leaf "properly" entry can be 'addressed' with the aggregate ID of `system:subsystem:property` from the top
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
of direct fields in the structure. It is not much of an issue for structures that contain dozens, or even hundreds, of 
fields (per layer). For much larger structures, which have a fixed layout, there is an option for a potentially much 
faster hash-based lookup also. E.g. instead of `xGetField()` you may use `xLookupField()`:

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
to ensure that field names added this way are never duplicated.


#### Iterating over elements

You can easily iterate over the elements also. This is one application where you may want to know the internal layout
of `XStructure`, namely that it contains a simple linked-list of `XField` fields. One way to iterate over a strucures
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
structure (and its substructures) in decending alphabetical order:

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

Once you have an `XStructure` data object, you can easily convert it to a JSON string representation, as:

```c
  // Obtain a JSON string representation of the structure 's'.
  char *json = xjsonToString(s);
```

Or, you can do the reverse and create an `XStructure` from its JSON representation, either from a string (a 
0-terminated `char` array):

```c
  int lineNumber = 0;
  XStructure *s1 = xjsonParseAt(json, &lineNumber);
  if (s1 == NULL) {
     // Oops, there was some problem...
  }
```

or from a file, e.g. specified by the file name/path that contains a JSON definition of the structured data.

```c
  XStructure *s1 = xjsonParseFilename("myStructure.json", &lineNumber);
  if (s1 == NULL) {
     // Oops, there was some problem...
  }
```


-----------------------------------------------------------------------------

<a name="error-handling"></a>
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

The JSON parser &amp; emitter can also sink its error messages to a designated file or stream, which can be set by 
`xjsonSetErrorStream(FILE *)`.
 
 -----------------------------------------------------------------------------
 
<a name="debugging-support"></a>
## Debugging support

The __xchange__ library provides two macros: `xvprintf()` and `xdprintf()`, for printing verbose and debug messages
to `stderr`. Both work just like `printf()`, but they are conditional on verbosity being enabled via 
`xSetVerbose(boolean)` and the global variable `xDebug` being `TRUE` (non-zero), respectively. Applications using 
__xchange__ may use these macros to produce their own verbose and/or debugging outputs conditional on the same global 
settings. 

You can also turn debug messages by defining the `DEBUG` constant for the compiler, e.g. by adding `-DDEBUG` to 
`CFLAGS` prior to calling `make`. 


<a name="future-plans"></a>
## Future plans

There are a number of ways this little library can evolve and grow in the not too distant future. Some of the obvious
paths forward are:

 - Add regression testing and code coverage tracking (high priority)
 - Add support for [BSON](https://bsonspec.org/spec.html) -- MongoDB's binary exchange format. (It may require 
   expanding the `XType` struct for binary subtype.)
 - Add support for complex-valued data types (`X_COMPLEX`).
 - Add support for 128-bit floating point types (`X_FLOAT128`).
 - Improved debug support, e.g. with built-in error tracing.
 - Improved error handling, e.g. by consistently setting `errno` beyond just the __xchange__ error status.
 
If you have an idea for a must have feature, please let me (Attila) know. Pull requests, for new features or fixes to
existing ones are especially welcome! 
 


