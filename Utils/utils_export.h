
#if defined(_WIN32)
#  if defined(Utils_EXPORTS)
#    define UTILS_EXPORT __declspec(dllexport)
#	 define EXPIMP_TEMPLATE
#  else
#    define UTILS_EXPORT __declspec(dllimport)
#	 //define EXPIMP_TEMPLATE extern
#  endif
#else // non windows
#  define UTILS_EXPORT
#endif
