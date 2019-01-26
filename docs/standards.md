# Codeing and other standards applied to K4A project

These standards apply all code in this repository.

## Modules
Module naming should be simple, short, and without underscores. The module file name should match the actual module name.
Modules should be coded in C when ever possible. It is acceptable to introduce CPP or other type files to link in
existing projects written in another langue. The use of these additional languages should be mininimzed where possible.

## CMakeLists.txt
CMake projects should be self describing.
If a CMakeLists.txt file is describing a new library, then all dependencys for the new library should be defined in the
file. CMake should automatically know where to find the libraries header files, dependencies, and library itself, all by
specifying the target dependecy through 'target_link_libraries' consumed by a user.

CMakeLists.txt projects shall also include k4ainternal::[alias] and an internal shortcut to the library. This will
allow the reader to quickly and easily know the difference between internal and public dependencies.

## Coding Standards

* Macros are all capital letters
* Naming of structures, types, enums are:
    * All lower case.
    * Underscore used to seperate words.
    * Naming should be [area]\_[noun].
    * TODO? Public types should be k4a_[area]\_[noun].
    * Enums individual elements:
        * should use the same [area]\_[noun] as the beginning of each element.
        * Should seperate words with '_'.
        * Should be in all capitol letters.
* Naming of functions are:
    * All lower case
    * Underscore used to seperate words:
    * Naming should be [area]\_[verb]_[noun]
    * Public functions should be k4a_[area]\_[verb]_[noun]
* Stop / Free / Destroy functions should not return an error. If it is not possible to handle all error cases, and
 there is nothing the user can do to resolve the issue, then it is acceptable to crash. If the there is action the
 user could take (not including action the total restart or PC reboot) then returning an error specific to the recovery
 action is acceptable.

 ## Clang-format

 * The default build enforces clang-format cleanliness. The K4A_VALIDATE_CLANG_FORMAT cmake option controls this
   behavior
 * To reformat source files automatically, run the "clangformat" target (e.g. "ninja clangformat")
