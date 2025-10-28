#include <fck_canvas.h>

#include <fck_apis.h>
#include <fckc_apidef.h>

#include <fck_render.h>

int fck_canvas_draw_sprite(fck_renderer *renderer, fck_texture const *texture, fck_rect_src const *src, fck_rect_dst const *dst)
{
	fckc_u32 width;
	fckc_u32 height;
	int result = renderer->vt->texture->dimensions(*texture, &width, &height);
	if (!result)
	{
		return 0;
	}

	fckc_f32 uv_min_x = (src->min_x / width);
	fckc_f32 uv_min_y = (src->min_y / height);
	fckc_f32 uv_max_x = (src->max_x / width);
	fckc_f32 uv_max_y = (src->max_y / height);

	fckc_f32 dst_half_w = dst->w * 0.5f;
	fckc_f32 dst_half_h = dst->h * 0.5f;
	fckc_f32 dst_min_x = dst->cx - dst_half_w;
	fckc_f32 dst_min_y = dst->cy - dst_half_h;
	fckc_f32 dst_max_x = dst->cx + dst_half_w;
	fckc_f32 dst_max_y = dst->cy + dst_half_h;

	fck_vertex_2d vertices[] = {
		(fck_vertex_2d){.position[0] = dst_min_x,
	                    .position[1] = dst_min_y,
	                    .uv[0] = uv_min_x,
	                    .uv[1] = uv_min_y,
	                    .col[0] = 255,
	                    .col[1] = 255,
	                    .col[2] = 255,
	                    .col[3] = 255},
		(fck_vertex_2d){.position[0] = dst_min_x,
	                    .position[1] = dst_max_y,
	                    .uv[0] = uv_min_x,
	                    .uv[1] = uv_max_y,
	                    .col[0] = 255,
	                    .col[1] = 255,
	                    .col[2] = 255,
	                    .col[3] = 255},
		(fck_vertex_2d){.position[0] = dst_max_x,
	                    .position[1] = dst_max_y,
	                    .uv[0] = uv_max_x,
	                    .uv[1] = uv_max_y,
	                    .col[0] = 255,
	                    .col[1] = 255,
	                    .col[2] = 255,
	                    .col[3] = 255},
		(fck_vertex_2d){.position[0] = dst_max_x,
	                    .position[1] = dst_min_y,
	                    .uv[0] = uv_max_x,
	                    .uv[1] = uv_min_y,
	                    .col[0] = 255,
	                    .col[1] = 255,
	                    .col[2] = 255,
	                    .col[3] = 255},
	};
	fck_index indices[6] = {0, 1, 2, 2, 3, 0};

	return renderer->vt->raw(renderer->obj, *texture,           //
	                         vertices, fck_arraysize(vertices), //
	                         indices, fck_arraysize(indices));  //
}

int fck_canvas_draw_rect(fck_renderer *renderer, fck_rect_dst const *dst)
{
	fckc_f32 dst_half_w = dst->w * 0.5f;
	fckc_f32 dst_half_h = dst->h * 0.5f;
	fckc_f32 dst_min_x = dst->cx - dst_half_w;
	fckc_f32 dst_min_y = dst->cy - dst_half_h;
	fckc_f32 dst_max_x = dst->cx + dst_half_w;
	fckc_f32 dst_max_y = dst->cy + dst_half_h;

	fck_vertex_2d vertices[] = {
		(fck_vertex_2d){.position[0] = dst_min_x, .position[1] = dst_min_y, .col[0] = 1.0f, .col[1] = 1.0f, .col[2] = 1.0f, .col[3] = 1.0f},
		(fck_vertex_2d){.position[0] = dst_min_x, .position[1] = dst_max_y, .col[0] = 1.0f, .col[1] = 1.0f, .col[2] = 1.0f, .col[3] = 1.0f},
		(fck_vertex_2d){.position[0] = dst_max_x, .position[1] = dst_max_y, .col[0] = 1.0f, .col[1] = 1.0f, .col[2] = 1.0f, .col[3] = 1.0f},
		(fck_vertex_2d){.position[0] = dst_max_x, .position[1] = dst_min_y, .col[0] = 1.0f, .col[1] = 1.0f, .col[2] = 1.0f, .col[3] = 1.0f},
	};
	fck_index indices[6] = {0, 1, 2, 2, 3, 0};

	return renderer->vt->raw(renderer->obj, renderer->vt->texture->null(), //
	                         vertices, fck_arraysize(vertices),            //
	                         indices, fck_arraysize(indices));             //
}

static fck_canvas_api canvas_api = {
	.sprite = fck_canvas_draw_sprite,
	.rect = fck_canvas_draw_rect,
};

FCK_EXPORT_API fck_canvas_api *fck_main(fck_api_registry *apis, void *params)
{
	apis->add("FCK_CANVAS", &canvas_api);
	return &canvas_api;
}