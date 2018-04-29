#ifndef PACK_MPEG2_H
#define PACK_MPEG2_H

#include <fstream>
#include <vector>

#include <cstdint>

bool pack_mpeg2(std::ifstream& mpeg2_file, std::vector<uint8_t>& videoStream, std::vector<uint8_t>& audioStream);

#endif // PACK_MPEG2_H
