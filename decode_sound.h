#ifndef DECODE_SOUND_H
#define DECODE_SOUND_H

#include <string>
#include <vector>
#include <cstdint>

#include <AL/al.h>

void decode_psx(uint8_t* encodedData, int16_t * outbuf, int32_t samples_to_do);
void decode_psx_mono(uint8_t* encodedData, int16_t * outbuf, int32_t samples_to_do);

ALuint loadAST(const std::string& filepath);
std::vector<ALuint> loadVB2(const std::string& filepath);

void decode_ads(std::vector<uint8_t>& buffer);

#endif // DECODE_SOUND_H
