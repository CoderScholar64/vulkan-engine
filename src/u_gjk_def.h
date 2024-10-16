#ifndef U_GJK_DEF_29
#define U_GJK_DEF_29

#include "raymath.h"

#include <stddef.h>

#define U_COLLISION_SIMPLEX_VERTEX_AMOUNT 4

typedef struct UCollisionPolyhedron {
    size_t amountVertices;
    Vector3 vertices[];
} UCollisionPolyhedron;

typedef struct UCollisionSphere {
    Vector3 position;
    float radius;
} UCollisionSphere;

typedef enum UCollisionState {
    U_COLLISION_FALSE,
    U_COLLISION_NOT_DETERMINED,
    U_COLLISION_TRUE
} UCollisionState;

typedef struct UGJKMetaData {
    struct {
        size_t amountVertices;
        Vector3 vertices[U_COLLISION_SIMPLEX_VERTEX_AMOUNT];
    } simplex;
    Vector3 direction;
    Vector3 support;
    unsigned countDown;
} UGJKMetaData;

typedef struct UGJKReturn {
    Vector3 normal;
    float distance;
    UCollisionState result; // normal is only valid if this is set to COLLISION.
} UGJKReturn;

typedef struct UGJKBackoutEdge {
    size_t edgeIndexes[2];
} UGJKBackoutEdge;

typedef struct UGJKBackoutTriangle {
    Vector3 normal;
    float distance;
    size_t vertexIndexes[3];
} UGJKBackoutTriangle;

typedef struct UGJKBackoutCache {
    size_t            edgeAmount;
    size_t            edgeLimit;
    UGJKBackoutEdge *pEdges;

    size_t                faceAmount;
    size_t                faceLimit;
    UGJKBackoutTriangle *pFaces;

    size_t                newFaceAmount;
    size_t                newFaceLimit;
    UGJKBackoutTriangle *pNewFaces;

    size_t   vertexAmount;
    size_t   vertexLimit;
    Vector3 *pVertices;
} UGJKBackoutCache;

#define U_GJK_EMPTY_POLYGON(name, count)\
    UCollisionPolyhedron name = {count}

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
