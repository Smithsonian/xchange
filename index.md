---
excerpt: structured data representation and JSON support for C/C++.
---

<img src="/xchange/resources/Sigmyne-logo-200x44.png" alt="Sigmyne logo" width="200" height="44" align="right"><br clear="all">
<img src="https://img.shields.io/github/v/release/Sigmyne/xchange?label=github" class="badge" alt="GitHub release version" align="left">
<img src="https://img.shields.io/vcpkg/v/libxchange" class="badge" alt="vcpkg version">
<img src="https://img.shields.io/fedora/v/libxchange?color=lightblue" class="badge" alt="Fedora package version" align="left">
<img src="https://img.shields.io/badge/dynamic/json.svg?url=https://raw.githubusercontent.com/attipaci/homebrew-pub/pkginfo/xchange.json&query=$.versions.stable&label=homebrew" class="badge" alt="Homebrew package version" align="left">
<br clear="all">

The __xchange__ library provides structured data representation and exchange in C/C++, and includes support for JSON 
parsing and generation. It is free to use, in any way you like, without licensing restrictions.

For JSON parsing end emitting, __xchange__ provides a higher-level data model than __cjson__, with high-level 
functions for accessing and manipulating data both with less code and with cleaner code.

The __xchange__ library was created, and is maintained, by Attila Kovács (Sigmyne, LLC), and it is available through 
the [Sigmyne/xchange](https://github.com/Sigmyne/xchange) repository on GitHub. 

This site contains various online resources that support the library:

__Documentation__

 - [User's guide](doc/README.md) (`README.md`)
 - [API Documentation](apidoc/html/files.html)
 - [History of changes](doc/CHANGELOG.md) (`CHANGELOG.md`)
 - [Issues](https://github.com/Sigmyne/xchange/issues) affecting __xchange__ releases (past and/or present)
 - [Community Forum](https://github.com/Sigmyne/xchange/discussions) &ndash; ask a question, provide feedback, or 
   check announcements.
 
__Downloads__

 - [Releases](https://github.com/Sigmyne/xchange/releases) from GitHub

__Linux Packages__

SuperNOVAS is also available in packaged for for Fedora / EPEL It has the following package structure, which allows 
non-bloated installations of just the parts that are needed for the particular use case(s):

 | __Fedora / EPEL RPM__                 |  Description                                 |
 |---------------------------------------|----------------------------------------------|
 | `libxchange`                          | Runtime library (`libxchange.so.1`)          |
 | `libxchange-devel`                    | C headers and unversion shared library       |
 | `libxchange-doc`                      | HTML Developer documentation                 |
 

__vcpkg Registry__

You can also install the __xchange__ library with `vcpkg` on Linux, MacOS, Windows, and Android as:

```bash
  $ vcpkg install libxchange
```

__Homebrew__ 

Or, install via the Homebrew package manager (MacOS and Linux) through the maintainer's own Tap:

```bash
  $ brew tap attipaci/pub
  $ brew install xchange
```

For more customized installations, you can add the following options also:

 | Option                 | Description                                                                    |
 |:---------------------- |:------------------------------------------------------------------------------ |
 | `--with-doxygen`       | Install local HTML documentation, compiled with Doxygen, also                  |

