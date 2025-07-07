# Changelog

All notable changes to the [Smithsonian/xchange](https://github.com/Smithsonian/xchange) library will be documented 
in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/), and this project adheres to 
[Semantic Versioning](https://semver.org/spec/v2.0.0.html).


## [Unreleased]

### Added

 - `xParseFloat()` to parse floats without rounding errors that might result if parsing as `double` and then casting 
   as `float`.

## [1.0.1] - 2025-07-01

### Fixed

 - Handling of serialized strings in `xClearField()` and `xCopyOfField()`.
 
 - Handling of heterogeneous arrays (type `X_FIELD`) in `xCopyOfField()`.

### Added
 
 - `xDestroyLookupAndData()` to destroy a lookup table _including_ all the data that was referenced inside it. 
 
### Changed

 - `xCreateField()` to treat `X_RAW` types always as scalars, ignoring the dimensions provided.
 

## [1.0.0] - 2025-03-31

Initial public release.
