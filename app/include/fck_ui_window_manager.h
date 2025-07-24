#ifndef FCK_UI_WINDOW_MANAGER_H_INCLUDED
#define FCK_UI_WINDOW_MANAGER_H_INCLUDED

#include <fckc_inttypes.h>

typedef struct fck_ui fck_ui;
typedef struct fck_ui_window fck_ui_window;
typedef struct fck_ui_window_manager fck_ui_window_manager;

typedef enum fck_ui_window_manager_text_input_signal_type
{
	FCK_UI_WINDOW_MANAGER_TEXT_INPUT_SIGNAL_NONE,
	FCK_UI_WINDOW_MANAGER_TEXT_INPUT_SIGNAL_START,
	FCK_UI_WINDOW_MANAGER_TEXT_INPUT_SIGNAL_STOP,
} fck_ui_window_manager_text_input_signal_type;


typedef fckc_u32 nk_flags;
typedef int (*fck_ui_user_window_draw_content_function)(struct fck_ui* ui, fck_ui_window* window, void* userdata);

typedef struct fck_ui_window
{
	float x;
	float y;
	float w;
	float h;
	nk_flags flags;
	const char* title;
	void* userdata;
	fck_ui_user_window_draw_content_function on_content;
} fck_ui_window;

// Window Management related
fck_ui_window_manager* fck_ui_window_manager_alloc(fckc_size_t capacity);
void fck_ui_window_manager_free(fck_ui_window_manager* manager);

fck_ui_window* fck_ui_window_manager_create(fck_ui_window_manager* manager, const char* title, void* userdata,
	fck_ui_user_window_draw_content_function on_content);

fck_ui_window* fck_ui_window_manager_view(fck_ui_window_manager* manager, fckc_size_t index);
fckc_size_t fck_ui_window_manager_count(fck_ui_window_manager* manager);

// UI related
fck_ui_window_manager_text_input_signal_type fck_ui_window_manager_query_text_input_signal(fck_ui* ui, fck_ui_window_manager* window_manager);
void fck_ui_window_manager_tick(fck_ui* ui, fck_ui_window_manager* manager, int x, int y, int w, int h);

#endif // !FCK_UI_WINDOW_MANAGER_H_INCLUDED
