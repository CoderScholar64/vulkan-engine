#ifndef V_INIT_29
#define V_INIT_29

#include "v_results.h"
#include "context.h"

/**
 * Initialize the vulkan renderer.
 * @warning This function does not clean up the previous vulkan renderer. It will cause dangling pointer issues if v_deinit is not used.
 * @return A VEngineResult. If its type is VE_SUCCESS then this function did not detect any problems during the creation of Vulkan.
 */
VEngineResult v_init_alloc(Context *this);

/**
 * Deallocate the vulkan renderer.
 * @warning Do not use this if v_init() is not called yet. This might cause a crash.
 */
void v_init_dealloc(Context *this);

/**
 * Recreate the swap chain which v_init() had created.
 * @warning Make sure that v_init() is called first before using this method.
 * @return A VEngineResult. If its type is VE_SUCCESS then the swap chain recreation had happened. Otherwise it would return various errors.
 */
VEngineResult v_init_recreate_swap_chain(Context *this);

#endif // V_INIT_29
