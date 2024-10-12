#include "u_gjk.h"

#include <assert.h>
#include <float.h>

#define U_GJK_MAX_RESOLVE 8

static Vector3 polyhedronFindFurthestPoint(const UGJKPolyhedron *pPolygon, Vector3 direction);
static int modifySimplex(UGJKMetaData *this);
static int line(UGJKMetaData *this);
static int triangle(UGJKMetaData *this);
static int tetrahedron(UGJKMetaData *this);
static int epaGetFaceNormals(const Vector3 *pPolyTope, size_t polyTopeAmount, const UGJKBackoutTriangle *pFaces, size_t facesAmount, Vector4 *pNormals, size_t *pNormalAmount, size_t normalAmountMax, size_t *pMinTriangleIndex);
static int epaAddIfUniqueEdge(const UGJKBackoutTriangle *pFaces, size_t facesAmount, size_t faceIndex, unsigned a, unsigned b, UGJKBackoutEdge *pEdges, size_t *pEdgeAmount, size_t edgeMax);

static inline int sameDirection(Vector3 direction, Vector3 ao) { return Vector3DotProduct(direction, ao) > 0.0f; }

UGJKReturn u_gjk_poly(const UGJKPolyhedron *pPoly0, const UGJKPolyhedron *pPoly1, UGJKBackoutCache *pBackoutCache) {
    assert(pPoly0 != NULL);
    assert(pPoly1 != NULL);
    assert(pPoly0->amountVertices != 0);
    assert(pPoly1->amountVertices != 0);
    // pBackoutCache can be NULL or a valid address.

    UGJKReturn gjkReturn = {0};
    gjkReturn.result = U_GJK_NO_COLLISION;

    UGJKMetaData gjkMetadata = {0};
    gjkMetadata.pConvexShape[0] = pPoly0;
    gjkMetadata.pConvexShape[1] = pPoly1;

    gjkMetadata.simplex.amountVertices = 1;
    gjkMetadata.simplex.vertices[0] = Vector3Subtract(polyhedronFindFurthestPoint(pPoly0, (Vector3){0, 1, 0}), polyhedronFindFurthestPoint(pPoly1, (Vector3){0, -1, 0}));
    gjkMetadata.direction = Vector3Negate(gjkMetadata.simplex.vertices[0]);

    gjkMetadata.countDown = pPoly0->amountVertices + pPoly1->amountVertices;

    if(gjkMetadata.countDown < U_GJK_MAX_RESOLVE)
        gjkMetadata.countDown = U_GJK_MAX_RESOLVE;

    while(gjkMetadata.countDown != 0) {
        gjkMetadata.support = Vector3Subtract(polyhedronFindFurthestPoint(pPoly0, gjkMetadata.direction), polyhedronFindFurthestPoint(pPoly1, Vector3Negate(gjkMetadata.direction)));

        if(Vector3DotProduct(gjkMetadata.support, gjkMetadata.direction) <= 0)
            return gjkReturn;

        for(unsigned i = 4; i != 1; i--) {
            gjkMetadata.simplex.vertices[i - 1] = gjkMetadata.simplex.vertices[i - 2];
        }
        gjkMetadata.simplex.vertices[0] = gjkMetadata.support;
        gjkMetadata.simplex.amountVertices++;

        int state = modifySimplex(&gjkMetadata);

        assert(state != -1);

        if(state == 1) {
            gjkReturn.result = U_GJK_COLLISION;
            break;
        }

        gjkMetadata.countDown--;
    }

    if(gjkReturn.result == U_GJK_NO_COLLISION)
        gjkReturn.result = U_GJK_NOT_DETERMINED;

    if(pBackoutCache == NULL)
        return gjkReturn;

    pBackoutCache->vertexAmount = gjkMetadata.simplex.amountVertices;
    for(size_t v = 0; v < pBackoutCache->vertexAmount; v++)
        pBackoutCache->pVertices[v] = gjkMetadata.simplex.vertices[v];

    pBackoutCache->faceAmount = 4;
    pBackoutCache->pFaces[0] = (UGJKBackoutTriangle) {0, 1, 2};
    pBackoutCache->pFaces[1] = (UGJKBackoutTriangle) {0, 3, 1};
    pBackoutCache->pFaces[2] = (UGJKBackoutTriangle) {0, 2, 3};
    pBackoutCache->pFaces[3] = (UGJKBackoutTriangle) {1, 3, 2};

    size_t minTriangleIndex;

    epaGetFaceNormals(
        pBackoutCache->pVertices, pBackoutCache->vertexAmount,
        pBackoutCache->pFaces, pBackoutCache->faceAmount,
        pBackoutCache->pNormals, &pBackoutCache->normalAmount, pBackoutCache->normalLimit,
        &minTriangleIndex);

    // gjkMetadata.direction is Vector3 minNormal;
    float minDistance = FLT_MAX;
    float sDistance;

    while(minDistance == FLT_MAX) {
        gjkMetadata.direction.x = pBackoutCache->pNormals[minTriangleIndex].x;
        gjkMetadata.direction.y = pBackoutCache->pNormals[minTriangleIndex].y;
        gjkMetadata.direction.z = pBackoutCache->pNormals[minTriangleIndex].z;
        minDistance = pBackoutCache->pNormals[minTriangleIndex].w;

        gjkMetadata.support = Vector3Subtract(polyhedronFindFurthestPoint(pPoly0, gjkMetadata.direction), polyhedronFindFurthestPoint(pPoly1, Vector3Negate(gjkMetadata.direction)));
        sDistance = Vector3DotProduct(gjkMetadata.direction, gjkMetadata.support);

        if(fabs(sDistance - minDistance ) > 0.001f) {
            minDistance = FLT_MAX;

            pBackoutCache->edgeAmount = 0;

            for(size_t i = 0; i < pBackoutCache->normalAmount; i++) {
                if(sameDirection((Vector3){pBackoutCache->pNormals[minTriangleIndex].x, pBackoutCache->pNormals[minTriangleIndex].y, pBackoutCache->pNormals[minTriangleIndex].z}, gjkMetadata.support)) {
                    epaAddIfUniqueEdge(pBackoutCache->pFaces, pBackoutCache->faceAmount, i, 0, 1, pBackoutCache->pEdges, &pBackoutCache->edgeAmount, pBackoutCache->edgeLimit);
                    epaAddIfUniqueEdge(pBackoutCache->pFaces, pBackoutCache->faceAmount, i, 1, 2, pBackoutCache->pEdges, &pBackoutCache->edgeAmount, pBackoutCache->edgeLimit);
                    epaAddIfUniqueEdge(pBackoutCache->pFaces, pBackoutCache->faceAmount, i, 2, 0, pBackoutCache->pEdges, &pBackoutCache->edgeAmount, pBackoutCache->edgeLimit);

                    pBackoutCache->pFaces[i] = pBackoutCache->pFaces[pBackoutCache->faceAmount];
                    pBackoutCache->faceAmount--;

                    pBackoutCache->pNormals[i] = pBackoutCache->pNormals[pBackoutCache->normalAmount];
                    pBackoutCache->normalAmount--;

                    i--;
                }
            }
        }
    }

    return gjkReturn;
}

static int modifySimplex(UGJKMetaData *this) {
    switch(this->simplex.amountVertices) {
        case 2:
            return line(this);
        case 3:
            return triangle(this);
        case 4:
            return tetrahedron(this);
        default:
            return -1;
    }
}

static int line(UGJKMetaData *this) {
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

static int triangle(UGJKMetaData *this) {
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
            return line(this);
        }
    }
    else {
        if(sameDirection(Vector3CrossProduct(ab, abc), ao)) {
            this->simplex.amountVertices = 2;
            this->simplex.vertices[0] = a;
            this->simplex.vertices[1] = b;
            return line(this);
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

static int tetrahedron(UGJKMetaData *this) {
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
        return triangle(this);
    }
    if(sameDirection(acd, ao)) {
        this->simplex.amountVertices = 3;
        this->simplex.vertices[0] = a;
        this->simplex.vertices[1] = c;
        this->simplex.vertices[2] = d;
        return triangle(this);
    }
    if(sameDirection(adb, ao)) {
        this->simplex.amountVertices = 3;
        this->simplex.vertices[0] = a;
        this->simplex.vertices[1] = d;
        this->simplex.vertices[2] = b;
        return triangle(this);
    }

    return 1;

}

static int epaGetFaceNormals(const Vector3 *pPolyTope, size_t polyTopeAmount, const UGJKBackoutTriangle *pFaces, size_t facesAmount, Vector4 *pNormals, size_t *pNormalAmount, size_t normalAmountMax, size_t *pMinTriangleIndex) {
    assert(pPolyTope != NULL);
    assert(polyTopeAmount >= 4);
    assert(pFaces != NULL);
    assert(facesAmount >= 4);
    assert(pNormals != NULL);
    assert(pNormalAmount != NULL);
    assert(*pNormalAmount != 0);
    assert(pMinTriangleIndex != NULL);

    float minDistance = FLT_MAX;

    *pMinTriangleIndex = 0;

    for(size_t faceIndex = 0; faceIndex < facesAmount; faceIndex++) {
        const Vector3 a = pPolyTope[pFaces[faceIndex].vertexIndexes[0]];
        const Vector3 b = pPolyTope[pFaces[faceIndex].vertexIndexes[1]];
        const Vector3 c = pPolyTope[pFaces[faceIndex].vertexIndexes[2]];

        Vector3 normal = Vector3Normalize(Vector3CrossProduct(Vector3Subtract(b, a),Vector3Subtract(c, a)));
        float distance = Vector3DotProduct(normal, a);

        if(distance < 0.0f) {
            normal = Vector3Negate(normal);
            distance = -distance;
        }

        if(*pNormalAmount == normalAmountMax)
            return 0; // Ran out of normal space.

        pNormals[*pNormalAmount].x = normal.x;
        pNormals[*pNormalAmount].y = normal.y;
        pNormals[*pNormalAmount].z = normal.z;
        pNormals[*pNormalAmount].w = distance;
        (*pNormalAmount)++;
        assert(*pNormalAmount != 0);

        if(distance < minDistance) {
            *pMinTriangleIndex = faceIndex;
            minDistance = distance;
        }
    }
    return 1;
}

static int epaAddIfUniqueEdge(const UGJKBackoutTriangle *pFaces, size_t facesAmount, size_t faceIndex, unsigned a, unsigned b, UGJKBackoutEdge *pEdges, size_t *pEdgeAmount, size_t edgeMax) {
    assert(pFaces != NULL);
    assert(facesAmount >= 4);
    assert(faceIndex < facesAmount);
    assert(a < 3);
    assert(b < 3);
    assert(pEdges != NULL);
    assert(pEdgeAmount != NULL);
    assert(*pEdgeAmount >= 6);

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
        pEdges[reverseEdgeIndex] = pEdges[*pEdgeAmount - 1];
        (*pEdgeAmount)--;
        return 1;
    }
    else if(*pEdgeAmount < edgeMax) {
        pEdges[*pEdgeAmount] = (UGJKBackoutEdge) {a, b};
        (*pEdgeAmount)++;
        return 1;
    }
    return 0; // Not enough cache space.
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
