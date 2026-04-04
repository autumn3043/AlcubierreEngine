#ifndef ALCUBIERRE_EXTERN_MODELPRECOMPILER_PUBLIC_H
#define ALCUBIERRE_EXTERN_MODELPRECOMPILER_PUBLIC_H

#include <cstdint>
#include <cstring>
#include <stdexcept>
class SerialiseException : public std::runtime_error {
    public:
        SerialiseException(std::string message) : std::runtime_error(message) {}
};

#include "core/Registry/base/GlobalDefinitions.h"

const uint64_t MAGIC = 2234876u;

struct AlcPrecompiledData {
    public:
        enum dataType : uint8_t {
            MESH = 0,
            MATERIAL = 1,
            TEXTURE = 2
        };

        struct Header {
            Header() : magic(MAGIC) {}
            Header(Hash_T _hash, dataType _type) : magic(MAGIC), hash(_hash), type(_type) {}

            uint64_t magic;

            Hash_T hash;
            dataType type;

            Header(char* blobData, uint64_t blobDataSize) : magic(MAGIC) {
                if(blobDataSize < sizeof(Header)) throw SerialiseException("Blob data size " + std::to_string(blobDataSize) + " was less than the standard asset header size");
                Header other; 
                memcpy(&other, blobData, sizeof(Header));
                if(magic != other.magic) throw SerialiseException("Asset header magic mismatch. The file may be malformed");
                hash = other.hash;
                type = other.type;
            }
        };

        const Header header;

        AlcPrecompiledData() = delete;
        virtual ~AlcPrecompiledData() = default;
        AlcPrecompiledData(Hash_T _hash, dataType _type) : header(_hash, _type) {}
        AlcPrecompiledData(char* blobData, uint64_t blobDataSize) : header(blobData, blobDataSize) {}

        virtual uint64_t assetSize() = 0;

    protected:
        int copy(void* dst, char* src, uint64_t size, uint64_t* index, uint64_t& totalSize) {
            if(*index + size > totalSize) throw SerialiseException("Asset serialisation stepped beyond end");

            memcpy(reinterpret_cast<char*>(dst), src + *index, size);
            *index += size;

            return 0;
        }
};

struct Texture : public AlcPrecompiledData {
    Texture(Hash_T _hash) : AlcPrecompiledData(_hash, TEXTURE) {}

    uint64_t texelDataSize = 0;
    uint8_t* texelData = nullptr;

    uint64_t assetSize() override { 
        return sizeof(Header) 
            + sizeof(uint64_t) + texelDataSize;
    }

    Texture(char* blobData, uint64_t blobDataSize) : AlcPrecompiledData(blobData, blobDataSize) {
        uint64_t index = sizeof(Header);

        copy(&texelDataSize, blobData, sizeof(uint64_t), &index, blobDataSize);
        texelData = new uint8_t[texelDataSize];
        copy(texelData, blobData, texelDataSize, &index, blobDataSize);
    }
};

struct Material : public AlcPrecompiledData {
    Material(Hash_T _hash) : AlcPrecompiledData(_hash, MATERIAL) {}

    struct Properties {
        float ambient[3];
        float diffuse[3];
        float specular[3];
        float transmittance[3];
        float emission[3];
        float dissolve;
    };

    Properties properties;

    struct TextureHashes {
        Hash_T ambient;
        Hash_T diffuse;
        Hash_T specular;
        Hash_T specular_highlight;
        Hash_T bump;
        Hash_T displacement;
        Hash_T alpha;
        Hash_T reflection;
    };

    TextureHashes textures;

    uint64_t assetSize() override { 
        return sizeof(Header) 
            + sizeof(Properties) 
            + sizeof(TextureHashes);
    }

    Material(char* blobData, uint64_t blobDataSize) : AlcPrecompiledData(blobData, blobDataSize) {
        uint64_t index = sizeof(Header);

        copy(&properties, blobData, sizeof(Properties), &index, blobDataSize);
        copy(&textures, blobData, sizeof(TextureHashes), &index, blobDataSize);
    }
};

struct Mesh : public AlcPrecompiledData {
    Mesh(Hash_T _hash) : AlcPrecompiledData(_hash, MESH) {}

    uint64_t verticeCount = 0;
    Vertex* vertices = nullptr;

    struct SubMesh {
        Hash_T materialHash;

        uint64_t indiceCount = 0;
        uint32_t* indices = nullptr;

        uint64_t size() { return sizeof(Hash_T) + sizeof(uint64_t) + sizeof(uint32_t) * indiceCount; }
    };

    uint64_t subMeshCount = 0;
    SubMesh* subMeshes = nullptr;

    uint64_t assetSize() override {
        uint64_t subMeshTotalSize = 0;
        for(uint64_t i = 0; i < subMeshCount; i++) {
            subMeshTotalSize += subMeshes[i].size();
        }
        return sizeof(Header) + sizeof(uint64_t) + sizeof(Vertex) * verticeCount + sizeof(uint64_t) + subMeshTotalSize;
    }

    Mesh(char* blobData, uint64_t blobDataSize) : AlcPrecompiledData(blobData, blobDataSize) {
        uint64_t index = sizeof(Header);

        copy(&verticeCount, blobData, sizeof(uint64_t), &index, blobDataSize);
        vertices = new Vertex[verticeCount];
        copy(vertices, blobData, sizeof(Vertex) * verticeCount, &index, blobDataSize);

        copy(&subMeshCount, blobData, sizeof(uint64_t), &index, blobDataSize);
        subMeshes = new SubMesh[subMeshCount];

        for(uint64_t i = 0; i < subMeshCount; i++) {
            copy(&subMeshes[i].materialHash, blobData, sizeof(Hash_T), &index, blobDataSize);
            copy(&subMeshes[i].indiceCount, blobData, sizeof(uint64_t), &index, blobDataSize);
            subMeshes[i].indices = new uint32_t[subMeshes[i].indiceCount];
            copy(subMeshes[i].indices, blobData, sizeof(uint32_t) * subMeshes[i].indiceCount, &index, blobDataSize);
        }
    }
};

#endif