#include "u_gjk.h"

#include <assert.h>
#include <float.h>
#include <stdlib.h>

#define U_GJK_MAX_RESOLVE 8

static Vector3 polyhedronFindFurthestPoint(const UGJKPolyhedron *pPolygon, Vector3 direction);
static Vector3 sphereFindFurthestPoint(const UGJKSphere *pSphere, Vector3 direction);

static int gjkModifySimplex(UGJKMetaData *this);
static int gjkLine(UGJKMetaData *this);
static int gjkTriangle(UGJKMetaData *this);
static int gjkTetrahedron(UGJKMetaData *this);
static void epaGetFaceNormals(const Vector3 *pPolyTope, size_t polyTopeAmount, UGJKBackoutTriangle *pFaces, size_t facesAmount, size_t *pMinTriangleIndex);
static int epaAddIfUniqueEdge(const UGJKBackoutTriangle *pFaces, size_t facesAmount, size_t faceIndex, unsigned a, unsigned b, UGJKBackoutEdge *pEdges, size_t *pEdgeAmount, size_t edgeMax);

static inline int sameDirection(Vector3 direction, Vector3 ao) { return Vector3DotProduct(direction, ao) > 0.0f; }

#define U_GJK_IMPLEMENTATION(maxResolve, shape0SupportFunction, shape1SupportFunction) \
    UGJKReturn gjkReturn = {0};\
    gjkReturn.result = U_GJK_NO_COLLISION;\
\
    UGJKMetaData gjkMetadata = {0};\
\
    gjkMetadata.simplex.amountVertices = 1;\
    gjkMetadata.simplex.vertices[0] = Vector3Subtract(shape0SupportFunction(pShape0, (Vector3){0, 1, 0}), shape1SupportFunction(pShape1, (Vector3){0, -1, 0}));\
\
    gjkMetadata.direction = Vector3Negate(gjkMetadata.simplex.vertices[0]);\
\
    gjkMetadata.countDown = maxResolve;\
\
    if(gjkMetadata.countDown < U_GJK_MAX_RESOLVE)\
        gjkMetadata.countDown = U_GJK_MAX_RESOLVE;\
\
    while(gjkMetadata.countDown != 0) {\
\
        gjkMetadata.support = Vector3Subtract(shape0SupportFunction(pShape0, gjkMetadata.direction), shape1SupportFunction(pShape1, Vector3Negate(gjkMetadata.direction)));\
\
        if(Vector3DotProduct(gjkMetadata.support, gjkMetadata.direction) <= 0)\
            return gjkReturn;\
\
        for(unsigned i = 4; i != 1; i--) {\
            gjkMetadata.simplex.vertices[i - 1] = gjkMetadata.simplex.vertices[i - 2];\
        }\
        gjkMetadata.simplex.vertices[0] = gjkMetadata.support;\
        gjkMetadata.simplex.amountVertices++;\
\
        int state = gjkModifySimplex(&gjkMetadata);\
\
        assert(state != -1);\
\
        if(state == 1) {\
            gjkReturn.result = U_GJK_COLLISION;\
            break;\
        }\
\
        gjkMetadata.countDown--;\
    }\
\
    if(gjkReturn.result == U_GJK_NO_COLLISION)\
        gjkReturn.result = U_GJK_NOT_DETERMINED;\

#define U_GJK_EPA_IMPLEMENTATION(shape0SupportFunction, shape1SupportFunction)\
    if(pBackoutCache == NULL)\
        return gjkReturn;\
\
    pBackoutCache->vertexAmount = gjkMetadata.simplex.amountVertices;\
    for(size_t v = 0; v < pBackoutCache->vertexAmount; v++)\
        pBackoutCache->pVertices[v] = gjkMetadata.simplex.vertices[v];\
\
    pBackoutCache->faceAmount = 4;\
    pBackoutCache->pFaces[0] = (UGJKBackoutTriangle) {{}, 0, {0, 1, 2}};\
    pBackoutCache->pFaces[1] = (UGJKBackoutTriangle) {{}, 0, {0, 3, 1}};\
    pBackoutCache->pFaces[2] = (UGJKBackoutTriangle) {{}, 0, {0, 2, 3}};\
    pBackoutCache->pFaces[3] = (UGJKBackoutTriangle) {{}, 0, {1, 3, 2}};\
\
    size_t minFaceIndex;\
\
    epaGetFaceNormals(\
        pBackoutCache->pVertices, pBackoutCache->vertexAmount,\
        pBackoutCache->pFaces, pBackoutCache->faceAmount,\
        &minFaceIndex);\
\
    /* gjkMetadata.direction is Vector3 minNormal;*/\
    float minDistance = FLT_MAX;\
    float sDistance;\
\
    pBackoutCache->newFaceAmount = 1;\
\
    while(minDistance == FLT_MAX) {\
        gjkMetadata.direction = pBackoutCache->pFaces[minFaceIndex].normal;\
        minDistance = pBackoutCache->pFaces[minFaceIndex].distance;\
\
        gjkMetadata.support = Vector3Subtract(shape0SupportFunction(pShape0, gjkMetadata.direction), shape1SupportFunction(pShape1, Vector3Negate(gjkMetadata.direction)));\
\
        sDistance = Vector3DotProduct(gjkMetadata.direction, gjkMetadata.support);\
\
        if(fabs(sDistance - minDistance) > 0.001f && pBackoutCache->vertexAmount < pBackoutCache->vertexLimit && pBackoutCache->newFaceAmount != 0) {\
            minDistance = FLT_MAX;\
\
            pBackoutCache->edgeAmount = 0;\
\
            int edgeLimitHit = 0;\
\
            for(size_t i = 0; i < pBackoutCache->faceAmount; i++) {\
                if(sameDirection(pBackoutCache->pFaces[minFaceIndex].normal, gjkMetadata.support)) {\
                    edgeLimitHit |= epaAddIfUniqueEdge(pBackoutCache->pFaces, pBackoutCache->faceAmount, i, 0, 1, pBackoutCache->pEdges, &pBackoutCache->edgeAmount, pBackoutCache->edgeLimit);\
                    edgeLimitHit |= epaAddIfUniqueEdge(pBackoutCache->pFaces, pBackoutCache->faceAmount, i, 1, 2, pBackoutCache->pEdges, &pBackoutCache->edgeAmount, pBackoutCache->edgeLimit);\
                    edgeLimitHit |= epaAddIfUniqueEdge(pBackoutCache->pFaces, pBackoutCache->faceAmount, i, 2, 0, pBackoutCache->pEdges, &pBackoutCache->edgeAmount, pBackoutCache->edgeLimit);\
\
                    if(edgeLimitHit) {\
                        gjkReturn.result = U_GJK_NOT_DETERMINED;\
                        return gjkReturn;\
                    }\
\
                    pBackoutCache->faceAmount--;\
                    pBackoutCache->pFaces[i] = pBackoutCache->pFaces[pBackoutCache->faceAmount];\
\
                    i--;\
                }\
            }\
\
            pBackoutCache->newFaceAmount = 0;\
            for(size_t edgeIndex = 0; edgeIndex < pBackoutCache->edgeAmount; edgeIndex++) {\
                pBackoutCache->pNewFaces[pBackoutCache->newFaceAmount].vertexIndexes[0] = pBackoutCache->pEdges[edgeIndex].edgeIndexes[0];\
                pBackoutCache->pNewFaces[pBackoutCache->newFaceAmount].vertexIndexes[1] = pBackoutCache->pEdges[edgeIndex].edgeIndexes[1];\
                pBackoutCache->pNewFaces[pBackoutCache->newFaceAmount].vertexIndexes[2] = pBackoutCache->vertexAmount;\
                pBackoutCache->newFaceAmount++;\
            }\
\
            pBackoutCache->pVertices[pBackoutCache->vertexAmount] = gjkMetadata.support;\
            pBackoutCache->vertexAmount++;\
\
            size_t minNewFaceIndex;\
\
            epaGetFaceNormals(\
                pBackoutCache->pVertices, pBackoutCache->vertexAmount,\
                pBackoutCache->pNewFaces, pBackoutCache->newFaceAmount,\
                &minNewFaceIndex);\
\
            float oldMinDistance = FLT_MAX;\
            for(size_t i = 0; i < pBackoutCache->faceAmount; i++) {\
                if(pBackoutCache->pFaces[i].distance < oldMinDistance) {\
                    oldMinDistance = pBackoutCache->pFaces[i].distance;\
                    minFaceIndex = i;\
                }\
            }\
\
            if(pBackoutCache->pNewFaces[minNewFaceIndex].distance < oldMinDistance) {\
                minFaceIndex = minNewFaceIndex + pBackoutCache->faceAmount;\
            }\
\
            if(pBackoutCache->faceAmount + pBackoutCache->newFaceAmount >= pBackoutCache->faceLimit) {\
                gjkReturn.result = U_GJK_NOT_DETERMINED;\
                return gjkReturn;\
            }\
\
            for(size_t i = 0; i < pBackoutCache->newFaceAmount; i++) {\
                pBackoutCache->pFaces[pBackoutCache->faceAmount + i] = pBackoutCache->pNewFaces[i];\
            }\
            pBackoutCache->faceAmount += pBackoutCache->newFaceAmount;\
        }\
    }

UGJKReturn u_gjk_poly(const UGJKPolyhedron *pShape0, const UGJKPolyhedron *pShape1, UGJKBackoutCache *pBackoutCache) {
    assert(pShape0 != NULL);
    assert(pShape1 != NULL);
    assert(pShape0->amountVertices != 0);
    assert(pShape1->amountVertices != 0);
    // pBackoutCache can be NULL or a valid address.

    U_GJK_IMPLEMENTATION(pShape0->amountVertices + pShape1->amountVertices, polyhedronFindFurthestPoint, polyhedronFindFurthestPoint)

    U_GJK_EPA_IMPLEMENTATION(polyhedronFindFurthestPoint, polyhedronFindFurthestPoint)

    gjkReturn.normal = gjkMetadata.direction;
    gjkReturn.distance = minDistance + 0.001f;

    return gjkReturn;
}

UGJKReturn u_gjk_poly_sphere(const UGJKPolyhedron *pShape0, const UGJKSphere *pShape1, UGJKBackoutCache *pBackoutCache) {
    assert(pShape0 != NULL);
    assert(pShape1 != NULL);
    assert(pShape0->amountVertices != 0);
    // pBackoutCache can be NULL or a valid address.

    U_GJK_IMPLEMENTATION(3 * pShape0->amountVertices, polyhedronFindFurthestPoint, sphereFindFurthestPoint)

    U_GJK_EPA_IMPLEMENTATION(polyhedronFindFurthestPoint, sphereFindFurthestPoint)

    gjkReturn.normal = gjkMetadata.direction;
    gjkReturn.distance = minDistance + 0.001f;

    return gjkReturn;
}

UGJKBackoutCache u_gjk_alloc_backout_cache(size_t extraVertices) {
    size_t triangleAmount = 4 + 2 * extraVertices;
    size_t   vertexAmount = 4   +   extraVertices;
    size_t     edgeAmount = 6 + 3 * extraVertices;

    UGJKBackoutCache backoutCache;
    backoutCache.edgeLimit    =     edgeAmount;
    backoutCache.faceLimit    = triangleAmount;
    backoutCache.newFaceLimit = triangleAmount;
    backoutCache.vertexLimit  =   vertexAmount;

    size_t memorySize = 0;
    memorySize += sizeof(UGJKBackoutEdge)     * backoutCache.edgeLimit;
    memorySize += sizeof(UGJKBackoutTriangle) * backoutCache.faceLimit;
    memorySize += sizeof(UGJKBackoutTriangle) * backoutCache.newFaceLimit;
    memorySize += sizeof(Vector3)             * backoutCache.vertexLimit;

    void *pMem = malloc(memorySize);

    backoutCache.pVertices = pMem;
    pMem = pMem + sizeof(Vector3) * backoutCache.vertexLimit;

    backoutCache.pEdges = pMem;
    pMem = pMem + sizeof(UGJKBackoutEdge) * backoutCache.edgeLimit;

    backoutCache.pFaces = pMem;
    pMem = pMem + sizeof(UGJKBackoutTriangle) * backoutCache.faceLimit;

    backoutCache.pNewFaces = pMem;
    pMem = pMem + sizeof(UGJKBackoutTriangle) * backoutCache.newFaceLimit;

    return backoutCache;
}

void u_gjk_free_backout_cache(UGJKBackoutCache *pBackoutCache) {
    assert(pBackoutCache != NULL);

    free(pBackoutCache->pVertices);
}

static int gjkModifySimplex(UGJKMetaData *this) {
    switch(this->simplex.amountVertices) {
        case 2:
            return gjkLine(this);
        case 3:
            return gjkTriangle(this);
        case 4:
            return gjkTetrahedron(this);
        default:
            return -1;
    }
}

static int gjkLine(UGJKMetaData *this) {
    const Vector3 a = this->simplex.vertices[0];
    const Vector3 b = this->simplex.vertices[1];

    const Vector3 ab = Vector3Subtract(b, a);
    const Vector3 ao = Vector3Negate(a);

    if(sameDirection(ab, ao))
        this->direction = Vector3CrossProduct(Vector3CrossProduct(ab, ao), ab);
    else {
        this->simplex.amountVertices = 1;
        this->simplex.vertices[0] = a;
        this->direction = ao;
    }

    return 0;
}

static int gjkTriangle(UGJKMetaData *this) {
    const Vector3 a = this->simplex.vertices[0];
    const Vector3 b = this->simplex.vertices[1];
    const Vector3 c = this->simplex.vertices[2];

    const Vector3 ab = Vector3Subtract(b, a);
    const Vector3 ac = Vector3Subtract(c, a);
    const Vector3 ao = Vector3Negate(a);

    const Vector3 abc = Vector3CrossProduct(ab, ac);

    if(sameDirection(Vector3CrossProduct(abc, ac), ao)) {
        this->simplex.amountVertices = 2;
        this->simplex.vertices[0] = a;
        if(sameDirection(ac, ao)) {
            this->simplex.vertices[1] = c;
            this->direction = Vector3CrossProduct(Vector3CrossProduct(ac, ao), ac);
        }
        else {
            this->simplex.vertices[1] = b;
            return gjkLine(this);
        }
    }
    else {
        if(sameDirection(Vector3CrossProduct(ab, abc), ao)) {
            this->simplex.amountVertices = 2;
            this->simplex.vertices[0] = a;
            this->simplex.vertices[1] = b;
            return gjkLine(this);
        }
        else {
            if(sameDirection(abc, ao)) {
                this->direction = abc;
            }
            else {
                this->simplex.amountVertices = 3;
                this->simplex.vertices[0] = a;
                this->simplex.vertices[1] = c;
                this->simplex.vertices[2] = b;
                this->direction = Vector3Negate(abc);
            }
        }
    }

    return 0;
}

static int gjkTetrahedron(UGJKMetaData *this) {
    const Vector3 a = this->simplex.vertices[0];
    const Vector3 b = this->simplex.vertices[1];
    const Vector3 c = this->simplex.vertices[2];
    const Vector3 d = this->simplex.vertices[3];

    const Vector3 ab = Vector3Subtract(b, a);
    const Vector3 ac = Vector3Subtract(c, a);
    const Vector3 ad = Vector3Subtract(d, a);
    const Vector3 ao = Vector3Negate(a);

    const Vector3 abc = Vector3CrossProduct(ab, ac);
    const Vector3 acd = Vector3CrossProduct(ac, ad);
    const Vector3 adb = Vector3CrossProduct(ad, ab);

    if(sameDirection(abc, ao)) {
        this->simplex.amountVertices = 3;
        this->simplex.vertices[0] = a;
        this->simplex.vertices[1] = b;
        this->simplex.vertices[2] = c;
        return gjkTriangle(this);
    }
    if(sameDirection(acd, ao)) {
        this->simplex.amountVertices = 3;
        this->simplex.vertices[0] = a;
        this->simplex.vertices[1] = c;
        this->simplex.vertices[2] = d;
        return gjkTriangle(this);
    }
    if(sameDirection(adb, ao)) {
        this->simplex.amountVertices = 3;
        this->simplex.vertices[0] = a;
        this->simplex.vertices[1] = d;
        this->simplex.vertices[2] = b;
        return gjkTriangle(this);
    }

    return 1;

}

static void epaGetFaceNormals(const Vector3 *pPolyTope, size_t polyTopeAmount, UGJKBackoutTriangle *pFaces, size_t facesAmount, size_t *pMinTriangleIndex) {
    assert(pPolyTope != NULL);
    //assert(polyTopeAmount >= 4);
    assert(pFaces != NULL);
    //assert(facesAmount > 0);
    assert(pMinTriangleIndex != NULL);

    float minDistance = FLT_MAX;

    *pMinTriangleIndex = 0;

    for(size_t faceIndex = 0; faceIndex < facesAmount; faceIndex++) {
        const Vector3 a = pPolyTope[pFaces[faceIndex].vertexIndexes[0]];
        const Vector3 b = pPolyTope[pFaces[faceIndex].vertexIndexes[1]];
        const Vector3 c = pPolyTope[pFaces[faceIndex].vertexIndexes[2]];

        pFaces[faceIndex].normal = Vector3Normalize(Vector3CrossProduct(Vector3Subtract(b, a),Vector3Subtract(c, a)));
        pFaces[faceIndex].distance = Vector3DotProduct(pFaces[faceIndex].normal, a);

        if(pFaces[faceIndex].distance < 0.0f) {
            pFaces[faceIndex].normal = Vector3Negate(pFaces[faceIndex].normal);
            pFaces[faceIndex].distance = -pFaces[faceIndex].distance;
        }

        if(pFaces[faceIndex].distance < minDistance) {
            *pMinTriangleIndex = faceIndex;
            minDistance = pFaces[faceIndex].distance;
        }
    }
}

static int epaAddIfUniqueEdge(const UGJKBackoutTriangle *pFaces, size_t facesAmount, size_t faceIndex, unsigned a, unsigned b, UGJKBackoutEdge *pEdges, size_t *pEdgeAmount, size_t edgeMax) {
    assert(pFaces != NULL);
    assert(facesAmount >= 0);
    assert(faceIndex < facesAmount);
    assert(a < 3);
    assert(b < 3);
    assert(pEdges != NULL);
    assert(pEdgeAmount != NULL);

    size_t reverseEdgeIndex = *pEdgeAmount;

    for(size_t edgeIndex = 0; edgeIndex < *pEdgeAmount; edgeIndex++) {
        if( pEdges[edgeIndex].edgeIndexes[0] == pFaces[faceIndex].vertexIndexes[b] &&
            pEdges[edgeIndex].edgeIndexes[1] == pFaces[faceIndex].vertexIndexes[a] )
        {
            reverseEdgeIndex = edgeIndex;
            break;
        }
    }

    if(reverseEdgeIndex != *pEdgeAmount) {
        (*pEdgeAmount)--;
        pEdges[reverseEdgeIndex] = pEdges[*pEdgeAmount];
        return 0;
    }
    else if(*pEdgeAmount < edgeMax) {
        pEdges[*pEdgeAmount] = (UGJKBackoutEdge) {pFaces[faceIndex].vertexIndexes[a], pFaces[faceIndex].vertexIndexes[b]};
        (*pEdgeAmount)++;
        return 0;
    }
    return 1; // Not enough cache space.
}

static Vector3 polyhedronFindFurthestPoint(const UGJKPolyhedron *pPolygon, Vector3 direction) {
    Vector3 maxPoint = pPolygon->vertices[pPolygon->amountVertices - 1];
    float maxDistance = Vector3DotProduct(pPolygon->vertices[pPolygon->amountVertices - 1], direction);
    float distance;

    for(size_t i = pPolygon->amountVertices - 1; i != 0; i--) {
        distance = Vector3DotProduct(pPolygon->vertices[i - 1], direction);

        if(distance > maxDistance) {
            maxDistance = distance;
            maxPoint = pPolygon->vertices[i - 1];
        }
    }

    return maxPoint;
}

static Vector3 sphereFindFurthestPoint(const UGJKSphere *pSphere, Vector3 direction) {
    return Vector3Add(pSphere->position, Vector3Scale(Vector3Normalize(direction), pSphere->radius));
}
