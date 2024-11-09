#ifndef U_CONFIG_29
#define U_CONFIG_29

#include "u_config_def.h"
#include "context.h"

void u_config_defaults(UConfig *pConfig);

int u_config_gather_graphics_cards(const Context *const context, UConfig *pConfig);

int u_config_gather_limits(const Context *const context, UConfig *pConfig);

int u_config_load(UConfig *pConfig);

int u_config_save(const UConfig *const pConfig);

#endif // U_CONFIG_29
