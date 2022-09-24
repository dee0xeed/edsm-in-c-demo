
#ifndef STATE_MACHINE_DESC_H_FOR_ENGINE_ONLY
#define STATE_MACHINE_DESC_H_FOR_ENGINE_ONLY

#include "engine/smd.h"

struct edsm_desc *smd_load_desc(char *dir, char *file);
struct state *smd_get_state_set(struct edsm_desc *md);

// api?
#endif
