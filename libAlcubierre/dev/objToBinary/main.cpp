#include "mesh_def.h"

const uint32_t VERSION = 1;
const uint32_t HASHSEED = 385701799u + VERSION;

#include <fstream>
#include <cassert>
#include <string>
#include <iostream>
#include <vector>
#include <unordered_map>

#include "deps/xxhash.h"
#include "deps/tiny_obj_loader.h"

std::string readFile(std::string path) {
    if(path.empty()) {
        return std::string();
    }

    std::fstream file;
    file.open(path, std::ios_base::in);

    if(!file.is_open()) {
        std::cout << "Could not open '" << path << "'" << std::endl;
        std::exit(0);
    }

    std::string r = std::string((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    file.close();
    return r;
}

struct VertexWrapper {
    Vertex actual;
    float sum;

    const uint32_t hash = XXH32(actual.pos, sizeof(float) * 3, HASHSEED);

    VertexWrapper(Vertex _actual) : actual(_actual) {
        sum = actual.pos[0] + actual.pos[1] + actual.pos[2];
    }

    bool operator==(const VertexWrapper& other) const {
        if(sum != other.sum) return false;
        for(int i = 0; i < 3; i++) {
            if(actual.pos[i] != other.actual.pos[i]) return false;
        }
        return true;
    }
};

namespace std {
    template <>
    struct hash<VertexWrapper> {
        uint32_t operator()(const VertexWrapper& vertex) const noexcept {
            return vertex.hash;
        }
    };
}

int parseInput(AlcubierreEngineMesh* mesh, std::string path_obj, std::string path_mtl) {
    std::cout << "Parsing input file(s)" << std::endl;

    std::string objData = readFile(path_obj);
    std::string mtlData = readFile(path_mtl);

    tinyobj::ObjReader reader;
    tinyobj::ObjReaderConfig readerConfig;
    reader.ParseFromString(objData, mtlData, readerConfig);

    //We try to proceed anyway. If we fail, then we fail. If we don't, then the user can analyse the debug output and decide for themselves if the output we produce is invalid.
    if(!reader.Warning().empty()) {
        std::cout << "tinyobjloader indicated a problem: " << reader.Warning() << std::endl;
    }
    if(!reader.Error().empty()) {
        std::cout << "tinyobjloader indicated an error: " << reader.Error() << std::endl;
    }

    tinyobj::attrib_t attributes = reader.GetAttrib();
    std::vector<tinyobj::shape_t> shapes = reader.GetShapes();
    std::vector<tinyobj::material_t> materials = reader.GetMaterials();

    std::vector<VertexWrapper> allVertices;

    std::cout << "Collecting vertices" << std::endl;

    for(int i = 0; i < shapes.size(); i++) {
        tinyobj::shape_t& shape = shapes[i];

        for(int j = 0; j < shape.mesh.indices.size(); j++) {
            tinyobj::index_t& index = shape.mesh.indices[j];

            float x = attributes.vertices[3 * index.vertex_index + 0];
            float y = attributes.vertices[3 * index.vertex_index + 1];
            float z = attributes.vertices[3 * index.vertex_index + 2];

            Vertex v = {
                .pos = {x, y, z}
            };

            allVertices.push_back(v);
        }
    }

    std::cout << "Collected " << std::to_string(allVertices.size()) << " vertices" << std::endl;

    std::unordered_map<VertexWrapper, int> uniqueVerticesMap;
    std::vector<Vertex> uniqueVertices;
    std::vector<uint32_t> indices;

    std::cout << "Discarding duplicate vertices:" << std::endl;

    int lastLoggedProgress = 0;
    for(int i = 0; i < allVertices.size(); i++) {
        int progress = static_cast<int>(static_cast<float>(i) / allVertices.size() * 100);
        if(progress >= lastLoggedProgress + 5) {
            std::cout << "   " << std::to_string(progress) << "%" << std::endl;
            lastLoggedProgress = progress;
        }

        if(uniqueVerticesMap.contains(allVertices[i])) {
            indices.push_back(uniqueVerticesMap[allVertices[i]]);
        } else {
            uniqueVertices.push_back(allVertices[i].actual);
            uniqueVerticesMap.emplace(allVertices[i], uniqueVertices.size() - 1);
            indices.push_back(uniqueVertices.size() - 1);
        }
    }

    int discardedCount = allVertices.size() - uniqueVertices.size();
    std::cout << "Discarded " << std::to_string(discardedCount) << " vertices, saving " << std::to_string(mesh->header.vertexSize * discardedCount) << " bytes" << std::endl;

    mesh->header.hash = XXH64(objData.data(), objData.size(), HASHSEED);
    mesh->header.version = VERSION;

    mesh->body.vertexCount = uniqueVertices.size();
    mesh->body.vertices = uniqueVertices.data();
    mesh->body.indexCount = indices.size();
    mesh->body.indices = indices.data();

    return 0;
}

int serialise(AlcubierreEngineMesh* mesh, std::string path) {
    std::cout << "Serialising to output" << std::endl;

    std::fstream file;
    file.open(path, std::ios_base::out | std::ios_base::trunc);
    if(!file.is_open()) {
        std::cout << "Could not open '" << path << "'" << std::endl;
        std::exit(0);
    }

    uint64_t bufferSize = sizeof(mesh->header) + sizeof(mesh->body.vertexCount) + mesh->header.vertexSize * mesh->body.vertexCount + sizeof(mesh->body.indexCount) + mesh->header.indexSize * mesh->body.indexCount;
    char* data = new char[bufferSize];
    uint64_t size = 0;

    file.write(reinterpret_cast<char*>(&mesh->header), sizeof(mesh->header));
    file.write(reinterpret_cast<char*>(&mesh->body.vertexCount), sizeof(uint64_t));
    file.write(reinterpret_cast<char*>(mesh->body.vertices), mesh->header.vertexSize * mesh->body.vertexCount);
    file.write(reinterpret_cast<char*>(&mesh->body.indexCount), sizeof(uint64_t));
    file.write(reinterpret_cast<char*>(mesh->body.indices), mesh->header.indexSize * mesh->body.indexCount);

    file.close();

    return 0;
}

int main(int argc, char* argv[]) {
    std::string objfile, mtlfile, outfile;

    if(argc == 3) {
        objfile = std::string(argv[1]);
        mtlfile = std::string();
        outfile = std::string(argv[2]);
    } else if(argc == 4) {
        objfile = std::string(argv[1]);
        mtlfile = std::string(argv[2]);
        outfile = std::string(argv[3]);
    } else {
        std::cout << "Correct usage: '" << argv[0] << "[input.obj] [input.mtl (optional)] [output.h]'" << std::endl;
        std::exit(0);
    }

    AlcubierreEngineMesh mesh = AlcubierreEngineMesh();

    parseInput(&mesh, objfile, mtlfile);
    serialise(&mesh, outfile);

    std::cout << "Done!" << std::endl;

    return 0;
}