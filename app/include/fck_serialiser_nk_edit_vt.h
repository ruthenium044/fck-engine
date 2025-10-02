#ifndef FCK_SERIALISER_NK_EDIT_VT_H_INCLUDED
#define FCK_SERIALISER_NK_EDIT_VT_H_INCLUDED

struct fck_serialiser_vt;
typedef struct nk_context fck_ui_ctx;

typedef struct fck_nk_serialiser
{
	struct fck_serialiser_vt *vt;
	fck_ui_ctx *ctx;
} fck_nk_serialiser;

extern struct fck_serialiser_vt *fck_nk_edit_vt;
extern struct fck_serialiser_vt *fck_nk_read_vt;

#endif // FCK_SERIALISER_NK_EDIT_VT_H_INCLUDED