#include "u_gjk.h"

#include <assert.h>

#define U_GJK_MAX_RESOLVE 8

static Vector3 polyhedronFindFurthestPoint(const UGJKPolyhedron *pPolygon, Vector3 direction);
static int modifySimplex(UGJKMetaData *this);
static int line(UGJKMetaData *this);
static int triangle(UGJKMetaData *this);
static int tetrahedron(UGJKMetaData *this);
static inline int sameDirection(Vector3 direction, Vector3 ao) { return Vector3DotProduct(direction, ao) > 0.0f; }

UGJKReturn u_gjk_poly(const UGJKPolyhedron *pPoly0, const UGJKPolyhedron *pPoly1, int backout) {
    assert(pPoly0 != NULL);
    assert(pPoly1 != NULL);
    assert(pPoly0->amountVertices != 0);
    assert(pPoly1->amountVertices != 0);
    assert(backout == 0 || backout == 1);

    UGJKReturn gjkReturn = {0};
    gjkReturn.result = U_GJK_NO_COLLISION;

    UGJKMetaData gjkMetadata = {0};
    gjkMetadata.pConvexShape[0] = pPoly0;
    gjkMetadata.pConvexShape[1] = pPoly1;

    gjkMetadata.simplex.amountVertices = 1;
    gjkMetadata.simplex.vertices[0] = Vector3Subtract(polyhedronFindFurthestPoint(pPoly0, (Vector3){0, 1, 0}), polyhedronFindFurthestPoint(pPoly1, (Vector3){0, -1, 0}));
    gjkMetadata.direction = Vector3Negate(gjkMetadata.simplex.vertices[0]);
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
