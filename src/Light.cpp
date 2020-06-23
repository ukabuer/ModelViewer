#include <Light.hpp>
#include <render_pass/ShadowPass.hpp>

using namespace std;

Light::Light() { shadow_pass = make_unique<ShadowPass>(); }