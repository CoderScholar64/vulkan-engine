#ifndef U_MAZE_DEF_29
#define U_MAZE_DEF_29

#include "v_raymath.h"

#include <stddef.h>
#include <stdint.h>

typedef struct UMazeVertexMetaData {
    Vector2 position;
} UMazeVertexMetaData;

typedef struct UMazeVertex {
    UMazeVertexMetaData metadata;
    size_t linkAmount;
    struct UMazeVertex **ppVertexLinks;
} UMazeVertex;

typedef struct UMazeLink {
    UMazeVertex *pVertexLink[2];
} UMazeLink;

typedef struct UMazeData {
    size_t vertexAmount;
    UMazeVertex *pVertices;
    size_t linkAmount;
    UMazeVertex **ppVertexLinks;
} UMazeData;

typedef struct UMazeGenResult {
    const UMazeData *pSource; // Reference

    size_t linkAmount; // Can be zero.
    UMazeLink *pLinks; // Can be NULL.

    UMazeData vertexMazeData; // Can be empty
} UMazeGenResult;

#endif // U_MAZE_DEF_29
