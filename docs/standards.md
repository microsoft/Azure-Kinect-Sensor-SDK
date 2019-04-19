# Coding and other standards applied to Azure Kinect project

These standards apply all code in this repository.

## Modules

A module is a small library that provides functionality that is a building block SKD. Functionally a module has a create and destroy function. It also uses a handle so that module state can be maintained. Module naming should be simple, short, and without underscores. The module file name should match the actual module name.
Modules should be coded in C whenever possible. It is acceptable to introduce CPP or other type files to link in
existing projects written in another language. The use of additional languages should be minimized where possible.

Each module should have tests written for it to verify its functionality. Depending on the scope of functionality the test may be unit based or functional.

## CMakeLists.txt

CMake projects should be self describing.
If a CMakeLists.txt file is describing a new library, then all dependencies for the new library should be defined in the
file. CMake should automatically know where to find the libraries header files, dependencies, and library itself, all by
specifying the target dependency through 'target_link_libraries' consumed by a user.

CMakeLists.txt projects shall also include k4ainternal::[alias] and an internal shortcut to the library. This will
allow the reader to quickly and easily know the difference between internal and public dependencies.

## Coding Standards

* Use [Clang-Format](../.clang-format) to manage document formatting
* Macros are all capital letters
* Naming of structures, types, enums are:
  * All lower case.
  * Underscore used to separate words.
  * Should end with \_t to indicate it is a type.
  * Naming should be [area]\_[noun]\_t.
  * Public types should be k4a_[area]\_[noun]\_t.
  * Enums individual elements:
    * Should use the same [area]\_[noun] as the beginning of each element.
    * Should separate words with '_'.
    * Should be in all capital letters.
* Naming of functions are:
    * All lower case
    * Underscore used to separate words:
    * Naming should be [area]\_[verb]\_[noun]
    * Public functions should be k4a\_[area]\_[verb]\_[noun]
* Stop / Free / Destroy functions should not return an error type. If it is not possible to handle all error cases, and
 there is nothing the user can do to resolve the issue, then it is acceptable to crash. If there is action the
 user could take (not including the total application restart or PC reboot) then returning an error specific to the recovery
 action is acceptable.

 ## Clang-format

 * The default build enforces clang-format cleanliness. The K4A_VALIDATE_CLANG_FORMAT CMake option controls this
   behavior
 * To reformat source files automatically, run the "clangformat" target (e.g. "ninja clangformat")
