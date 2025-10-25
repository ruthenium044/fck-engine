#include <fckc_apidef.h>
#include <stdio.h>

FCK_EXPORT_API int fck_main()
{
	printf("%s loaded and initialised\n", __FILE_NAME__);
	return 0;
}
