#ifndef V_RAYMATH_29
#define V_RAYMATH_29

#include "raymath.h"

Matrix MatrixVulkanPerspective(double fovY, double aspect, double nearPlane, double farPlane);

#endif // V_RAYMATH_29

#ifdef V_RAYMATH_29_IMPLEMENTATION
Matrix MatrixVulkanPerspective(double fovY, double aspect, double nearPlane, double farPlane)
{
    // Thanks to https://www.vincentparizet.com/blog/posts/vulkan_perspective_matrix/
    // Reverse Z buffer https://tomhultonharrop.com/mathematics/graphics/2023/08/06/reverse-z.html
    Matrix perspective = { 0 };

    double focal_length = 1.0 / tan(0.5 * fovY);

    perspective.m0  = focal_length / aspect;
    perspective.m5  = -focal_length;
    perspective.m10 = nearPlane / (farPlane - nearPlane);
    perspective.m11 = -1;
    perspective.m14 = (farPlane * nearPlane) / (farPlane - nearPlane);

    return perspective;
}
#endif
