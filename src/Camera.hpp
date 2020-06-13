#pragma once
#include <Eigen/Core>
#include <cstdint>
#define PI 3.1415926

class Camera {
public:
    enum class Projection : uint32_t {
        Perspective,
        Orthographic,
    };

    enum class Fov : uint32_t { VERTICAL,
                                HORIZONTAL };

    void lookAt(const Eigen::Vector3f &position, const Eigen::Vector3f &target,
                const Eigen::Vector3f &up) noexcept;

    void setProjection(Projection projection, float left, float right,
                       float top, float bottom, float near, float far) noexcept;

    void setProjection(float fov, float aspect, float z_near, float z_far,
                       Fov direction = Fov::VERTICAL) noexcept;

    void setModelMatrix(const Eigen::Matrix4f &view) noexcept;

    [[nodiscard]] const Eigen::Matrix4f &getCullingProjectionMatrix() const
            noexcept {
        return projection_matrix_for_culling;
    }

    [[nodiscard]] const Eigen::Matrix4f &getProjectionMatrix() const noexcept {
        return projection_matrix;
    }

    [[nodiscard]] auto getViewMatrix() const noexcept -> const Eigen::Matrix4f & {
        return view_matrix;
    }

    [[nodiscard]] auto getPosition() const noexcept -> Eigen::Vector3f {
        Eigen::Vector4f column = view_matrix.col(3);
        return Eigen::Vector3f{column.x(), column.y(), column.z()};
    }

    [[nodiscard]] auto getUpVector() const noexcept -> Eigen::Vector3f {
        Eigen::Vector4f column = view_matrix.col(1);
        return (Eigen::Vector3f{column.x(), column.y(), column.z()}).normalized();
    }

    [[nodiscard]] auto getNear() const noexcept -> float { return z_near; }

    [[nodiscard]] auto getFar() const noexcept -> float { return z_far; }

private:
    float z_near = 0.01f;
    float z_far = 1000.f;

    Eigen::Matrix4f projection_matrix;// projection matrix (infinite far)
    Eigen::Matrix4f
            projection_matrix_for_culling;// projection matrix (with far plane)

    Eigen::Matrix4f view_matrix;
};
