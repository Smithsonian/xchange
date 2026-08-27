# Changelog

All notable changes to the [Sigmyne/xchange](https://github.com/Sigmyne/xchange) library will be documented 
in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/), and this project adheres to 
[Semantic Versioning](https://semver.org/spec/v2.0.0.html).


## [1.3.0] - 2026-08-27

Feature release, possibly around 1 September 2026.

### Fixed

 - #33: `x_snprintf()` to return the actual number of bytes printed, since this is how the library and its 
   dependencies have used this function.

 - CMake `xchangeConfig` to skip requiring math lib for non-Windows platforms in general, since it can fail if the 
   math library is in the build path, but not in the search path, such as for some cross builds (see e.g. the vcpkg 
   Android builds)

### Added

 - #32: Added `XBoolean` as the xchange-specific boolean type, replacing `boolean` in prior versions. (The `boolean`
   type is still defined for compatibility, but can be disabled by defining `_TYPEDEF_BOOLEAN` prior to including
   `xchange.h` in your source code.).

 - Added `xIsDebug()` function to check on `xDebug`. While the global variable is fine in most cases, they are 
   problematic for Windows DLLs. It's better to use purely functional access instead.
   
 - `xvprintf()` / `xdprintf()` macros to reference functions instead of global vars (see above comment on Windows
   DLLs).
   
### Changed

 - #32: Changed `boolean` parameter and return types to the equivalent `XBoolean`, to disambiguate the 
   xchange-specific boolean type from other definitions. (Other header may define `boolean` also, and so by choosing
   a more unique type name, we reduce the chance of namespace conflicts.)
   
 - #36: Staged GitHub Actions workflows, to fail early and cancel dependent jobs in case of errors.
   

## [1.2.0] - 2026-06-08

High priority bug fixes and version bump.

### Fixed

 - #26: Fixed space allocation in `xLookupPutAllAsync()` / `xLookupRemoveAllAsync()`.
 
 - #28: `xGetAsDoubleAtIndex()` returned integer rounded values when the field stored `float` or `double` type data. 
   Now, it returns the floating-point value directly.
 
 - Fixed memleak in `xLookupRemove()`.
 
 - Fixed memleak in `xDestroyLookup()`.
 
 - `xLookupPut()` did not return `X_NO_INIT` as expected if lookup table was not initialized.

 - Fix potential buffer overflow at build time in `docedit.c` (`sprintf()` to `snprintf()`).
 
 - Fixed max string size for 8-byte signed integer (-> 20 bytes + termination).
 
 - Fixed insufficient checking in `xIsFieldValid()`.
 
 - Fixed botched read loop in `xjsonParseFile()`.
 
 - Removed the unwanted `fclose()` in `xjsonParseFile()`. Caller is responsible for closing the file after the call, 
   as appropriate.
   
 - Extra comma when printing `X_FLOAT` value to JSON.
 
 - Fixed JSON unicode processing.
 
 - Fixed `PrintPrimitive()` return value for `X_CHAR(n)` types in `xjson.c`.
 
 - `xGetAsLongAtIndex()` and `xGetAsDoubleAtIndex()` for string/raw types.

### Added

 - Now installing `xmutex.h` containing portable mutex macros.

### Changed

 - #29: Use `snprintf()` instead of `sprintf()` provided it's available. (On older platforms prior to the C99 
   standard, it defaults to `sprintf()`.)
   
 - #29: Use `size_t` or `long` (if needs to be signed) instead of `int` for string length parameters.

 - Changed `xlookup` hash algorithm to FNV-1a.
 
 - Rounding instead of downcast from double in `xGetAsLongAtIndex()`.

 - CMake install to skip `.gitignore` in `examples/`.
 
 - `Makefile` doc install to match CMake. 

 - CMake export targets from build-directory (for dependent builds)
 
 - `examples/Makefile` to work standalone, without `config.mk`.


## [1.1.2] - 2026-04-27

Maintenance release with improved portability (esp. Windows, MacOS, and BSD). It also enables ports to
vcpkg and Homebrew (coming very soon...).

### Fixed

 - #23: Removed unneeded includes of UNIX-specific `unistd.h` (blocking portability). 
 
### Added

 - #23: Portability to Windows, MacOS, and BSD.

 - #23: CMake build configuration, alongside the GNU make config.
 
 - #23: New GitHub Actions workflows for multi-platform checks.

### Changed
 
 - #23: Use portable mutexes in `xlookup.c`.
 
 - #23: `xjson` error/warning reporting via `static` functions instead of niche macros that Windows does not support.

 - #23: `xGetAsLongAtIndex()` and `xGetAsDoubleAtIndex()` switch to setting errno to `EINVAL` instead of `ENOSR`, as
   the latter is not defined on BSD.


## [1.1.1] - 2026-02-16

Maintenance release with minor code style improvements.

### Changed

 - #18: A few code style improvements, spotted by cppcheck.
 

## [1.1.0] - 2025-11-10

Minor feature release with bug fixes.

### Fixed

 - #15: `xPrintFloat()` printed an extra digit, which would appear as a 'rounding error' in decimal representations.

 - #16: Width detection of platform-specific built-in integer types (i.e., `short`, `int`, `long`, and `long long`). 
   The previous implementation included `stdint.h` with `__STDC_LIMIT_MACROS` defined. However, if the application 
   source, then included `stdint.h` _before_ `xchange.h`, then the fixed-width integer limits were left undefined. As 
   a result, we no longer rely on `stdint.h` providing these limits.

### Added

 - `xParseFloat()` to parse floats without rounding errors that might result if parsing as `double` and then casting 
   as `float`.


## [1.0.1] - 2025-07-01

Bug fix release.

### Fixed

 - Handling of serialized strings in `xClearField()` and `xCopyOfField()`.
 
 - Handling of heterogeneous arrays (type `X_FIELD`) in `xCopyOfField()`.

### Added
 
 - `xDestroyLookupAndData()` to destroy a lookup table _including_ all the data that was referenced inside it. 
 
### Changed

 - `xCreateField()` to treat `X_RAW` types always as scalars, ignoring the dimensions provided.
 

## [1.0.0] - 2025-03-31

Initial public release.
