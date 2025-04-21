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
	img.width = 200;
	img.height = 100;
	img.size = img.width * img.height;
	img.data = (unsigned char*) ::malloc(img.size);
	img.format = PIXELFORMAT_R8;

	unsigned char* p = img.data;
	for (int i = 0;i < 100;i++)
		for (int j = 0; j < img.width; j++) {
			*p++ = i & 2 ? 0 : 1;
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
