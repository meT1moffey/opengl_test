#pragma once

struct Color {
    Color() {}
    Color(float red_, float green_, float blue_, float alpha_ = 1) :
            red(red_),
            green(green_),
            blue(blue_),
            alpha(alpha_) {}
    float red;
    float green;
    float blue;
    float alpha;
};