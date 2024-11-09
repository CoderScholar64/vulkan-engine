#ifndef U_CONFIG_DEF_29
#define U_CONFIG_DEF_29

typedef struct UConfigParameters {
    int width;
    int height;
    int sample_count;
    int graphics_card_index;
    int pixel_format_index;
} UConfigParameters;

typedef struct UConfig {
    UConfigParameters min;
    UConfigParameters current;
    UConfigParameters max;
} UConfig;

#endif // U_CONFIG_DEF_29
