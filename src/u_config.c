#include "u_config.h"

void u_config_defaults(UConfig *pConfig) {
    pConfig->current.width = 1024;
    pConfig->current.height = 764;
    pConfig->current.sample_count = 1;
    pConfig->current.graphics_card_index = -1;
    pConfig->current.pixel_format_index  = -1;
}

int u_config_gather_graphics_cards(const Context *const context, UConfig *pConfig) {
    //
}

int u_config_gather_limits(const Context *const context, UConfig *pConfig) {
    //
}

int u_config_load(UConfig *pConfig) {
    //
}

int u_config_save(const UConfig *const pConfig) {
    //
}
