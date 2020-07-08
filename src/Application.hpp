#pragma once
#include <cstdint>
#include <functional>
#include <list>
#include <unordered_map>

class Application {
public:
  struct Desc {
    const char *title = nullptr;
    uint32_t width = 0;
    uint32_t height = 0;
  };

  enum struct InputEvent {
    MouseMove,
    MouseDown,
    MouseUp,
    MouseWheel,
  };

  struct Input {
    static std::array<float, 2> cursor_position;
    static std::array<float, 2> scroll_offset;
    static std::array<bool, 2> mouse_down;
  };

  explicit Application(const Application::Desc &desc);

  virtual ~Application();

  void on(InputEvent event, const std::function<void()> &callback);

  virtual void run();

  const Desc &get_desc() { return desc; }

  virtual void update(){};

protected:
  void *window = nullptr;

private:
  Desc desc{};
  using Callback = std::function<void()>;
  using CallbackList = std::list<Callback>;
  std::unordered_map<InputEvent, CallbackList> callbacks;
};
