
#if defined(_WIN32)
#  if defined(OdbDesign_EXPORTS)
#    define DECLSPEC __declspec(dllexport)
#	 define EXPIMP_TEMPLATE
#  else
#    define DECLSPEC __declspec(dllimport)
#	 //define EXPIMP_TEMPLATE extern
#  endif
#else // non windows
#  define DECLSPEC
#endif
