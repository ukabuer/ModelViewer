#include "Camera.hpp"
#include <Eigen/Core>
#include <Eigen/Geometry>
#include <cmath>

using namespace Eigen;

static auto perspective(float left, float right, float top, float bottom,
                        float near, float far) -> Matrix4f {
    Matrix4f m = Matrix4f::Zero();

    m(0, 0) = (2.f * near) / (right - left);
    m(0, 2) = (right + left) / (right - left);
    m(1, 1) = (2.f * near) / (top - bottom);
    m(1, 2) = (top + bottom) / (top - bottom);
    m(2, 2) = -(far + near) / (far - near);
    m(2, 3) = -(2.f * far * near) / (far - near);
    m(3, 2) = -1.f;

    return m;
}

static auto orthographic(float left, float right, float top, float bottom,
                         float near, float far) -> Matrix4f {
    Matrix4f m = Matrix4f::Zero();

    const auto w = 1.0f / (right - left);
    const auto h = 1.0f / (top - bottom);
    const auto p = 1.0f / (far - near);

    const auto x = (right + left) * w;
    const auto y = (top + bottom) * h;
    const auto z = (far + near) * p;

    m(0, 0) = 2.0f * w;
    m(0, 3) = -x;
    m(1, 1) = 2.0f * h;
    m(1, 3) = -y;
    m(2, 2) = -2.0f * p;
    m(2, 3) = -z;
    m(3, 3) = 1.0f;

    return m;
}

static auto lookAtMatrix(const Vector3f &pos, const Vector3f &target,
                         const Vector3f &up) -> Matrix4f {
    Matrix4f m = Matrix4f::Zero();

    Vector3f zAxis = (pos - target).normalized();
    Vector3f xAxis = up.cross(zAxis).normalized();
    Vector3f yAxis = zAxis.cross(xAxis);

    m(0, 0) = xAxis[0];
    m(0, 1) = yAxis[0];
    m(0, 2) = zAxis[0];
    m(0, 3) = pos[0];
    m(1, 0) = xAxis[1];
    m(1, 1) = yAxis[1];
    m(1, 2) = zAxis[1];
    m(1, 3) = pos[1];
    m(2, 0) = xAxis[2];
    m(2, 1) = yAxis[2];
    m(2, 2) = zAxis[2];
    m(2, 3) = pos[2];
    m(3, 3) = 1.f;

    return m;
}

void Camera::setProjection(Projection projection, float left, float right,
                           float top, float bottom, float near,
                           float far) noexcept {
    Matrix4f p;
    switch (projection) {
        case Projection::Perspective:
            p = perspective(left, right, top, bottom, near, far);
            projection_matrix_for_culling = p;

            p(2, 2) = -1;       // lim(far->inf) = -1
            p(3, 2) = -2 * near;// lim(far->inf) = -2*near

            break;
        case Projection::Orthographic:
            p = orthographic(left, right, top, bottom, near, far);
            projection_matrix_for_culling = p;
            break;
    }

    projection_matrix = p;
    z_near = near;
    z_far = far;
}

void Camera::setProjection(float fov, float aspect, float near, float far,
                           Fov direction) noexcept {
    float w, h;
    auto s = std::tan(fov * PI / 360.0f) * near;

    if (direction == Fov::VERTICAL) {
        w = s * aspect;
        h = s;
    } else {
        w = s;
        h = s / aspect;
    };

    this->setProjection(Projection::Perspective, -w, w, h, -h, near, far);
}

void Camera::lookAt(const Eigen::Vector3f &position,
                    const Eigen::Vector3f &target,
                    const Eigen::Vector3f &up) noexcept {
    this->view_matrix = lookAtMatrix(position, target, up);
}

void Camera::setModelMatrix(const Eigen::Matrix4f &view) noexcept {
    this->view_matrix = view;
}
