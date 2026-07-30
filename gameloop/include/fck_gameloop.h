#ifndef FCK_GAMELOOP_H_INCLUDED
#define FCK_GAMELOOP_H_INCLUDED

#define fck_gameloop_interface_name "fck-gameloop"

struct fck_api_registry;
struct fck_ec;
struct kll_allocator;
struct fck_ec_api;
struct fck_nk;
struct fck_nuklear_api;

struct fck_sprite_api;
struct fck_sprites;

typedef struct fck_gameloop
{
	void *handle;
} fck_gameloop;

// The gameloop should stay submissive
// It should not do too much, instead it should work with APIs
// Primarily it needs to know the following things:
// - Where to get APIs and features from
// - Where the state of everything lives
// - The not solved part, API + Object, how should they get bundled up together?

typedef struct fck_gameloop_create_parameters
{
	struct fck_api_registry *apis;
	struct fck_ec *state;

	struct fck_ec_api* ec;
} fck_gameloop_create_parameters;

typedef struct fck_gameloop_destroy_parameters
{
	struct fck_api_registry *apis;
	struct fck_ec *state;

	struct fck_ec_api* ec;
} fck_gameloop_destroy_parameters;

typedef struct fck_gameloop_edit_parameters
{
	struct fck_api_registry *apis;
	struct fck_ec *state;
	struct fck_nk *view;

	// Maybe pass down the parts of fck_nk that are actually useable? 
	// I.e, panels, elements, etc.
	struct fck_nuklear_api *nk;
	struct fck_ec_api *ec;
} fck_gameloop_edit_parameters;

typedef struct fck_gameloop_tick_parameters
{
	struct fck_api_registry *apis;
	struct fck_ec *state;
	struct fck_sprites *sprites;

	struct fck_ec_api *ec;
} fck_gameloop_tick_parameters;

typedef struct fck_gameloop_interface
{
	const char *name;
	// TODO: Think about frame allocation and all that...
	fck_gameloop (*create)(struct kll_allocator *allocator, const fck_gameloop_create_parameters *params);
	void (*destroy)(fck_gameloop loop, const fck_gameloop_destroy_parameters *params);

	int (*edit)(fck_gameloop loop, const fck_gameloop_edit_parameters *params);
	int (*tick)(fck_gameloop loop, const fck_gameloop_tick_parameters *params);
} fck_gameloop_interface;

#endif // !FCK_GAMELOOP_H_INCLUDED