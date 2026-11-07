#pragma once

// Parser-only umbrella header: exposes the FileModel / ProductModel surface
// (ODB++ archive parsing and product model construction) WITHOUT any of the
// embedded server app classes. Library customers integrating OdbDesign into
// their own applications should include this header instead of OdbDesign.h,
// which additionally pulls in App/ (DesignCache, server app base) and,
// transitively, the Crow HTTP headers.

#include "FileModel/Design/FileArchive.h"
#include "FileModel/parse_error.h"
#include "FileModel/invalid_odb_error.h"

#include "ProductModel/Design.h"
