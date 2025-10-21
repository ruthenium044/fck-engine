#ifndef FCK_SERIALISER_STRING_VT_H_INCLUDED
#define FCK_SERIALISER_STRING_VT_H_INCLUDED

#include <fckc_apidef.h>

#if defined(FCK_SER_EXT_STRING_EXPORT)
#define FCK_SER_EXT_STRING_API FCK_EXPORT_API
#else
#define FCK_SER_EXT_STRING_API FCK_IMPORT_API
#endif

FCK_SER_EXT_STRING_API extern struct fck_serialiser_vt* fck_string_writer_vt;
FCK_SER_EXT_STRING_API extern struct fck_serialiser_vt* fck_string_reader_vt;

#endif // FCK_SERIALISER_STRING_VT_H_INCLUDED