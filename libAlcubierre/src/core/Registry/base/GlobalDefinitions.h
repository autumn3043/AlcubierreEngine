#ifndef ALCENGINE_CORE_REGISTRY_BASE_GLOBALSTRUCTS_H
#define ALCENGINE_CORE_REGISTRY_BASE_GLOBALSTRUCTS_H

#include <string>
#include <vector>
#include <cstdint>
#include <cassert>

typedef uint64_t Hash_T;

struct Vector2 {
    float val[2];
    float& x = val[0];
    float& y = val[1];
    
    Vector2(float _x, float _y) : val(_x, _y) {};
    Vector2(float* _val) : val(_val[0], _val[1]) {};

    Vector2(const Vector2& other) : val{other.val[0], other.val[1]} {}

    Vector2& operator=(const Vector2& other) {
        if (this != &other) {
            val[0] = other.val[0];
            val[1] = other.val[1];
        }
        return *this;
    }

    Vector2() : val(0.0f, 0.0f) {}
};

struct Vector3 {
    float val[3];
    float& x = val[0];
    float& y = val[1];
    float& z = val[2];
    
    Vector3(float _x, float _y, float _z) : val(_x, _y, _z) {};
    Vector3(float* _val) : val(_val[0], _val[1], _val[2]) {};

    Vector3(const Vector3& other) : val{other.val[0], other.val[1], other.val[2]} {}

    Vector3& operator=(const Vector3& other) {
        if (this != &other) {
            val[0] = other.val[0];
            val[1] = other.val[1];
            val[2] = other.val[2];
        }
        return *this;
    }

    Vector3() : val(0.0f, 0.0f, 0.0f) {}
};

struct Vertex {
    Vector3 pos;
    Vector3 normal;
    Vector2 uv;
};

#endif