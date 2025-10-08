#pragma once

struct Vec2 {
    float x;
    float y;
    bool operator!=(const Vec2& other) const { return x != other.x || y != other.y; }
};

struct Vec3 {
    float x;
    float y;
    float z;
    bool operator!=(const Vec3& other) const { return x != other.x || y != other.y || z != other.z; }
};

struct Vec4 {
    float x;
    float y;
    float z;
    float w;
    bool operator!=(const Vec4& other) const { return x != other.x || y != other.y || z != other.z || w != other.w; }
};
