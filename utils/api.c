
#include <stdlib.h>
#include "load-tool.h"

void *get_api(char *name)
{
	return load_tool_old(NULL, name);
}
