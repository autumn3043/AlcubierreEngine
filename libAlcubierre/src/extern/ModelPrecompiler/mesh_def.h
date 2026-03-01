#ifndef ALCUBIERRE_ENGINE_FILETYPE_MESH_H
#define ALCUBIERRE_ENGINE_FILETYPE_MESH_H

#include <cstdint>
#include <cstring>

#include "core/Registry/base/GlobalDefinitions.h"

const uint64_t MAGIC = 2234876u;

struct AlcubierreEngineMesh {
    public:
        struct Header {
            uint64_t magic = MAGIC;

            uint64_t hash;
            uint32_t version;

            uint64_t vertexSize = sizeof(Vector3);
            uint64_t indexSize = sizeof(uint32_t);
        };

        Header header;

        struct Body {
            uint64_t vertexCount;
            Vector3* vertices;

            uint64_t indexCount;
            uint32_t* indices;
        };

        Body body;

    private:
        int copy(char* dst, char* src, uint64_t size, uint64_t* offset) {
            memcpy(dst, src, size);
            *offset += size;

            return 0;
        }

        bool verify(char* data, uint64_t size) {
            if(size < sizeof(Header)) return false;

            Header other;
            memcpy(reinterpret_cast<char*>(&other), data, sizeof(Header));

            if(header.magic != other.magic) return false;
            if(header.vertexSize != other.vertexSize) return false;
            if(header.indexSize != other.indexSize) return false;

            return true;
        }

    public:
        int deserialise(char* data, uint64_t size) {
            if(!verify(data, size)) return 1;

            uint64_t offset = 0;

            copy(reinterpret_cast<char*>(&header), data + offset, sizeof(Header), &offset);
            copy(reinterpret_cast<char*>(&body.vertexCount), data + offset, sizeof(uint64_t), &offset);
            body.vertices = new Vector3[body.vertexCount];
            copy(reinterpret_cast<char*>(body.vertices), data + offset, header.vertexSize * body.vertexCount, &offset);
            copy(reinterpret_cast<char*>(&body.indexCount), data + offset, sizeof(uint64_t), &offset);
            body.indices = new uint32_t[body.indexCount];
            copy(reinterpret_cast<char*>(body.indices), data + offset, header.indexSize * body.indexCount, &offset);

            return 0;
        }
};

#endif