/**
 * Copyright (c) 2025 Dhani Novan
 *
 **/

#ifndef LOVE_GRAPHICS_MUGEN_SPRITE_H
#define LOVE_GRAPHICS_MUGEN_SPRITE_H

// LOVE
#include "common/Object.h"

namespace love
{
namespace graphics
{

class MugenSprite : public Object
{
public:

	static love::Type type;

	MugenSprite(const char* sff_file);
	MugenSprite(std::string sff_file);
	~MugenSprite();

private:

}; // MugenSprite

} // graphics
} // love

#endif // LOVE_GRAPHICS_MUGEN_SPRITE_H
