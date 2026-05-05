

#include "fck_input.h"
#include "fck_os.h"
#include "fckc_apidef.h"

FCK_IMPORT_API fck_input_source *input_source_mouse;
FCK_IMPORT_API fck_input_source* input_source_keyboard;
FCK_IMPORT_API fck_input *input_api;

int main(int argc, char **argv)
{
	input_api->add(input_source_mouse);
	input_api->add(input_source_keyboard);

	fck_window window = os->win->create("Test", 420, 640);

	for (;;)
	{
		fck_input_event events[32] = {0};
		fckc_size_t result = input_api->events(events, fck_arraysize(events));
		for (fckc_size_t index = 0; index < result; index++)
		{
			fck_input_event *e = events + index;
			{
				os->io->log("%s - %llu - %u \t %s - %s: %f %f", e->source->name, e->owner, e->description->id, e->description->name,
				            fck_input_data_type_to_string(e->description->data_type), e->data.as_floats[0], e->data.as_floats[1]);
			}
		}
	}

	return 0;
}