/**
 * Copyright (c) 2025 Dhani Novan
 *
 **/

#ifndef LOVE_GRAPHICS_MUGEN_SPRITE_H
#define LOVE_GRAPHICS_MUGEN_SPRITE_H

// LOVE
#include "common/Object.h"
#include "Palette.h"
#include "mugen/mugen_sff.h"
#include "graphics/Shader.h"

namespace love
{
namespace graphics
{

// Forward declarations (MUST!!)
class Graphics;
	
class MugenSprite : public Object
{
public:

	static love::Type type;
	std::string filename;
	SffHeader header;
	std::vector<Sprite> sprites;
	std::vector<Palette> palettes;
	Shader* shader;
	std::map<std::array<int, 2>, int> sprites_map;	// map of sprites from group, number to index

	MugenSprite(const char* sff_file);
	MugenSprite(std::string sff_file);
	~MugenSprite();

	void draw(Graphics* gfx, int group, int number, int x, int y);
	void draw(Graphics* gfx, Palette* pal, int group, int number, int x, int y);
	int getSpriteIndex(int group, int number);
private:

}; // MugenSprite

} // graphics
} // love

#endif // LOVE_GRAPHICS_MUGEN_SPRITE_H
