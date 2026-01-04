#ifndef ALCENGINE_CORE_REGISTRY_BASE_GLOBALSTRUCTS_H
#define ALCENGINE_CORE_REGISTRY_BASE_GLOBALSTRUCTS_H

#include <string>
#include <vector>
#include <cstdint>

struct Vector {
    Vector(float _x, float _y, float _z) : val(_x, _y, _z) {};
    Vector(float* _val) : val(_val[0], _val[1], _val[2]) {};

    float& x = val[0];
    float& y = val[1];
    float& z = val[2];

    float val[3];
};

#endif