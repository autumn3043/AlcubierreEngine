#include "private.h"

#include "CLI/CLI.hpp"

int main(int argc, char** argv) {
    CLI::App cliInputs;
    argv = cliInputs.ensure_utf8(argv);

    cliInputs.add_flag("-q,--quiet", QUIET, "Disable log output");
    std::vector<std::string> objectFilePaths;
    cliInputs.add_option("-o,--object", objectFilePaths, "Object (mesh) file path(s). Valid filetypes include: " + SUPPORTED_MESH_FILETYPES);
    std::vector<std::string> textureFilePaths;
    cliInputs.add_option("-t,--texture", textureFilePaths, "Texture file path(s). Valid filetypes include: " + SUPPORTED_TEXTURE_FILETYPES);
    cliInputs.add_option("-a,--assets", ASSETS_OUTPUT_DIRECTORY_PATH, "Assets output directory path");

    CLI11_PARSE(cliInputs, argc, argv);

    for(int i = 0; i < textureFilePaths.size(); i++) parseTexture(textureFilePaths[i]);
    for(int i = 0; i < objectFilePaths.size(); i++) parseObject(objectFilePaths[i]);
    return 0;
}

#include <iostream>

int log(std::string message) {
    if(QUIET) return 1;
    else {
        return debuglog(message);
    }
}

int debuglog(std::string message) {
    std::cout << message << std::endl;

    return 0;
}

#include <fstream>

std::string stringFromFilePath(std::string path) {
    if(path.empty()) {
        return std::string();
    }

    std::fstream file;
    file.open(path, std::ios_base::in);

    if(!file.is_open()) {
        debuglog("Could not open file at path '" + path + "'");
        std::exit(1);
    }

    std::string r = std::string((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    file.close();
    return r;
}

#include "xxhash.h"

Hash_T generateHash(void* data, uint64_t dataSize) {
    return XXH64(data, dataSize, HASHSEED);
}

Hash_T generateHash(std::string& data) {
    return generateHash(data.data(), data.size());
}

Hash_T generateHash(Vertex& data) {
    float allData[8];

    memcpy(allData, &data.pos.val, sizeof(float) * 3);
    memcpy(allData + 3, &data.normal.val, sizeof(float) * 3);
    memcpy(allData + 3, &data.uv.val, sizeof(float) * 2);
    
    return generateHash(allData, sizeof(float) * 8);
}

#include <filesystem>

#include "ktx.h"

int writeBinaryToFile(std::fstream& file, const void* data, uint64_t dataSize) {
    file.write(reinterpret_cast<const char*>(data), dataSize);

    return 0;
}

int parseTexture(std::string& textureFilePath) {
    std::string filename = std::filesystem::path(textureFilePath).stem().string();
    Hash_T hash = generateHash(filename);
    Texture texture = Texture(hash);

    ktxTexture2* data;
    ktxTexture2_CreateFromNamedFile(textureFilePath.c_str(), KTX_TEXTURE_CREATE_LOAD_IMAGE_DATA_BIT, &data);
    ktxTexture2_WriteToMemory(data, &texture.texelData, &texture.texelDataSize);

    std::string path = ASSETS_OUTPUT_DIRECTORY_PATH + "textures/" + std::to_string(hash) + ".blob";
    std::filesystem::create_directories(std::filesystem::path(path).parent_path());
    std::fstream file;
    file.open(path, std::ios_base::out | std::ios_base::trunc | std::ios_base::binary);
    if(!file.is_open()) {
        debuglog("Could not create/open output '" + path + "' for asset " + filename);
        return 1;
    }

    writeBinaryToFile(file, &texture.header, sizeof(texture.header));
    writeBinaryToFile(file, &texture.texelDataSize, sizeof(texture.texelDataSize));
    writeBinaryToFile(file, texture.texelData, texture.texelDataSize);

    file.close();

    ktxTexture2_Destroy(data);

    return 0;
}

#include "tiny_obj_loader.h"

int parseMaterial(tinyobj::material_t& data) {
    Hash_T hash = generateHash(data.name);
    Material material = Material(hash);

    for(int i = 0; i < 3; i++) {
        material.properties.ambient[i] = static_cast<float>(data.ambient[i]);
        material.properties.diffuse[i] = static_cast<float>(data.diffuse[i]);
        material.properties.specular[i] = static_cast<float>(data.specular[i]);
        material.properties.transmittance[i] = static_cast<float>(data.transmittance[i]);
        material.properties.emission[i] = static_cast<float>(data.emission[i]);
    }
    material.properties.dissolve = data.dissolve;

    material.textures.ambient = generateHash(data.ambient_texname);
    material.textures.diffuse = generateHash(data.diffuse_texname);
    material.textures.specular = generateHash(data.specular_texname);
    material.textures.specular_highlight = generateHash(data.specular_highlight_texname);
    material.textures.bump = generateHash(data.bump_texname);
    material.textures.displacement = generateHash(data.displacement_texname);
    material.textures.alpha = generateHash(data.alpha_texname);
    material.textures.reflection = generateHash(data.reflection_texname);

    std::string path = ASSETS_OUTPUT_DIRECTORY_PATH + "materials/" + std::to_string(hash) + ".blob";
    std::filesystem::create_directories(std::filesystem::path(path).parent_path());
    std::fstream file;
    file.open(path, std::ios_base::out | std::ios_base::trunc | std::ios_base::binary);
    if(!file.is_open()) {
        debuglog("Could not create/open output '" + path + "' for asset " + data.name);
        return 1;
    }

    writeBinaryToFile(file, &material.header, sizeof(material.header));
    writeBinaryToFile(file, &material.properties, sizeof(material.properties));
    writeBinaryToFile(file, &material.textures, sizeof(material.textures));

    file.close();

    return 0;
}

int parseObject(std::string& meshFilePath) {
    tinyobj::ObjReader reader;
    tinyobj::ObjReaderConfig readerConfig;
    readerConfig.mtl_search_path = std::filesystem::path(meshFilePath).parent_path().string();
    reader.ParseFromFile(meshFilePath, readerConfig);

    tinyobj::attrib_t attributes = reader.GetAttrib();
    std::vector<tinyobj::shape_t> shapes = reader.GetShapes();
    std::vector<tinyobj::material_t> materials = reader.GetMaterials();

    std::vector<Vertex> uniqueVertices;
    std::unordered_map<Hash_T, int> uniqueVerticesMap;
    std::unordered_map<int, std::vector<uint32_t>> materialsMap;

    for(int i = 0; i < shapes.size(); i++) {
        int indexIntoIndices = 0;
        for(int j = 0; j < shapes[i].mesh.num_face_vertices.size(); j++) {
            if(shapes[i].mesh.num_face_vertices[j] == 3) {
                for(int k = 0; k < 3; k++) {
                    tinyobj::index_t index = shapes[i].mesh.indices[indexIntoIndices + k];

                    Vertex vert;
                    vert.pos = {attributes.vertices[index.vertex_index * 3], attributes.vertices[index.vertex_index * 3 + 1], attributes.vertices[index.vertex_index * 3 + 2]};
                    if(index.normal_index >= 0) {
                        vert.normal = {attributes.normals[index.normal_index * 3], attributes.normals[index.normal_index * 3 + 1], attributes.normals[index.normal_index * 3 + 2]};
                    }
                    if(index.texcoord_index >= 0) {
                        vert.uv = {attributes.texcoords[index.texcoord_index * 2], attributes.texcoords[index.texcoord_index * 2 + 1]};
                    }
                    
                    Hash_T vertHash = generateHash(vert);

                    if(!uniqueVerticesMap.contains(vertHash)) {
                        uniqueVertices.emplace_back(vert);
                        uniqueVerticesMap.emplace(vertHash, uniqueVertices.size() - 1);
                    }

                    if(!materialsMap.contains(shapes[i].mesh.material_ids[j])) {
                        materialsMap.emplace(shapes[i].mesh.material_ids[j], std::vector<uint32_t>{});
                    }

                    materialsMap[shapes[i].mesh.material_ids[j]].push_back(uniqueVerticesMap[vertHash]);
                }
            }

            indexIntoIndices += shapes[i].mesh.num_face_vertices[j];
        }
    }

    std::string filename = std::filesystem::path(meshFilePath).filename().string();
    Hash_T hash = generateHash(filename);
    Mesh mesh = Mesh(hash);

    mesh.verticeCount = uniqueVertices.size();
    mesh.vertices = new Vertex[mesh.verticeCount];
    memcpy(mesh.vertices, uniqueVertices.data(), sizeof(Vertex) * mesh.verticeCount);

    mesh.subMeshCount = materialsMap.size();
    mesh.subMeshes = new Mesh::SubMesh[mesh.subMeshCount];

    std::unordered_map<int, std::vector<uint32_t>>::iterator it = materialsMap.begin();
    for(uint64_t i = 0; i < mesh.subMeshCount; i++, it++) {
        if(it->first == -1) mesh.subMeshes[i].materialHash = 0;
        else {
            mesh.subMeshes[i].materialHash = generateHash(materials[it->first].name);
            parseMaterial(materials[it->first]);
        }
        mesh.subMeshes[i].indiceCount = it->second.size();
        mesh.subMeshes[i].indices = new uint32_t[mesh.subMeshes[i].indiceCount];
        memcpy(mesh.subMeshes[i].indices, it->second.data(), sizeof(uint32_t) * mesh.subMeshes[i].indiceCount);
    }

    std::string path = ASSETS_OUTPUT_DIRECTORY_PATH + "meshes/" + std::to_string(hash) + ".blob";
    std::filesystem::create_directories(std::filesystem::path(path).parent_path());
    std::fstream file;
    file.open(path, std::ios_base::out | std::ios_base::trunc | std::ios_base::binary);
    if(!file.is_open()) {
        debuglog("Could not create/open output '" + path + "' for asset " + filename);
        return 1;
    }

    writeBinaryToFile(file, &mesh.header, sizeof(mesh.header));
    writeBinaryToFile(file, &mesh.verticeCount, sizeof(mesh.verticeCount));
    writeBinaryToFile(file, mesh.vertices, sizeof(Vertex) * mesh.verticeCount);
    writeBinaryToFile(file, &mesh.subMeshCount, sizeof(mesh.subMeshCount));
    for(uint64_t i = 0; i < mesh.subMeshCount; i++) {
        Mesh::SubMesh& subMesh = mesh.subMeshes[i];
        writeBinaryToFile(file, &subMesh.materialHash, sizeof(subMesh.materialHash));
        writeBinaryToFile(file, &subMesh.indiceCount, sizeof(subMesh.indiceCount));
        writeBinaryToFile(file, subMesh.indices, sizeof(uint32_t) * subMesh.indiceCount);
    }

    file.close();

    return 0;
}