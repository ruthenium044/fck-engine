#ifndef FCKC_APIDEF_H_INCLUDED
#define FCKC_APIDEF_H_INCLUDED

#define FCK_STATIC_API

#if defined(_WIN32) || defined(__CYGWIN__)
#define FCK_EXPORT_API __declspec(dllexport)
#define FCK_IMPORT_API __declspec(dllimport)
#endif

#if __GNUC__ >= 4
#define FCK_EXPORT_API __attribute__((visibility("default")))
#define FCK_IMPORT_API __attribute__((visibility("default")))
#endif

#endif // !FCKC_APIDEF_H_INCLUDED
