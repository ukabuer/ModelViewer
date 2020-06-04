#pragma once
#include "Camera.hpp"
#include <iostream>

class TrackballController {
public:
    TrackballController(uint32_t width, uint32_t height)
        : width(width), height(height) {}

    void onMouseDown(bool isLeft, int x, int y) {
        if (isLeft) {
            this->state = State::Rotate;
        } else {
            this->state = State::Pan;
        }

        if (this->state == State::Rotate) {
            move_current = get_mouse_on_circle(x, y);
            move_prev = move_current;
        } else {
            pan_start = this->get_mouse_on_screen(x, y);
            pan_end = pan_start;
        }
    }

    void onMouseUp(bool isLeft, int x, int y) { this->state = State::None; }

    void onMouseWheelScroll(float delta) {
        this->state = State::Zoom;
        this->zoom_delta = delta * zoom_speed;
    }

    void onMouseMove(int x, int y) {
        switch (this->state) {
            case State::Rotate: {
                this->move_prev = this->move_current;
                this->move_current = this->get_mouse_on_circle(x, y);
                break;
            }
            case State::Zoom: {
                break;
            }
            case State::Pan: {
                this->pan_end = this->get_mouse_on_screen(x, y);
                break;
            }
            default:
                break;
        }
    }

    void update();

    float pan_speed = 0.01f;
    float zoom_speed = 0.01f;
    float rotate_speed = 1.0f;

    Eigen::Vector3f position = {0.0f, 0.0f, 1.0f};
    Eigen::Vector3f up = {0.0f, 1.0f, 0.0f};
    Eigen::Vector3f target = {0.0f, 0.0f, 0.0f};

private:
    enum class State { Rotate,
                       Pan,
                       Zoom,
                       None };

    Eigen::Vector3f eye = {0.0f, 0.0f, -1.0f};

    State state = State::None;

    Eigen::Vector2f move_prev = {0.0f, 0.0f};
    Eigen::Vector2f move_current = {0.0f, 0.0f};

    Eigen::Vector2f pan_start = {0.0f, 0.0f};
    Eigen::Vector2f pan_end = {0.0f, 0.0f};

    float zoom_delta = 0.f;

    uint32_t width = 0;
    uint32_t height = 0;

    auto get_mouse_on_circle(int x, int y) const -> Eigen::Vector2f {
        return {(static_cast<float>(x) - width * 0.5f) / (width * 0.5f),
                (height * 0.5f - static_cast<float>(y)) / (width * 0.5f)};
    }

    auto get_mouse_on_screen(int x, int y) const -> Eigen::Vector2f {
        return {static_cast<float>(x) / width, static_cast<float>(y) / height};
    }

    void zoom();
    void rotate();
    void pan();
};
