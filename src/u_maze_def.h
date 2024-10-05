#ifndef U_MAZE_DEF_29
#define U_MAZE_DEF_29

#include "v_raymath.h"

#include <stddef.h>
#include <stdint.h>

typedef struct UMazeVertexMetaData {
    struct {
        uint32_t isVisited :  1;
        uint32_t count :     31;
    } data;
    Vector2 position;
} UMazeVertexMetaData;

typedef struct UMazeVertex {
    UMazeVertexMetaData metadata;
    struct UMazeVertex **ppConnections;
} UMazeVertex;

typedef struct UMazeConnection {
    UMazeVertex *pConnections[2];
} UMazeConnection;

typedef struct UMazeData {
    size_t vertexAmount;
    UMazeVertex *pVertices;
    size_t connectionAmount;
    UMazeVertex **ppConnections;
} UMazeData;


#endif // U_MAZE_DEF_29
