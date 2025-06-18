/**
 * @name OdbDesign Security Checks
 * @description Custom security checks for OdbDesign C++ project
 * @kind problem
 * @problem.severity warning
 * @precision medium
 * @id cpp/odbdesign-security
 * @tags security
 *       correctness
 *       odbdesign
 */

import cpp

// Check for unsafe string operations
from FunctionCall fc, Function f
where 
  f = fc.getTarget() and
  (
    // Unsafe C string functions
    f.hasGlobalName("strcpy") or
    f.hasGlobalName("strcat") or
    f.hasGlobalName("sprintf") or
    f.hasGlobalName("gets") or
    // Potentially unsafe buffer operations
    f.hasGlobalName("memcpy") or
    f.hasGlobalName("memmove")
  ) and
  // Exclude protobuf generated files
  not fc.getFile().getAbsolutePath().matches("%/protobuf/%") and
  not fc.getFile().getAbsolutePath().matches("%.pb.%") and
  // Exclude vcpkg dependencies
  not fc.getFile().getAbsolutePath().matches("%/vcpkg/%") and
  not fc.getFile().getAbsolutePath().matches("%/vcpkg_installed/%")
select fc, "Potentially unsafe string/buffer operation: " + f.getName() + 
          ". Consider using safer alternatives like strncpy, strncat, or std::string."
