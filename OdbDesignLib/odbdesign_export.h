
#if defined(_WIN32)
#  if defined(OdbDesign_EXPORTS)
#    define ODBDESIGN_EXPORT __declspec(dllexport)
#	 define EXPIMP_TEMPLATE
#  else
#    define ODBDESIGN_EXPORT __declspec(dllimport)
#	 //define EXPIMP_TEMPLATE extern
#  endif
#else // non windows define it to nothing (Linux exports by default)
#  define ODBDESIGN_EXPORT
#endif
