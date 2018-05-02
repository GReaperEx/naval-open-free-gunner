#ifndef PAK_UNPACK_H
#define PAK_UNPACK_H

#include "VCD_input.h"

bool pak_unpack(const std::string& pakFile, VCD_fs* fs);
bool pak_unpack(VCD_FileTree* leaf, VCD_fs* fs);

#endif // PAK_UNPACK_H
