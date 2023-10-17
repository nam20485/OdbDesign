#
# patch-vcpkg-install.ps1
#

# have to patch the vcpkg install by copying this file over its counterpart because the vcpkg version 
# included with VS has a defect in it (fixed in a preview version, but not in a released version yet)

# see vcpkg GitHub issue #33506 and comment with workaround:
# https://github.com/microsoft/vcpkg/issues/33506#issuecomment-1704949644

$msys_path = "$env:VCPKG_ROOT\scripts\msys"
If (!(Test-Path -PathType container $msys_path))
{
    New-Item -ItemType Directory -Path $msys_path -Verbose
}

Copy-Item -Force -Verbose scripts\patch\compile_wrapper_consider_clang-cl.patch "$msys_path\compile_wrapper_consider_clang-cl.patch"
