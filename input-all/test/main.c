#include "fck_input.h"
#include "fck_os.h"
#include "fckc_apidef.h"

#include "fck_gamepad_input.h"
#include "fck_mouse.h"
#include "fck_pkey.h"

#include "fck_text_input.h"
#include "fckc_assert.h"

int main(int argc, char **argv)
{
	FCK_IMPORT_API fck_input *fck_input_all_load(void);

	fck_input *input = fck_input_all_load();

	fck_input_source *mouse = NULL;
	fck_input_source *keyboard = NULL;
	fck_input_source *text = NULL;
	fck_input_source *dualsense = NULL;

	fck_window window = os->win->create("Test", 420, 640);
	// os->win->text_input_start(window);

	fck_input_source **sources;
	fckc_size_t result = input->sources(&sources);

	for (fckc_size_t index = 0; index < result; index++)
	{
		fck_input_source *source = sources[index];
		if (input->is(source, "physical-keyboard"))
		{
			os->io->log("Found: %s", source->name);
			keyboard = source;
		}
		if (input->is(source, "mouse"))
		{
			os->io->log("Found: %s", source->name);
			mouse = source;
		}
		if (input->is(source, "text"))
		{
			os->io->log("Found: %s", source->name);
			text = source;
		}
		if (input->is(source, "dualsense"))
		{
			os->io->log("Found: %s", source->name);
			dualsense = source;
		}
	}
	fck_assert(keyboard);
	fck_assert(dualsense);

	for (;;)
	{
		fck_input_event events[32] = {0};
		int iteration = 0;
		// for (;;)
		{
			iteration = iteration + 1;
			fckc_size_t result = input->events(events, fck_arraysize(events));
			for (fckc_size_t index = 0; index < result; index++)
			{
				fck_input_event *e = events + index;
				os->io->log("%s - %llu - %u \t %s - %s: %f %f", e->source->name, e->owner, e->description->id, e->description->name,
				                fck_input_data_type_to_string(e->description->data_type), e->data.floats[0], e->data.floats[1]);
				/*if (e->source == keyboard)
				{
				    switch (e->description->id)
				    {
				    case fck_pkey_a:
				        os->io->log("Event Left");
				        break;
				    case fck_pkey_d:
				        os->io->log("Event Right");
				        break;
				    case fck_pkey_w:
				        os->io->log("Event Up");
				        break;
				    case fck_pkey_s:
				        os->io->log("Event Down");
				        break;
				    }
				}*/
			}
			// if (result == 0)
			//{
			//	break;
			// }
		}

		if (iteration > 1)
		{
			// Hm, we pump too aggressively
			os->io->log("Iterations: %d", iteration);
		}

		fckc_u32 ids[] = {fck_gamepad_dpad_left, fck_gamepad_dpad_right, fck_gamepad_dpad_up, fck_gamepad_dpad_down};
		fck_input_data states[fck_arraysize(ids)];

		fckc_u64 *owners;
		fckc_size_t owner_count = dualsense->owners(&owners);
		for (fckc_size_t index = 0; index < owner_count; index++)
		{
			fckc_u64 owner = owners[index];
			dualsense->states(owner, ids, states, fck_arraysize(states));
			if (states[0].scalar > 0.0f)
			{
				os->io->log("State Left");
			}
			if (states[1].scalar > 0.0f)
			{
				os->io->log("State Right");
			}
			if (states[2].scalar > 0.0f)
			{
				os->io->log("State Up");
			}
			if (states[3].scalar > 0.0f)
			{
				os->io->log("State Down");
			}
		}

		// fckc_u32 ids[] = {fck_pkey_a, fck_pkey_d, fck_pkey_w, fck_pkey_s};
		// fck_input_data states[fck_arraysize(ids)];

		// keyboard->states(0, ids, states, fck_arraysize(states));
		// if (states[0].as_scalar > 0.0f)
		//{
		//	os->io->log("State Left");
		// }
		// if (states[1].as_scalar > 0.0f)
		//{
		//	os->io->log("State Right");
		// }
		// if (states[2].as_scalar > 0.0f)
		//{
		//	os->io->log("State Up");
		// }
		// if (states[3].as_scalar > 0.0f)
		//{
		//	os->io->log("State Down");
		// }
	}

	return 0;
}
