#ifndef U_GJK_29
#define U_GJK_29

#include "u_gjk_def.h"

UGJKReturn u_gjk_poly(const UGJKPolyhedron *pPoly0, const UGJKPolyhedron *pPoly1, UGJKBackoutCache *pBackoutCache);

UGJKReturn u_gjk_poly_sphere(const UGJKPolyhedron *pPoly, const UGJKSphere *pSphere, UGJKBackoutCache *pBackoutCache);

//UGJKReturn u_gjk_sphere(..., int backout);

UGJKBackoutCache u_gjk_alloc_backout_cache(size_t extraVertices);

void u_gjk_free_backout_cache(UGJKBackoutCache *pBackoutCache);

#endif // U_GJK_DEF_29
