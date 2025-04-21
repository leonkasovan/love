#pragma once

// LodePNG
#include "lodepng/lodepng.h"
#include <zlib.h>

// C headers
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

// C++ headers
#include <map>
#include <vector>
#include <array>
#include <string>
#include <algorithm>

// LOVE headers
#include "common/Color.h"
#include "image/Image.h"
#include "graphics/Palette.h"

#define MAX_PAL_NO 512

namespace love {
namespace graphics {

// Forward declarations (MUST!!)
class MugenSprite;
class Image;

// SFF
typedef struct {
	uint8_t Ver3, Ver2, Ver1, Ver0;
	uint32_t FirstSpriteHeaderOffset;
	uint32_t FirstPaletteHeaderOffset;
	uint32_t NumberOfSprites;
	uint32_t NumberOfPalettes;
} SffHeader;

typedef struct {
	uint32_t palettes[MAX_PAL_NO][256];
	int paletteMap[MAX_PAL_NO];
	int numPalettes;
} PaletteList;

typedef struct {
	uint16_t Group;
	uint16_t Number;
	uint16_t Size[2];
	int16_t Offset[2];
	int palidx;
	int rle;
	uint8_t coldepth;
	// uint8_t* data;
	love::graphics::Image* image;
	size_t atlas_x, atlas_y;
} Sprite;

typedef struct {
	SffHeader header;
	Sprite** sprites;
	char filename[256];
	PaletteList palList;    // SFF v2
	std::vector<love::Color32*> palettes;   // SFF v1
	std::map<int, int> palette_usage;
	std::map<int, int> format_usage;
	size_t numLinkedSprites;
} Sff;

typedef struct {
	uint16_t width, height;
	struct stbrp_rect* rects;
	Sff* sff;
	int usePalette;
} Atlas;

// Load from memory
// int loadSff(Sff* sff, uint8_t* data);
// int readSpriteDataV1(Sprite* s, u_int8_t* data, Sff* sff, uint64_t offset, uint32_t datasize, uint32_t nextSubheader, Sprite* prev, std::vector<love::Color32*>* palettes, bool c00, bool paletteSame);
// uint32_t readSpriteDataV2(Sprite* s, Sff* data, uint64_t offset, uint32_t datasize, MugenSprite* sff);

// Load from file
int readSffHeader(MugenSprite* sff, FILE* file, uint32_t* lofs, uint32_t* tofs);
int readSpriteHeaderV1(Sprite* sprite, FILE* file, uint32_t* ofs, uint32_t* size, uint16_t* link);
int readSpriteHeaderV2(Sprite* sprite, FILE* file, uint32_t* ofs, uint32_t* size, uint32_t lofs, uint32_t tofs, uint16_t* link);
int readSpriteDataV1(Sprite* s, FILE* file, MugenSprite* sff, uint64_t offset, uint32_t datasize, uint32_t nextSubheader, Sprite* prev, std::vector<Palette>* palettes, bool c00);
int readSpriteDataV2(Sprite* s, FILE* file, uint64_t offset, uint32_t datasize, MugenSprite* sff);

void spriteCopy(Sprite* dst, const Sprite* src);
void printSprite(Sprite* sprite);
}
}