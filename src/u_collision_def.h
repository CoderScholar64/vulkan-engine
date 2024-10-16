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

typedef struct UCollisionGJK {
    struct {
        size_t amountVertices;
        Vector3 vertices[U_COLLISION_SIMPLEX_VERTEX_AMOUNT];
    } simplex;
    Vector3 direction;
    Vector3 support;
    unsigned countDown;
} UCollisionGJK;

typedef struct UCollisionReturn {
    Vector3 normal;
    float distance;
    UCollisionState result; // normal is only valid if this is set to COLLISION.
} UCollisionReturn;

typedef struct UCollisionEPAEdge {
    size_t vertexIndexes[2];
} UCollisionEPAEdge;

typedef struct UCollisionEPATriangle {
    Vector3 normal;
    float distance;
    size_t vertexIndexes[3];
} UCollisionEPATriangle;

typedef struct UCollisionBackoutCache {
    size_t              edgeAmount;
    size_t              edgeLimit;
    UCollisionEPAEdge *pEdges;

    size_t                 faceAmount;
    size_t                 faceLimit;
    UCollisionEPATriangle *pFaces;

    size_t                 newFaceAmount;
    size_t                 newFaceLimit;
    UCollisionEPATriangle *pNewFaces;

    size_t   vertexAmount;
    size_t   vertexLimit;
    Vector3 *pVertices;
} UCollisionBackoutCache;

#endif // U_GJK_DEF_29
