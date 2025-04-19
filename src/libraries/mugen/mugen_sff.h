
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

#define MAX_PAL_NO 512

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
	uint32_t* Pal;
	uint16_t Group;
	uint16_t Number;
	uint16_t Size[2];
	int16_t Offset[2];
	int palidx;
	int rle;
	uint8_t coldepth;
	uint8_t* data;
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

int loadSff(Sff* sff, uint8_t* data);
