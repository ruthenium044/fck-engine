#ifndef FCKC_APIDEF_H_INCLUDED
#define FCKC_APIDEF_H_INCLUDED

// TODO: get rid off this dumb fckc prefix... fck is enough
// TODO: no caps

#if defined(fck_static_export)
	#define FCK_EXPORT_API
	#define FCK_IMPORT_API
#else
	#if defined(_WIN32) || defined(__CYGWIN__)
		#define FCK_EXPORT_API __declspec(dllexport)
		#define FCK_IMPORT_API __declspec(dllimport)
	#elif __GNUC__ >= 4
		#define FCK_EXPORT_API __attribute__((visibility("default")))
		#define FCK_IMPORT_API __attribute__((visibility("default")))
	#else
		#define FCK_EXPORT_API
		#define FCK_IMPORT_API
	#endif
#endif

#endif // !FCKC_APIDEF_H_INCLUDED
