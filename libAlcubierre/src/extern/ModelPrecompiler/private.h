#ifndef ALCUBIERRE_EXTERN_MODELPRECOMPILER_PRIVATE_H
#define ALCUBIERRE_EXTERN_MODELPRECOMPILER_PRIVATE_H

#include "public.h"

const std::string SUPPORTED_MESH_FILETYPES = ".obj";
const std::string SUPPORTED_MATERIAL_FILETYPES = ".mtl";
const std::string SUPPORTED_TEXTURE_FILETYPES = ".png";
bool QUIET = false;
std::string ASSETS_OUTPUT_DIRECTORY_PATH = "assets/";

uint64_t HASHSEED = 238746u;

int log(std::string message);
int debuglog(std::string message);

std::string stringFromFilePath(std::string path);
Hash_T generateHash(void* data, uint64_t dataSize);
Hash_T generateHash(std::string& data);
Hash_T generateHash(Vertex& data);

int writeBinaryToFile(std::fstream& file, const void* data, uint64_t dataSize);

int parseTexture(std::string& textureFilePath);
int parseObject(std::string& objectFilePath);

#endif