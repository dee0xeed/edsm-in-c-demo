
#ifndef API_H
#define API_H

void *get_api(char *name);

#define api_declaration(stag) struct stag
#define api_definition(stag,var) struct stag var =

#endif
