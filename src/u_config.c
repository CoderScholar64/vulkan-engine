#include "u_config.h"

void u_config_defaults(UConfig *this) {
    this->current.width = 1024;
    this->current.height = 764;
    this->current.sample_count = 1;
    this->current.graphics_card_index = -1;
    this->current.pixel_format_index  = -1;
}

int u_config_gather_vulkan_devices(UConfig *this, const Context *const pContext) {
    //
}

int u_config_gather_vulkan_limits(UConfig *this, const Context *const pContext) {
    //
}

int u_config_load(UConfig *this, const char *const pUTF8Filepath) {
    //
}

int u_config_save(const UConfig *const this, const char *const pUTF8Filepath) {
    //
}
