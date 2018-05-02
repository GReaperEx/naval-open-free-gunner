#ifndef VCD_INPUT_H
#define VCD_INPUT_H

#include <string>
#include <vector>
#include <fstream>

struct VCD_FileInfo
{
    char fileName[32];
    union {
        struct {
            uint32_t fileRealSize;
            uint32_t fileFsSize;
            uint32_t fileBytePos;
            uint32_t resv;
        } file;
        struct {
            uint32_t dirEntries;
            uint32_t entriesDwordPos;
            uint32_t dirSubdirs;
            uint32_t subsDwordPos;
        } dir;
    };
};

struct VCD_FileTree
{
    VCD_FileInfo info;

    bool isDir;
    std::vector<VCD_FileTree*> leafs;
};

struct VCD_fs
{
    std::ifstream imageFile;
    VCD_FileTree* root;
    uint32_t dirTableEnd;
};

VCD_fs* VCD_open(const std::string& filePath);
void VCD_close(VCD_fs* fs);

bool VCD_isDir(const VCD_FileTree* leaf);

VCD_FileTree* VCD_getDirEntry(const std::string& filePath, VCD_fs* fs);

bool VCD_inputFile(std::vector<uint8_t>& buffer, VCD_FileTree* leaf, VCD_fs* fs);
bool VCD_inputFile(std::vector<uint8_t>& buffer, const std::string& filePath, VCD_fs* fs);

#endif // VCD_INPUT_H
