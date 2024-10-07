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

typedef struct UMazeLink {
    UMazeVertex *pVertexLink[2];
} UMazeLink;

typedef enum UMazeGenFlags {
    U_MAZE_GEN_LINKS    = 0x1,
    U_MAZE_GEN_VERTEXES = 0x2
} UMazeGenFlags;

typedef struct UMazeData {
    size_t vertexAmount;
    UMazeVertex *pVertices;
    size_t connectionAmount;
    UMazeVertex **ppConnections;
} UMazeData;

typedef struct UMazeGenResult {
    UMazeData *pSource; // Reference

    size_t linkAmount; // Can be zero.
    UMazeLink *pLinks; // Can be NULL.

    UMazeData *pVertexMazeData; // Can be NULL.
} UMazeGenResult;

#endif // U_MAZE_DEF_29
