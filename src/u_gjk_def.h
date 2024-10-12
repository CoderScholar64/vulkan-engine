#ifndef U_GJK_DEF_29
#define U_GJK_DEF_29

#include "raymath.h"

#include <stddef.h>

#define U_GJK_SIMPLEX_VERTEX_AMOUNT 4

typedef struct UGJKPolyhedron {
    size_t amountVertices;
    Vector3 vertices[];
} UGJKPolyhedron;

typedef enum UGJKState {
    U_GJK_NO_COLLISION,
    U_GJK_NOT_DETERMINED,
    U_GJK_COLLISION
} UGJKState;

typedef struct UGJKMetaData {
    const UGJKPolyhedron *pConvexShape[2];
    struct {
        size_t amountVertices;
        Vector3 vertices[U_GJK_SIMPLEX_VERTEX_AMOUNT];
    } simplex;
    Vector3 direction;
    Vector3 support;
    unsigned countDown;
} UGJKMetaData;

typedef struct UGJKReturn {
    Vector4 normal; // [x, y, z, depth]
    UGJKState result; // normal is only valid if this is set to COLLISION.
} UGJKReturn;

typedef struct UGJKBackoutEdge {
    size_t edgeIndexes[2];
} UGJKBackoutEdge;

typedef struct UGJKBackoutTriangle {
    size_t vertexIndexes[3];
} UGJKBackoutTriangle;

typedef struct UGJKBackoutCache {
    size_t    edgeAmount;
    size_t    edgeLimit;
    size_t    faceAmount;
    size_t    faceLimit;
    size_t newFaceAmount;
    size_t newFaceLimit;
    size_t  normalAmount;
    size_t  normalLimit;
    size_t  vertexAmount;
    size_t  vertexLimit;

    UGJKBackoutEdge     *pEdges;
    UGJKBackoutTriangle *pFaces;
    Vector3             *pVertices;
    Vector4             *pNormals;
    UGJKBackoutTriangle *pNewFaces;
    Vector4             *pNewNormals;
} UGJKBackoutCache;

#define U_GJK_EMPTY_POLYGON(name, count)\
    UGJKPolyhedron name = {count}

#define U_GJK_BOX(a_x, a_y, a_z, b_x, b_y, b_z) {\
    8, {\
    {a_x, a_y, a_z},\
    {a_x, a_y, b_z},\
    {a_x, b_y, a_z},\
    {a_x, b_y, b_z},\
    {b_x, a_y, a_z},\
    {b_x, a_y, b_z},\
    {b_x, b_y, a_z},\
    {b_x, b_y, b_z}\
    }}

#endif // U_GJK_DEF_29
