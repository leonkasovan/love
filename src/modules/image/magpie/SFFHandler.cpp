/**
 * Copyright (c) 2006-2024 LOVE Development Team
 *
 * This software is provided 'as-is', without any express or implied
 * warranty.  In no event will the authors be held liable for any damages
 * arising from the use of this software.
 *
 * Permission is granted to anyone to use this software for any purpose,
 * including commercial applications, and to alter it and redistribute it
 * freely, subject to the following restrictions:
 *
 * 1. The origin of this software must not be misrepresented; you must not
 *    claim that you wrote the original software. If you use this software
 *    in a product, an acknowledgment in the product documentation would be
 *    appreciated but is not required.
 * 2. Altered source versions must be plainly marked as such, and must not be
 *    misrepresented as being the original software.
 * 3. This notice may not be removed or altered from any source distribution.
 **/

#include "SFFHandler.h"

namespace love
{
namespace image
{
namespace magpie
{

bool SFFHandler::canDecode(Data* data)
{
	const unsigned char* in = (const unsigned char*) data->getData();
	if (in[0] != 'E' || in[1] != 'l' || in[2] != 'e' || in[3] != 'c'
		|| in[4] != 'b' || in[5] != 'y' || in[6] != 't' || in[7] != 'e'
		|| in[8] != 'S' || in[9] != 'p' || in[10] != 'r' || in[11] != 0) {
		return false; /*error: the first 12 bytes are not the correct SFF signature*/
	}
	return true;
}

FormatHandler::DecodedImage SFFHandler::decode(Data *data)
{
	DecodedImage img;
	Sff sff;
	uint16_t index = 20;	// index of sprite to decode

	// Decode the SFF file
	if (loadSff(&sff, (uint8_t*) data->getData())) {
		throw love::Exception("Could not decode SFF");
	}
	// img.width = sff.sprites[index]->Size[0];
	// img.height = sff.sprites[index]->Size[1];
	// img.size = img.width * img.height;
	// img.data = sff.sprites[index]->data;
	// img.format = PIXELFORMAT_R8;

	img.width = sff.sprites[index]->Size[0];
	img.height = sff.sprites[index]->Size[1];
	img.size = img.width * img.height * 4;
	if (sff.header.Ver0 == 2 && (sff.sprites[index]->rle == -12 || sff.sprites[index]->rle == -11)) {
		img.data = sff.sprites[index]->data;
		img.format = PIXELFORMAT_RGBA8;
	} else if (sff.header.Ver0 == 2 && sff.sprites[index]->rle == -10) {
		img.data = (unsigned char*) malloc(img.size);
		img.format = PIXELFORMAT_RGBA8;
		for (int y = 0; y < img.height; y++) {
			for (int x = 0; x < img.width; x++) {
				int i = y * img.width + x;
				int palIdx = sff.sprites[index]->data[i];
				uint32_t* pal = sff.palList.palettes[sff.sprites[index]->palidx];
				img.data[i * 4 + 0] = (pal[palIdx] >> 16) & 0xFF;
				img.data[i * 4 + 1] = (pal[palIdx] >> 8) & 0xFF;
				img.data[i * 4 + 2] = (pal[palIdx]) & 0xFF;
				img.data[i * 4 + 3] = (pal[palIdx] >> 24) & 0xFF;
			}
		}
	} else {
		img.data = (unsigned char*) malloc(img.size);
		// Convert indexed color (R8) sff.sprites[index]->data to RGBA
		img.format = PIXELFORMAT_RGBA8;
		for (int y = 0; y < img.height; y++) {
			for (int x = 0; x < img.width; x++) {
				int i = y * img.width + x;
				int palIdx = sff.sprites[index]->data[i];
				if (sff.header.Ver0 == 1) {
					Color32* pal = sff.palettes[sff.sprites[index]->palidx];
					img.data[i * 4 + 0] = pal[palIdx].r;
					img.data[i * 4 + 1] = pal[palIdx].g;
					img.data[i * 4 + 2] = pal[palIdx].b;
					img.data[i * 4 + 3] = pal[palIdx].a;
				} else {
					uint32_t* pal = sff.palList.palettes[sff.sprites[index]->palidx];
					img.data[i * 4 + 0] = (pal[palIdx] >> 16) & 0xFF;
					img.data[i * 4 + 1] = (pal[palIdx] >> 8) & 0xFF;
					img.data[i * 4 + 2] = (pal[palIdx]) & 0xFF;
					img.data[i * 4 + 3] = (pal[palIdx] >> 24) & 0xFF;
				}
			}
		}
	}
	
	return img;
}

void SFFHandler::freeRawPixels(unsigned char* mem) {
	// SFF uses malloc, realloc, and free.
	if (mem)
		::free(mem);
}

} // magpie
} // image
} // love
