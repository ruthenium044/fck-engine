// fck main
// TODO:
// - Graphics
// - Input handling (x)
// - Draw some images
// - Systems and data model
// - Data serialisation
// - Frame independence
// - Networking!! <- implies multiplayer

// SDL core - functionality such as creating a window and getting events
#include <SDL3/SDL.h>

// SDL image - Loads images... Many kinds. We only care about PNG
#include <SDL3_image/SDL_image.h>

// SDL net - networking... More later
#include <SDL3_net/SDL_net.h>

#include "fck_checks.h"
#include "fck_drop_file.h"
#include "fck_ecs_opaque_style.h"
#include "fck_file.h"
#include "fck_keyboard.h"
#include "fck_memory_stream.h"
#include "fck_mouse.h"
#include "fck_spritesheet.h"
#include "fck_ui.h"

struct fck_sprite
{
	SDL_FRect src;
};

struct fck_sprite_storage
{
	// Storage stuff
	SDL_Texture *texture;
	fck_sprite *sprites;
	size_t count;

	// Some configuration for drawing
	SDL_FlipMode flip_mode;
};

void fck_sprite_storage_alloc(fck_sprite_storage *storage, SDL_Texture *texture, size_t count)
{
	SDL_assert(storage != nullptr);
	storage->texture = texture;
	storage->sprites = (fck_sprite *)SDL_calloc(count, sizeof(fck_sprite));
	storage->count = count;
	storage->flip_mode = SDL_FLIP_NONE;
}

void fck_sprite_storage_flip_mode_set(fck_sprite_storage *storage, SDL_FlipMode flip_mode)
{
	SDL_assert(storage != nullptr);
	storage->flip_mode = flip_mode;
}

void fck_sprite_storage_set(fck_sprite_storage *storage, size_t at, fck_sprite const *sprite)
{
	SDL_assert(storage != nullptr);
	SDL_assert(at < storage->count);

	storage->sprites[at] = *sprite;
}

fck_sprite const *fck_sprite_storage_get(fck_sprite_storage *storage, size_t at)
{
	SDL_assert(storage != nullptr);
	SDL_assert(at < storage->count);

	return &storage->sprites[at];
}

void fck_sprite_storage_draw(fck_sprite_storage *storage, size_t at, SDL_FRect const *dst)
{
	SDL_assert(storage != nullptr);

	SDL_Renderer *renderer = SDL_GetRendererFromTexture(storage->texture);
	CHECK_ERROR(renderer != nullptr, SDL_GetError(), return);

	fck_sprite const *sprite = fck_sprite_storage_get(storage, at);

	CHECK_ERROR(SDL_RenderTextureRotated(renderer, storage->texture, &sprite->src, dst, 0.0, nullptr, storage->flip_mode), SDL_GetError());
}

void fck_sprite_storage_free(fck_sprite_storage *storage)
{
	SDL_free(storage->sprites);
	storage->texture = nullptr;
	storage->sprites = nullptr;
	storage->count = 0;
}

enum fck_frog_animation_type
{
	FCK_FROG_ANIMATION_TYPE_IDLE,
	FCK_FROG_ANIMATION_TYPE_MOVE_LEFT,
	FCK_FROG_ANIMATION_TYPE_MOVE_RIGHT,
	FCK_FROG_ANIMATION_TYPE_COUNT
};

struct fck_anim
{
	size_t frame_start;
	size_t frame_count;

	float frame_time;
};

struct fck_anim_storage_resource
{
	fck_anim animations[FCK_FROG_ANIMATION_TYPE_COUNT];
};

struct fck_anim_storage
{
	fck_sprite_storage *sprite_storage;
	fck_anim animations[FCK_FROG_ANIMATION_TYPE_COUNT];
	fck_frog_animation_type current_animation;

	size_t current_frame_index;

	float accumulator;
};

void fck_anim_storage_to_resource(fck_anim_storage *anim_storage, fck_anim_storage_resource *resource)
{
	SDL_assert(anim_storage != nullptr);
	SDL_assert(resource != nullptr);

	SDL_memcpy(resource->animations, anim_storage->animations, sizeof(resource->animations));
}

void fck_anim_storage_alloc(fck_anim_storage *anim_storage, fck_sprite_storage *sprite_storage)
{
	SDL_assert(anim_storage != nullptr);

	SDL_zerop(anim_storage);
	anim_storage->sprite_storage = sprite_storage;
	anim_storage->current_animation = FCK_FROG_ANIMATION_TYPE_IDLE;
	anim_storage->accumulator = 0.0f;
}

void fck_anim_storage_set(fck_anim_storage *anim_storage, fck_frog_animation_type type, size_t start, size_t count, float frame_time)
{
	SDL_assert(anim_storage != nullptr);
	SDL_assert(type < FCK_FROG_ANIMATION_TYPE_COUNT && type >= 0);

	fck_anim *anim = &anim_storage->animations[type];
	anim->frame_start = start;
	anim->frame_count = count;
	anim->frame_time = frame_time;
}

fck_anim const *fck_anim_storage_get(fck_anim_storage *anim_storage, fck_frog_animation_type type)
{
	SDL_assert(anim_storage != nullptr);
	SDL_assert(type < FCK_FROG_ANIMATION_TYPE_COUNT && type >= 0);

	return &anim_storage->animations[type];
}

fck_anim const *fck_anim_storage_current_get(fck_anim_storage *anim_storage)
{
	SDL_assert(anim_storage != nullptr);

	return &anim_storage->animations[anim_storage->current_animation];
}

void fck_anim_storage_current_set(fck_anim_storage *anim_storage, fck_frog_animation_type type)
{
	SDL_assert(anim_storage != nullptr);
	anim_storage->current_animation = type;
}

void fck_anim_storage_update(fck_anim_storage *anim_storage, float delta_time /*seconds*/)
{
	SDL_assert(anim_storage != nullptr);

	anim_storage->accumulator += delta_time;

	fck_anim const *anim = fck_anim_storage_current_get(anim_storage);

	if (anim_storage->accumulator > anim->frame_time)
	{
		anim_storage->accumulator = 0.0f;
		anim_storage->current_frame_index = anim_storage->current_frame_index + 1;
		if (anim_storage->current_frame_index >= anim->frame_count)
		{
			anim_storage->current_frame_index = 0;
		}
	}
}

void fck_anim_storage_draw(fck_anim_storage *anim_storage, SDL_FRect const *dst)
{
	SDL_assert(anim_storage != nullptr);

	fck_anim const *anim = fck_anim_storage_current_get(anim_storage);
	size_t sprite_index = anim_storage->current_frame_index + anim->frame_start;

	fck_sprite_storage_draw(anim_storage->sprite_storage, sprite_index, dst);
}

void fck_anim_storage_free(fck_anim_storage *anim_storage)
{
	SDL_assert(anim_storage != nullptr);
	// might get used later - but we have a safety check ;)
}

int fck_run_student_testbed(int, char **)
{
	CHECK_CRITICAL(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS), SDL_GetError());

	CHECK_CRITICAL(IMG_Init(IMG_INIT_PNG), SDL_GetError());

	SDL_Window *window = SDL_CreateWindow("Fck - Engine", 640, 640, 0);
	CHECK_CRITICAL(window != nullptr, SDL_GetError());

	SDL_Renderer *renderer = SDL_CreateRenderer(window, nullptr);
	CHECK_CRITICAL(renderer != nullptr, SDL_GetError());

	SDL_Texture *player_texture = IMG_LoadTexture(renderer, FCK_RESOURCE_DIRECTORY_PATH "player.png");
	CHECK_CRITICAL(player_texture != nullptr, SDL_GetError());

	float tw;
	float th;
	CHECK_CRITICAL(SDL_GetTextureSize(player_texture, &tw, &th), SDL_GetError());

	const int sprite_count = 5;

	fck_keyboard_state keyboard;
	SDL_zero(keyboard);

	fck_sprite_storage sprite_storage;
	fck_sprite_storage_alloc(&sprite_storage, player_texture, sprite_count);
	fck_sprite_storage_flip_mode_set(&sprite_storage, SDL_FLIP_VERTICAL);

	fck_anim_storage anim_storage;
	fck_anim_storage_alloc(&anim_storage, &sprite_storage);

	float sprite_size = tw / sprite_count;
	for (int index = 0; index < sprite_count; index++)
	{
		float sprite_x = sprite_size * index;
		fck_sprite sprite;
		sprite.src.x = sprite_x;
		sprite.src.y = 0.0f;
		sprite.src.w = sprite_size;
		sprite.src.h = th;
		fck_sprite_storage_set(&sprite_storage, index, &sprite);
	}

	if (fck_file_exists("", "animation_data", ".anim"))
	{
		fck_file_memory file_mem;
		if (fck_file_read("", "animation_data", ".anim", &file_mem))
		{
			fck_anim_storage_resource *anim_in_memory = (fck_anim_storage_resource *)file_mem.data;
			SDL_memcpy(&anim_storage.animations, anim_in_memory, sizeof(anim_storage.animations));
		}
	}
	else
	{
		fck_anim_storage_set(&anim_storage, FCK_FROG_ANIMATION_TYPE_IDLE, 0, 1, 0.25f);
		fck_anim_storage_set(&anim_storage, FCK_FROG_ANIMATION_TYPE_MOVE_LEFT, 1, 2, 0.25f);
		fck_anim_storage_set(&anim_storage, FCK_FROG_ANIMATION_TYPE_MOVE_RIGHT, 3, 2, 0.25f);

		fck_anim_storage_resource resource;
		fck_anim_storage_to_resource(&anim_storage, &resource);
		fck_file_write("", "animation_data", ".anim", &resource, sizeof(resource));
	}

	float x = 0;
	float y = 640 - 256.0f;

	Uint64 tp = SDL_GetTicks();

	const float animation_frame_time = 0.25f;
	float animation_frame_accumulator = 0.0f;
	size_t animation_frame_index = 0;

	float dash_cooldown_duration = 1.0f;
	float dash_cooldown_timer = 0.0f;

	bool is_running = true;
	while (is_running)
	{
		Uint64 now = SDL_GetTicks();
		Uint64 delta = now - tp;
		float delta_seconds = float(delta) / 1000.0f;

		tp = now;

		SDL_Event event;
		while (SDL_PollEvent(&event))
		{
			switch (event.type)
			{
			case SDL_EVENT_QUIT: {
				is_running = false;
				break;
			}
			case SDL_EVENT_KEY_DOWN:
			case SDL_EVENT_KEY_UP: {
				break;
			}
			default:
				break;
			}
		}

		fck_keyboard_state_update(&keyboard);

		// Update
		float direction = 0.0f;
		if (fck_key_down(&keyboard, SDL_SCANCODE_D))
		{
			fck_anim_storage_current_set(&anim_storage, FCK_FROG_ANIMATION_TYPE_MOVE_RIGHT);
			direction = direction + 0.5f;
		}
		if (fck_key_down(&keyboard, SDL_SCANCODE_A))
		{
			fck_anim_storage_current_set(&anim_storage, FCK_FROG_ANIMATION_TYPE_MOVE_LEFT);
			direction = direction - 0.5f;
		}
		if (direction == 0.0f)
		{
			fck_anim_storage_current_set(&anim_storage, FCK_FROG_ANIMATION_TYPE_IDLE);
		}
		x = x + direction;

		dash_cooldown_timer -= delta_seconds;
		dash_cooldown_timer = SDL_max(dash_cooldown_timer, 0.0f);
		if (dash_cooldown_timer <= 0.0f)
		{
			if (fck_key_just_down(&keyboard, SDL_SCANCODE_SPACE))
			{
				y = y - 64.0f;
				dash_cooldown_timer = dash_cooldown_duration;
			}
		}

		animation_frame_accumulator += delta_seconds;
		if (animation_frame_accumulator > animation_frame_time)
		{
			animation_frame_accumulator = 0.0f;
			animation_frame_index = animation_frame_index + 1;
			if (animation_frame_index >= sprite_storage.count)
			{
				animation_frame_index = 0;
			}
		}
		fck_anim_storage_update(&anim_storage, delta_seconds);
		// Render

		// Clear the final image
		SDL_SetRenderDrawColor(renderer, 0, 0, 20, 255);
		SDL_RenderClear(renderer);

		// Construct the final
		SDL_SetRenderDrawColor(renderer, 50, 175, 20, 255);

		fck_anim const *anim = fck_anim_storage_current_get(&anim_storage);
		size_t sprite_index = anim_storage.current_frame_index + anim->frame_start;

		SDL_FRect dst{x, y, 48.0f * 2.0f, 48.0f * 2.0f};
		fck_anim_storage_draw(&anim_storage, &dst);

		// Present final image
		SDL_RenderPresent(renderer);
	}

	fck_anim_storage_free(&anim_storage);

	fck_sprite_storage_free(&sprite_storage);

	SDL_Quit();
	return 0;
}