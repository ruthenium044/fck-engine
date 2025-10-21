#ifndef FCKC_APIDEF_H_INCLUDED
#define FCKC_APIDEF_H_INCLUDED

#define FCK_STATIC_API

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

#endif // !FCKC_APIDEF_H_INCLUDED
