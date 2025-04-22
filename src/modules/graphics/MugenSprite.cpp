/**
 * Copyright (c) 2025 Dhani Novan
 *
 **/

#include "MugenSprite.h"
#include "Graphics.h"
#include "image/ImageData.h"
#include "common/Exception.h"
#include "mugen/mugen_sff.h"
#include <stdio.h>

namespace love
{
namespace graphics
{

// Define the static type member
love::Type MugenSprite::type("MugenSprite", &Object::type);

// Constructor: Load the sprite from an SFF file (C-string version)
MugenSprite::MugenSprite(const char* sff_file)
{
    bool character = true;

    if (sff_file == nullptr) {
        throw love::Exception("SFF file path is null!");
    }
    
    FILE* file = fopen(sff_file, "r");
    if (!file) {
        // Handle error: file could not be opened
        throw love::Exception("Failed to open SFF file");
    }

    // Copy filename to the class member
    filename = sff_file;

    // Read the SFF file header and initialize the sprite
    uint32_t lofs, tofs;
    if (readSffHeader(this, file, &lofs, &tofs) != 0) {
        fclose(file);
        throw love::Exception("Failed to read SFF header: %s", sff_file);
    }

    // Print version
    if (this->header.Ver0 != 1) {
        std::map<std::array<int, 2>, int> uniquePals;
        this->palettes.clear();
        this->palettes.resize(this->header.NumberOfPalettes);
        for (int i = 0; i < this->header.NumberOfPalettes; i++) {
            fseek(file, this->header.FirstPaletteHeaderOffset + i * 16, SEEK_SET);
            int16_t gn[3];
            if (fread(gn, sizeof(uint16_t), 3, file) != 3) {
                fclose(file);
                throw love::Exception("Failed to read palette group: %s", sff_file);
            }
            // printf("Palette %d: Group %d, Number %d, ColNumber %d\n", i, gn[0], gn[1], gn[2]);

            uint16_t link;
            if (fread(&link, sizeof(uint16_t), 1, file) != 1) {
                fclose(file);
                throw love::Exception("Failed to read palette link: %s", sff_file);
            }
            // printf("Palette link: %d\n", link);

            uint32_t ofs, siz;
            if (fread(&ofs, sizeof(uint32_t), 1, file) != 1) {
                fclose(file);
                throw love::Exception("Failed to read palette offset: %s", sff_file);
            }
            if (fread(&siz, sizeof(uint32_t), 1, file) != 1) {
                fclose(file);
                throw love::Exception("Failed to read palette size: %s", sff_file);
            }

            // Check if the palette is unique
            std::array<int, 2> key = { gn[0], gn[1] };
            if (uniquePals.find(key) == uniquePals.end()) {
                fseek(file, lofs + ofs, SEEK_SET);
                love::Color32 color32[256];
                if (fread(color32, sizeof(love::Color32), 256, file) != 256) {
                    fclose(file);
                    throw love::Exception("Failed to read palette data: %s", sff_file);
                }
                //Convert Color32 to Colorf
                for (int c = 0;c < 256;c++) {
                    this->palettes[i].colorf[c].r = (float)color32[c].r / 255.0f;
                    this->palettes[i].colorf[c].g = (float)color32[c].g / 255.0f;
                    this->palettes[i].colorf[c].b = (float)color32[c].b / 255.0f;
                    this->palettes[i].colorf[c].a = (float)color32[c].a / 255.0f;
                }
                uniquePals[key] = i;
            } else {
                // If the palette is not unique, use the existing one
                printf("Palette %d(%d,%d) is not unique, using palette %d\nUntested code\n", i, gn[0], gn[1], uniquePals[key]);
                // this->palettes[i].color = this->palettes[uniquePals[key]].color;
            }
        }
    }
    
    // Loading sprites and store it in the sprites vector
    this->sprites.clear();
    this->sprites.resize(this->header.NumberOfSprites);
    Sprite* prev = NULL;
    long shofs = this->header.FirstSpriteHeaderOffset;
    for (int i = 0; i < this->header.NumberOfSprites; i++) {
        uint32_t xofs, size;
        uint16_t indexOfPrevious;

        // initialize sprite
        memset(&this->sprites[i], 0, sizeof(Sprite));
        this->sprites[i].palidx = -1;

        fseek(file, shofs, SEEK_SET);
        switch (this->header.Ver0) {
        case 1:
            if (readSpriteHeaderV1(&this->sprites[i], file, &xofs, &size, &indexOfPrevious) != 0) {
                fclose(file);
                throw love::Exception("Failed to read SFFv1 header: %s", sff_file);
            }
            break;
        case 2:
            if (readSpriteHeaderV2(&this->sprites[i], file, &xofs, &size, lofs, tofs, &indexOfPrevious) != 0) {
                fclose(file);
                throw love::Exception("Failed to read SFFv2 header: %s", sff_file);
            }
            // printf("readSpriteHeaderV2(%d: %d,%d) xofs=%d size=%d lofs=%d tofs=%d indexOfPrevious=%d\n", i, this->sprites[i]->Group, this->sprites[i]->Number, xofs, size, lofs, tofs, indexOfPrevious);
            // printf("readSpriteHeaderV2(%d: %d,%d) palidx=%d size=%d indexOfPrevious=%d\n", i, this->sprites[i]->Group, this->sprites[i]->Number, this->sprites[i]->palidx, size, indexOfPrevious);
            break;
        }

        if (size == 0) {
            if (indexOfPrevious < i) {
                spriteCopy(&this->sprites[i], &this->sprites[indexOfPrevious]);
                // printf("Info: Sprite[%d] use prev Sprite[%d]\n", i, indexOfPrevious);
            } else {
                fclose(file);
                throw love::Exception("Invalid sprite index: %s", sff_file);
            }
        } else {
            switch (this->header.Ver0) {
            case 1:
                if (this->sprites[i].Group == 0 && this->sprites[i].Number == 0) {
                    character = false;
                }
                // printf("Sprite[%d] (%d,%d) ", i, this->sprites[i]->Group, this->sprites[i]->Number);
                if (readSpriteDataV1(&this->sprites[i], file, this, shofs + 32, size, xofs, prev, character) != 0) {
                    fclose(file);
                    throw love::Exception("Failed to read SFFv1 data: %s", sff_file);
                }
                break;
            case 2:
                if (readSpriteDataV2(&this->sprites[i], file, xofs, size, this) != 0) {
                    fclose(file);
                    throw love::Exception("Failed to read SFFv2 data: %s", sff_file);
                }
                break;
            }

            // if use previous sprite Group 9000 and Number 0 only (fix for SFF v1)
            if (this->sprites[i].Group == 9000) {
                if (this->sprites[i].Number == 0) {
                    prev = &this->sprites[i];
                }
            } else {
                prev = &this->sprites[i];
            }
        }

        if (this->header.Ver0 == 1) {
            shofs = xofs;
        } else {
            shofs += 28;
        }
    }
    // if SFF == v1 then update total palette
    if (this->header.Ver0 == 1) {
        this->header.NumberOfPalettes = this->palettes.size();
    }

    // Clean up the file
    fclose(file);

    // Setup shader
    std::string vertexShader;
    std::string pixelShader = R"(
#version 330 core

uniform sampler2D texture0;
uniform vec4 palette[256];

in vec2 textureCoords;
out vec4 fragColor;

void main() {
    float indexF = texture(texture0, textureCoords).r;
    int index = int(indexF * 255.0 + 0.5); // +0.5 for rounding
    fragColor = palette[index];
})";
    auto gfx = Module::getInstance<graphics::Graphics>(Module::M_GRAPHICS);
    shader = gfx->newShader(vertexShader, pixelShader);

    // Print the loaded sprite information
    printf("Loaded SFF file: %s\n", sff_file);
    printf("Version: %d.%d.%d.%d\n", this->header.Ver0, this->header.Ver1, this->header.Ver2, this->header.Ver3);
    printf("Number of Sprites: %d\n", this->header.NumberOfSprites);
    printf("Number of Palettes: %d\n\n", this->header.NumberOfPalettes);

    // Print last sprite info
    uint32_t last_index = this->header.NumberOfSprites - 1;
    printf("0 "); printSprite(&this->sprites[0]);
    printf("%d ", last_index); printSprite(&this->sprites[last_index]);
}

// Constructor: Load the sprite from an SFF file (std::string version)
MugenSprite::MugenSprite(std::string sff_file)
    : MugenSprite(sff_file.c_str()) // Delegate to the C-string constructor
{
}

void MugenSprite::draw(Graphics* gfx, int group, int number, int x, int y) {
    image::ImageData* img = nullptr;
    
    // gfx->setShader(nullptr);
    // gfx->draw(img, ;
}

void MugenSprite::draw(Graphics* gfx, Palette* pal, int group, int number, int x, int y) {
    gfx->setShader(nullptr);
}

int MugenSprite::getSpriteIndex(int group, int number) {
    // Check if the sprite exists in the map
    auto it = sprites_map.find({group, number});
    if (it != sprites_map.end()) {
        return it->second;
    } else {
        // If not found, return an invalid index
        // return -1;
        return 2;
    }
}

// Destructor
MugenSprite::~MugenSprite()
{
    // TODO: Clean up any allocated resources if necessary
}

} // namespace graphics
} // namespace love