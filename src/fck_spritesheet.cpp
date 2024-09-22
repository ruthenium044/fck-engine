#include "fck_spritesheet.h"

#include <fck_memory_stream.h>
#include <SDL3/SDL_assert.h>
#include <SDL3/SDL_surface.h>
#include <SDL3_image/SDL_image.h>

static void rect_list_allocate(fck_rect_list *list, size_t capacity)
{
	list->data = (SDL_FRect *)SDL_calloc(capacity, sizeof(*list->data));
	list->capacity = capacity;
	list->count = 0;
}

static void rect_list_free(fck_rect_list *list)
{
	SDL_free(list->data);
	list->data = nullptr;
	list->capacity = 0;
	list->count = 0;
}

static bool rect_list_try_add(fck_rect_list *list, SDL_FRect const *rect)
{
	SDL_assert(list != nullptr);
	SDL_assert(list->data != nullptr);

	if (list->count >= list->capacity)
	{
		size_t new_capacity = list->count + 1;
		SDL_FRect *next_memory = (SDL_FRect *)SDL_realloc(list->data, sizeof(*list->data) * new_capacity);
		if (next_memory == nullptr)
		{
			return false;
		}
		list->data = next_memory;
		list->capacity = new_capacity;
	}

	list->data[list->count] = *rect;
	list->count = list->count + 1;
	return true;
}

struct extreme_points
{
	int min_x;
	int min_y;
	int max_x;
	int max_y;
};

static void point_list_allocate(fck_point_list *list, size_t capacity)
{
	list->points = (SDL_Point *)SDL_calloc(capacity, sizeof(*list->points));
	list->capacity = capacity;
	list->count = 0;
}

static void point_list_free(fck_point_list *list)
{
	SDL_free(list->points);
	list->points = nullptr;
	list->capacity = 0;
	list->count = 0;
}

static void point_list_push(fck_point_list *list, SDL_Point rect)
{
	SDL_assert(list != nullptr);
	SDL_assert(list->points != nullptr);

	if (list->count >= list->capacity)
	{
		size_t new_capacity = list->count + 1;
		SDL_Point *next_memory = (SDL_Point *)SDL_realloc(list->points, sizeof(*list->points) * new_capacity);
		SDL_assert(next_memory != nullptr);

		list->points = next_memory;
		list->capacity = new_capacity;
	}

	list->points[list->count] = rect;
	list->count = list->count + 1;
}

static bool point_list_try_pop(fck_point_list *list, SDL_Point *point)
{
	SDL_assert(list != nullptr);
	SDL_assert(list->points != nullptr);

	if (list->count <= 0)
	{
		return false;
	}

	*point = list->points[list->count - 1];
	list->count = list->count - 1;
	return true;
}

static void surface_flood_fill(SDL_Surface *surface, bool *is_closed, int sx, int sy, extreme_points *extremes)
{
	fck_point_list open_list;
	SDL_zero(open_list);
	point_list_allocate(&open_list, 16);
	point_list_push(&open_list, SDL_Point{sx, sy});

	SDL_Point current_point;
	while (point_list_try_pop(&open_list, &current_point))
	{
		int x = current_point.x;
		int y = current_point.y;
		size_t current = (y * surface->w) + x;
		if (is_closed[current])
		{
			continue;
		}
		is_closed[current] = true;

		Uint8 r, g, b, a;
		CHECK_ERROR(SDL_ReadSurfacePixel(surface, x, y, &r, &g, &b, &a), SDL_GetError(), continue);

		bool is_empty = r == 0 && g == 0 && b == 0 && a == 0;
		if (is_empty)
		{
			continue;
		}

		extremes->min_x = SDL_min(extremes->min_x, x);
		extremes->min_y = SDL_min(extremes->min_y, y);
		extremes->max_x = SDL_max(extremes->max_x, x);
		extremes->max_y = SDL_max(extremes->max_y, y);

		if (x > 0)
		{
			point_list_push(&open_list, SDL_Point{x - 1, y});
		}
		if (x <= surface->w)
		{
			point_list_push(&open_list, SDL_Point{x + 1, y});
		}
		if (y > 0)
		{
			point_list_push(&open_list, SDL_Point{x, y - 1});
		}
		if (y <= surface->h)
		{
			point_list_push(&open_list, SDL_Point{x, y + 1});
		}
	}

	point_list_free(&open_list);
}

static bool surface_identify_sprites(SDL_Surface *surface, fck_rect_list *rect_list)
{
	rect_list_allocate(rect_list, 256);

	bool *is_closed = (bool *)SDL_calloc(surface->w * surface->h, sizeof(bool));

	for (int y = 0; y < surface->h; y++)
	{
		for (int x = 0; x < surface->w; x++)
		{
			size_t index = (y * surface->w) + x;
			if (!is_closed[index])
			{
				extreme_points extreme_points{INT32_MAX, INT32_MAX, INT32_MIN, INT32_MIN};
				surface_flood_fill(surface, is_closed, x, y, &extreme_points);
				if (extreme_points.max_x != INT32_MIN && extreme_points.min_x != INT32_MAX &&
				    extreme_points.max_y != INT32_MIN && extreme_points.min_y != INT32_MAX)
				{
					SDL_FRect rect = {float(extreme_points.min_x), float(extreme_points.min_y),
					                  float(extreme_points.max_x) - extreme_points.min_x + 1,
					                  float(extreme_points.max_y) - extreme_points.min_y + 1};
					if (!rect_list_try_add(rect_list, &rect))
					{
						rect_list_free(rect_list);

						return false;
					}
					x = extreme_points.max_x;
				}
			}
		}
	}
	return true;
}

static void spritesheet_config_save(const char *file_name, fck_spritesheet const *out_sprites)
{
	fck_memory_stream stream;
	SDL_zero(stream);

	const size_t data_byte_size = sizeof(*out_sprites->rect_list.data) * out_sprites->rect_list.count;
	fck_memory_stream_write(&stream, (void *)&out_sprites->rect_list.count, sizeof(out_sprites->rect_list.count));
	fck_memory_stream_write(&stream, out_sprites->rect_list.data, data_byte_size);

	CHECK_CRITICAL(fck_file_write("", file_name, ".sprites", stream.data, stream.count), "Failed to write file");

	fck_memory_stream_close(&stream);
}

static bool spritesheet_config_exists(const char *file_name)
{
	return fck_file_exists("", file_name, ".sprites");
}

static void spritesheet_config_load(const char *file_name, fck_spritesheet *out_sprites)
{
	fck_memory_stream stream;
	SDL_zero(stream);

	fck_file_memory file_memory;
	CHECK_CRITICAL(fck_file_read("", file_name, ".sprites", &file_memory), "Failed to read file", return);

	stream.data = file_memory.data;
	stream.capacity = file_memory.size;

	out_sprites->rect_list.count = *(size_t *)fck_memory_stream_read(&stream, sizeof(out_sprites->rect_list.count));

	const size_t data_byte_size = sizeof(*out_sprites->rect_list.data) * out_sprites->rect_list.count;
	out_sprites->rect_list.data = (SDL_FRect *)fck_memory_stream_read(&stream, data_byte_size);
}

void fck_rect_list_view_create(fck_rect_list *list, size_t at, size_t count, fck_rect_list_view *view)
{
	SDL_assert(list != nullptr);
	SDL_assert(list->data != nullptr);
	SDL_assert(view != nullptr);
	SDL_assert(at + count < list->count);
	// view->rects is allowed to be != nullptr since it is non-alloc view span

	view->rect_list = list;
	view->begin = at;
	view->count = count;
}

SDL_FRect const *fck_rect_list_view_get(fck_rect_list_view const *view, size_t at)
{
	SDL_assert(view != nullptr);
	SDL_assert(view->rect_list != nullptr);
	SDL_assert(view->rect_list != nullptr);
	SDL_assert(at < view->count);

	SDL_FRect *view_begin = view->rect_list->data + view->begin;
	return view_begin + at;
}

void fck_spritesheet_free(fck_spritesheet *sprites)
{
	SDL_DestroyTexture(sprites->texture);
	sprites->texture = nullptr;
	rect_list_free(&sprites->rect_list);
}

bool fck_spritesheet_load(SDL_Renderer *renderer, const char *file_name, fck_spritesheet *out_sprites,
                          bool force_rebuild)
{
	SDL_assert(renderer != nullptr);
	SDL_assert(file_name != nullptr);
	SDL_assert(out_sprites != nullptr);

	fck_file_memory file_memory;
	if (!fck_file_read("", file_name, "", &file_memory))
	{
		return false;
	}

	SDL_IOStream *stream = SDL_IOFromMem(file_memory.data, file_memory.size);
	CHECK_ERROR(stream, SDL_GetError(), return false);

	// IMG_Load_IO frees stream!
	SDL_Surface *surface = IMG_Load_IO(stream, true);
	fck_file_free(&file_memory);
	CHECK_ERROR(surface, SDL_GetError(), return false);

	const SDL_Palette *palette = SDL_GetSurfacePalette(surface);
	const SDL_PixelFormatDetails *details = SDL_GetPixelFormatDetails(surface->format);
	CHECK_ERROR(details, SDL_GetError(), return false);

	if (force_rebuild || !spritesheet_config_exists(file_name))
	{
		fck_rect_list rects;
		surface_identify_sprites(surface, &out_sprites->rect_list);
		CHECK_ERROR(out_sprites->rect_list.data, "Failure in identifying rects");
		spritesheet_config_save(file_name, out_sprites);
	}
	else
	{
		spritesheet_config_load(file_name, out_sprites);
	}

	// Maybe generalise bitmap transparency
	bool set_color_key_result = SDL_SetSurfaceColorKey(surface, true, SDL_MapRGB(details, palette, 248, 0, 248));
	CHECK_ERROR(set_color_key_result, SDL_GetError(), return false);

	out_sprites->texture = SDL_CreateTextureFromSurface(renderer, surface);

	SDL_DestroySurface(surface);

	CHECK_ERROR(out_sprites->texture, SDL_GetError(), return false);

	return true;
}