
#include <fck_os.h>
#include <stdio.h>

static char *dashes_to_underscores(char *str, fckc_size_t length)
{
	for (fckc_size_t index = 0; index < length; index++)
	{
		if (str[index] == '-')
		{
			str[index] = '_';
		}
	}
	return str;
}

int main(int argc, char **argv)
{
	char **paths;
	fckc_size_t count = os->glob->executable("", "*", &paths);
	for (fckc_size_t index = 0; index < count; index++)
	{
		char *path = paths[index];
		char *api = os->glob->match(path, "fck-*.dll");
		if (api)
		{
			fck_shared_object so = os->so->load(path);
			if (os->so->is_valid(so))
			{
				char *extension = os->glob->find(path, ".dll");
				fckc_size_t length = (fckc_size_t)extension - (fckc_size_t)path;

				char buffer[1024];
				int result = snprintf(buffer, sizeof(buffer), "%.*s", (int)length, path);
				char* loadable = dashes_to_underscores(buffer, length);

				os->io->log("%s: %.*s", path, result, loadable);
				os->so->unload(so);
			}
		}
	}
	return 0;
}
