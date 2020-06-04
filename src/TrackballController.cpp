#include "TrackballController.hpp"
#include <Eigen/Core>
#include <Eigen/Geometry>

using namespace std;
using namespace Eigen;

void TrackballController::update() {
    this->eye = this->position - this->target;

    this->zoom();
    this->pan();
    this->rotate();

    this->position = this->target + this->eye;
}

void TrackballController::zoom() {
    auto factor = 1.0f + this->zoom_delta;

    if (factor != 1.0f && factor > 0.0f) {
        this->eye *= factor;
    }

    this->zoom_delta = 0.0f;
}

void TrackballController::pan() {
    Vector2f moved = this->pan_end - this->pan_start;

    if (moved.norm() <= 0.001f) {
        return;
    }

    float factor = this->eye.norm() * this->pan_speed;
    moved = moved * factor;

    Vector3f panned = this->eye.cross(this->up);
    panned = panned.normalized() * moved[0];
    panned += this->up.normalized() * moved[1];

    this->position += panned;
    this->target += panned;

    this->pan_start = this->pan_end;
}

void TrackballController::rotate() {
    Vector3f moved = {this->move_current[0] - this->move_prev[0],
                      this->move_current[1] - this->move_prev[1], 0.0f};
    float angle = moved.norm();

    if (angle <= 0.001f) {
        this->move_prev = this->move_current;
        return;
    }

    Vector3f eye_dir = this->eye.normalized();
    Vector3f up_dir = this->up.normalized();
    Vector3f sideway_dir = up_dir.cross(eye_dir).normalized();

    sideway_dir *= moved[0];
    up_dir *= moved[1];
    Vector3f move_dir = up_dir + sideway_dir;
    Vector3f axis = move_dir.cross(this->eye).normalized();

    angle *= rotate_speed;
    Quaternionf quaternion(AngleAxisf(angle, axis));

    this->eye = quaternion * this->eye;
    this->up = quaternion * this->up;

    this->move_prev = this->move_current;
}
