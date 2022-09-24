
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

#include <dlfcn.h>
#include "logging.h"

void *_do_load_tool(char *dllpath, char *symname)
{
	void *dll;
	void *tool;
	char *errstr;

	dll = dlopen(dllpath, RTLD_LAZY);
	if (!dll) {
		log_sys_msg("%s() - dlopen('%s'): %s\n", __func__, dllpath, dlerror());
		return NULL;
	};

	dlerror(); /* clear errors */

	tool = dlsym(dll, symname);
	errstr = dlerror();
	if (errstr) {
		log_sys_msg("%s() - dlsym('%s'): %s\n", __func__, symname, errstr);
		return NULL;
	}
	return tool;
}


void *load_tool_old(char *dllpath, char *symname)
{
	return _do_load_tool(dllpath, symname);
}

void *load_tool(char *dir, char *name, char *symname)
{
	int l;
	char *lib;
	void *tool;

	if (!dir) {
		log_bug_msg("%s() - NULL 'dir' in load_tool\n", __func__);
		return NULL;
	}

	if (!name) {
		log_bug_msg("%s() - NULL 'name' in load_tool\n", __func__);
		return NULL;
	}

	if (!symname) {
		log_bug_msg("%s() - NULL 'symname' in load_tool\n", __func__);
		return NULL;
	}

	l = 1 + strlen(dir) + strlen(name);
	lib = malloc(l);
	if (!lib) {
		log_sys_msg("%s() - malloc('%s'): %s\n", __func__, name, strerror(errno));
		return NULL;
	}

	memset(lib, 0, l);
	strcat(lib, dir);
	strcat(lib, name);

	log_dbg_msg("%s() - loading '%s' tool from '%s'\n", __func__, symname, lib);
	tool = _do_load_tool(lib, symname);
	free(lib);
	return tool;
}
