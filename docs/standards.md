# Coding and Other Standards

These standards apply all code in this repository.

## Modules

A module is a small library that provides functionality that is a building block of the SDK. Modules must 
have a create and destroy function and use a handle to maintain module state. Module naming should be
simple, short, and without underscores. The module file name should match the actual module name. Modules 
should be coded in C whenever possible. It is acceptable to introduce C++ or other types of files to link in 
existing projects written in another language. The use of additional languages should be minimized where possible.

Each module should have tests written for it to verify its functionality. Depending on the scope of 
functionality, the test may be unit based or functional.

## CMakeLists.txt

CMake projects should be self describing.
If a CMakeLists.txt file is describing a new library, then all dependencies for the new library should be defined in the
file. CMake should automatically know where to find the library's header files, dependencies, and the library itself, by
specifying the target dependency through 'target_link_libraries'.

CMakeLists.txt projects shall also include k4ainternal::[alias] and an internal shortcut to the library. This will
allow the reader to quickly and easily tell the difference between internal and public dependencies.

## Coding Standards

* Use [Clang-Format](../.clang-format) to manage document formatting.
* Macros are all uppercase.
* Naming of structures, types, enums use:
  * Lowercase.
  * Underscore to separate words.
  * \_t at the end of the name to indicate it is a type.
  * [area]\_[noun]\_t format.
  * Prepend k4a_ to public types; i.e. k4a_[area]\_[noun]\_t.
* Enum element names additionally use:
  * The same [area]\_[noun] as the beginning of each element.
  * Underscore to separate words.
  * uppercase.
* Naming of functions use:
  * Lowercase.
  * Underscores to separate words.
  * [area]\_[verb]\_[noun] format.
  * k4a_ at the start of public functions; i.e. k4a\_[area]\_[verb]\_[noun].
* Stop / Free / Destroy functions should not return an error type. If it is not possible to handle all error cases, and
 there is nothing the user can do to resolve the issue, then it is acceptable to crash. If there is action the
 user could take (not including an application restart or PC reboot), then returning an error specific to the recovery
 action is acceptable.
* Don't use default parameters in public APIs (C++/C#)

## Clang-format

* The default build enforces clang-format cleanliness. The K4A_VALIDATE_CLANG_FORMAT CMake option controls this
behavior.
* To reformat source files automatically, run the "clangformat" target (e.g. "ninja clangformat").
