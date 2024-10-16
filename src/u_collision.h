#ifndef U_GJK_29
#define U_GJK_29

#include "u_collision_def.h"

/**
 * This function determines if a polyhedron would collide with another polyhedron.
 * @param pPoly0 The first polyhedron to compute. @warning Do not put a zero point polyhedron.
 * @param pPoly1 The second polyhedron to compute. @warning Do not put a zero point polyhedron.
 * @param pBackoutCache The back out cache if it is needed. @note This can actually be NULL, but then the backout code will not be run.
 * @return U_GJK_NO_COLLISION if there is no collision. U_GJK_COLLISION if the collision happended U_GJK_NOT_DETERMINED if the collision cannot be confirmed.
 *          UCollisionReturn::normal and UCollisionReturn::depth is returned if pBackoutCache is used for this function. @note To backout pPoly0 multiply the normal by negative depth and add it to pPoly0
 */
UCollisionReturn u_collision_poly(const UCollisionPolyhedron *pPoly0, const UCollisionPolyhedron *pPoly1, UCollisionBackoutCache *pBackoutCache);

/**
 * This function determines if a polyhedron would collide with a sphere.
 * @param pPoly Polyhedron to compute. @warning Do not put a zero point polyhedron.
 * @param pSphere The sphere to test the collision with.
 * @param pBackoutCache The back out cache if it is needed. @note This can actually be NULL, but then the backout code will not be run.
 * @return U_GJK_NO_COLLISION if there is no collision. U_GJK_COLLISION if the collision happended U_GJK_NOT_DETERMINED if the collision cannot be confirmed.
 *          UCollisionReturn::normal and UCollisionReturn::depth is returned if pBackoutCache is used for this function. @note To backout pPoly multiply the normal by negative depth and add it to pPoly
 */
UCollisionReturn u_collision_poly_sphere(const UCollisionPolyhedron *pPoly, const UCollisionSphere *pSphere, UCollisionBackoutCache *pBackoutCache);

/**
 * This function determines if a sphere would collide with a sphere.
 * @note This function is faster than every single test.
 * @param pSphere0 A sphere to test the collision with.
 * @param pSphere1 A sphere to test the collision with.
 * @return U_GJK_NO_COLLISION if there is no collision. U_GJK_COLLISION if the collision happended U_GJK_NOT_DETERMINED if the collision cannot be confirmed.
 *          UCollisionReturn::normal and UCollisionReturn::depth is also returned. @note To backout pPoly multiply the normal by negative depth and add it to pPoly
 */
UCollisionReturn u_collision_sphere(const UCollisionSphere *pSphere0, const UCollisionSphere *pSphere1);

/**
 * This function allocates the backout cache.
 * @note This uses math to make a backout cache hold a polyhedron. Triangles, edges and vertices increases as extraVertices
 * @warning Use u_gjk_free_backout_cache to delete the returned backout cache.
 * @param extraVertices The amount of extra vertices to add. @note The amount of zero will still work, but other functions will not work well with the returned result.
 * @return A backout cache used to calculate getting shapes out of shapes.
 */
UCollisionBackoutCache u_collision_alloc_backout_cache(size_t extraVertices);

/**
 * This function frees the pBackoutCache.
 * @param pBackoutCache A pointer to the backout cache to delete the internals used.
 */
void u_collision_free_backout_cache(UCollisionBackoutCache *pBackoutCache);

#endif // U_GJK_DEF_29
