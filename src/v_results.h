#ifndef V_RESULT_29
#define V_RESULT_29

typedef enum {
    VE_SUCCESS                       =   1,
    VE_TIME_OUT                      =   0,
    VE_NOT_COMPATIBLE                =  -1,
    VE_SURFACE_FAILURE               =  -2,
    VE_QUERY_SWAP_CHAIN_FAILURE      =  -3,
    VE_INIT_INSTANCE_FAILURE         =  -4,
    VE_FIND_PHYSICAL_DEVICE_FAILURE  =  -5,
    VE_ALLOC_LOGICAL_DEVICE_FAILURE  =  -6,
    VE_RECORD_COMMAND_BUFFER_FAILURE =  -7,
    VE_ALLOC_MEMORY_V_BUFFER_FAILURE =  -8,
    VE_DRAW_FRAME_FAILURE            =  -9,
    VE_ALLOC_SWAP_CHAIN_FAILURE      = -10,
    VE_ALLOC_SWAP_CHAIN_I_V_FAILURE  = -11,
    VE_CREATE_RENDER_PASS_FAILURE    = -12,
    VE_ALLOC_GRAPH_PIPELINE_FAILURE  = -13,
    VE_ALLOC_FRAME_BUFFERS_FAILURE   = -14,
    VE_ALLOC_COMMAND_POOL_FAILURE    = -15,
    VE_CREATE_COMMAND_BUFFER_FAILURE = -16,
    VE_ALLOC_SYNC_OBJECTS_FAILURE    = -17,
    VE_COPY_BUFFER_FAILURE           = -18,
    VE_ALLOC_STATIC_BUFFER           = -19,
    VE_DESCRIPTOR_SET_LAYOUT_FAILURE = -20
} VEngineResultType;

typedef struct {
    VEngineResultType type;
    unsigned int      point;
} VEngineResult;

#define RETURN_RESULT_CODE(TYPE, POINT)\
{\
    VEngineResult result;\
    result.type  =  TYPE;\
    result.point = POINT;\
    return result;\
}


#endif // V_RESULT_29
