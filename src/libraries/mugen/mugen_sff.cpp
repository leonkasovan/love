#include "mugen_sff.h"

Sprite* newSprite() {
	Sprite* sprite = (Sprite*) malloc(sizeof(Sprite));
	memset(sprite, 0, sizeof(Sprite));
	sprite->palidx = -1;
	return sprite;
}

uint32_t readSffHeader(Sff* sff, uint8_t* data, uint32_t* lofs, uint32_t* tofs) {
	uint32_t offset = 0;

	// Validate header by comparing the first 12 bytes with "ElecbyteSpr\x0"
	char headerCheck[12];
	memcpy(headerCheck, data + offset, 12);
	offset += 12;
	if (memcmp(headerCheck, "ElecbyteSpr\0", 12) != 0) {
		fprintf(stderr, "Invalid SFF file [%s]\n", headerCheck);
		return 0;
	}

	// Read versions in the header
	sff->header.Ver3 = *(data + offset++);
	sff->header.Ver2 = *(data + offset++);
	sff->header.Ver1 = *(data + offset++);
	sff->header.Ver0 = *(data + offset++);

	uint32_t dummy;
	memcpy(&dummy, data + offset, sizeof(uint32_t));
	offset += sizeof(uint32_t);

	if (sff->header.Ver0 == 2) {
		for (int i = 0; i < 4; i++) {
			memcpy(&dummy, data + offset, sizeof(uint32_t));
			offset += sizeof(uint32_t);
		}

		// Read FirstSpriteHeaderOffset
		memcpy(&sff->header.FirstSpriteHeaderOffset, data + offset, sizeof(uint32_t));
		offset += sizeof(uint32_t);

		// Read NumberOfSprites
		memcpy(&sff->header.NumberOfSprites, data + offset, sizeof(uint32_t));
		offset += sizeof(uint32_t);

		// Read FirstPaletteHeaderOffset
		memcpy(&sff->header.FirstPaletteHeaderOffset, data + offset, sizeof(uint32_t));
		offset += sizeof(uint32_t);

		// Read NumberOfPalettes
		memcpy(&sff->header.NumberOfPalettes, data + offset, sizeof(uint32_t));
		offset += sizeof(uint32_t);

		// Read lofs
		memcpy(lofs, data + offset, sizeof(uint32_t));
		offset += sizeof(uint32_t);

		memcpy(&dummy, data + offset, sizeof(uint32_t));
		offset += sizeof(uint32_t);

		// Read tofs
		memcpy(tofs, data + offset, sizeof(uint32_t));
		offset += sizeof(uint32_t);
	} else if (sff->header.Ver0 == 1) {
		// Read NumberOfSprites
		memcpy(&sff->header.NumberOfSprites, data + offset, sizeof(uint32_t));
		offset += sizeof(uint32_t);

		// Read FirstSpriteHeaderOffset
		memcpy(&sff->header.FirstSpriteHeaderOffset, data + offset, sizeof(uint32_t));
		offset += sizeof(uint32_t);

		sff->header.FirstPaletteHeaderOffset = 0;
		sff->header.NumberOfPalettes = 0;
		*lofs = 0;
		*tofs = 0;
	} else {
		fprintf(stderr, "Unsupported SFF version: %d\n", sff->header.Ver0);
		return 0;
	}

	return offset;
}

uint32_t readSpriteHeaderV1(Sprite* sprite, uint8_t* data, uint32_t* ofs, uint32_t* size, uint16_t* link, uint8_t* ps) {
	uint32_t offset = 0;
	memcpy(ofs, data + offset, sizeof(uint32_t));
	offset += sizeof(uint32_t);
	memcpy(size, data + offset, sizeof(uint32_t));
	offset += sizeof(uint32_t);
	memcpy(&sprite->Offset[0], data + offset, sizeof(int16_t));
	offset += sizeof(int16_t);
	memcpy(&sprite->Offset[1], data + offset, sizeof(int16_t));
	offset += sizeof(int16_t);
	memcpy(&sprite->Group, data + offset, sizeof(int16_t));
	offset += sizeof(int16_t);
	memcpy(&sprite->Number, data + offset, sizeof(int16_t));
	offset += sizeof(int16_t);
	memcpy(link, data + offset, sizeof(uint16_t));
	offset += sizeof(uint16_t);
	memcpy(ps, data + offset, sizeof(uint8_t));
	offset += sizeof(uint8_t);

	// Print sprite header information
	// printf("Sprite v1 Group, Number: %d, %d\n", sprite->Group, sprite->Number);
	return offset;
}

void spriteCopy(Sprite* dst, const Sprite* src) {
	dst->Pal = src->Pal;
	dst->Group = src->Group;
	dst->Number = src->Number;
	dst->Size[0] = src->Size[0];
	dst->Size[1] = src->Size[1];
	dst->Offset[0] = src->Offset[0];
	dst->Offset[1] = src->Offset[1];
	dst->palidx = src->palidx;
	dst->rle = src->rle;
	dst->coldepth = src->coldepth;
	// dst->data = src->data;
}

int readPcxHeader(Sprite* s, uint8_t* data, uint64_t start_offset) {
	// fseek(file, start_offset, SEEK_SET);
	uint64_t offset = start_offset;
	uint16_t dummy;
	memcpy(&dummy, data + offset, sizeof(uint16_t));
	offset += sizeof(uint16_t);

	uint8_t encoding, bpp;
	memcpy(&encoding, data + offset, sizeof(uint8_t));
	offset += sizeof(uint8_t);

	memcpy(&bpp, data + offset, sizeof(uint8_t));
	offset += sizeof(uint8_t);
	if (bpp != 8) {
		fprintf(stderr, "Invalid PCX color depth: expected 8-bit, got %d\n", bpp);
		return 0;
	}

	uint16_t rect[4];
	memcpy(rect, data + offset, sizeof(uint16_t) * 4);
	offset += sizeof(uint16_t) * 4;

	offset = start_offset + 66;
	uint16_t bpl;
	memcpy(&bpl, data + offset, sizeof(uint16_t));
	offset += sizeof(uint16_t);

	s->Size[0] = rect[2] - rect[0] + 1;
	s->Size[1] = rect[3] - rect[1] + 1;
	if (encoding == 1) {
		s->rle = bpl;
	} else {
		s->rle = 0;
	}
	return offset;
}

uint8_t* RlePcxDecode(Sprite* s, uint8_t* srcPx, size_t srcLen) {
	if (srcLen == 0) {
		fprintf(stderr, "Warning PCX data length is zero\n");
		return NULL;
	}

	int dstLen = s->Size[0] * s->Size[1];
	// printf("Allocating memory for PCX decoded data dstLen=%ld srcLen=%ld %dx%d\n", dstLen, srcLen, s->Size[0], s->Size[1]);
	uint8_t* dstPx = (uint8_t*) malloc(dstLen);
	if (!dstPx) {
		fprintf(stderr, "Error allocating memory for PCX decoded data dstLen=%ld srcLen=%ld %dx%d\n", dstLen, srcLen, s->Size[0], s->Size[1]);
		return NULL;
	}

	size_t i = 0, j = 0, k = 0, w = s->Size[0];
	while (j < dstLen) {
		int n = 1, d = srcPx[i];
		if (i < (srcLen - 1)) {
			i++;
		}
		if (d >= 0xc0) {
			n = d & 0x3f;
			d = srcPx[i];
			if (i < (srcLen - 1)) {
				i++;
			}
		}
		for (; n > 0; n--) {
			if ((k < w) && (j < dstLen)) {
				dstPx[j] = d;
				j++;
			}
			k++;
			if (k == s->rle) {
				k = 0;
				n = 1;
			}
		}
	}
	s->rle = 0;
	return dstPx;
}

int readSpriteDataV1(Sprite* s, u_int8_t* data, Sff* sff, uint64_t offset, uint32_t datasize, uint32_t nextSubheader, Sprite* prev, std::vector<love::Color32*>* palettes, bool c00, bool paletteSame) {
	if (nextSubheader > offset) {
		// Ignore datasize except last
		datasize = nextSubheader - offset;
	}

	if (!readPcxHeader(s, data, offset)) {
		fprintf(stderr, "Error reading sprite PCX header\n");
		return -1;
	}

	uint64_t p_offset = offset + 128;
	uint32_t palHash = 0;
	uint32_t palSize;
	if (c00 || paletteSame) {
		palSize = 0;
	} else {
		palSize = 768;
	}
	if (datasize < 128 + palSize) {
		datasize = 128 + palSize;
	}

	size_t srcLen = datasize - (128 + palSize);
	uint8_t* srcPx = (uint8_t*) malloc(srcLen);
	if (!srcPx) {
		fprintf(stderr, "Error allocating memory for sprite data\n");
		return -1;
	}
	memcpy(srcPx, data + p_offset, srcLen);
	p_offset += srcLen;

	s->data = NULL;
	sff->format_usage[1]++;
	// printf("PCX: ps=%d ", ps);
	if (paletteSame) {
		// png_color* png_palette;
		if (prev != NULL) {
			s->palidx = prev->palidx;
			// printf("Info: Same palette (%d,%d) with (%d,%d) = %d\n", s->Group, s->Number, prev->Group, prev->Number, prev->palidx);
		}
		if (s->palidx < 0) {
			fprintf(stderr, "Error: invalid prev palette index %d\n", prev->palidx);
			return -1;
		} else {
			// fprintf(stderr, "Warning: palette index %d\npal_len=%d\n", s->palidx, palettes->size());
			// png_palette = palettes->at(s->palidx);
			// for (int i = 0; i < 256; i++) {
			//     png_palette[i].red = pal[i].red;
			//     png_palette[i].green = pal[i].green;
			//     png_palette[i].blue = pal[i].blue;
				// png_palette[i].red = 100;
				// png_palette[i].green = 200;
				// png_palette[i].blue = 0;
			// }
		}
		uint8_t* px = RlePcxDecode(s, srcPx, srcLen);
		free(srcPx);
		if (!px) {
			fprintf(stderr, "Error decoding PCX sprite data\n");
			return -1;
		}
		// palHash = fast_hash(pal, 256);
		// printf("old_pal=%d\n", s->palidx);
		s->data = px;
	} else {
		love::Color32* new_palette = new love::Color32[256];
		if (c00 || paletteSame) {
			p_offset = offset + datasize - 768;
		}

		uint8_t rgb[3];
		// printf("palidx=%u start offset=%u\n", palettes->size(), p_offset);
		for (int i = 0;i < 256;i++) {
			// if (fread(rgb, sizeof(uint8_t), 3, file) != 3) {
			// 	fprintf(stderr, "Error reading palette rgb data\n");
			// 	return -1;
			// }
			memcpy(rgb, data + p_offset, sizeof(uint8_t) * 3);
			p_offset += sizeof(uint8_t) * 3;
			new_palette[i].a = 255;
			if (i == 0) {
				new_palette[i].a = 0;
			}
			new_palette[i].r = rgb[0];
			new_palette[i].g = rgb[1];
			new_palette[i].b = rgb[2];
		}
		// printf("palidx=%u end offset=%u\n", palettes->size(), p_offset);
		palettes->push_back(new_palette);
		s->palidx = palettes->size() - 1;
		// savePalette(pal, fmt.Sprintf("%v %v %v.act", "char_pal", s.Group, s.Number))
		// printf("[DEBUG] src/main.cpp:%d\n", __LINE__);
		uint8_t* px = RlePcxDecode(s, srcPx, srcLen);
		free(srcPx);
		if (!px) {
			fprintf(stderr, "Error decoding PCX sprite data\n");
			return -1;
		}
		// printf("[DEBUG] src/main.cpp:%d\n", __LINE__);
		// palHash = fast_hash(pal, 256);
		// printf("new_pal=%d\n", s->palidx);
		// if (opt_extract) save_as_png(pngFilename, s->Size[0], s->Size[1], px, png_palette);
		s->data = px;
		// free(px);
	}
	// printf("[DEBUG] src/main.cpp:%d ps=%d paletteSame=%d palidx=%d palLen=%d palSize=%d srcLen=%ld\n", __LINE__, ps, paletteSame, s->palidx, palettes->size(), palSize, srcLen);
	// printf("%u\n", palHash);
	sff->palette_usage[s->palidx]++;
	return 0;
}

uint32_t readSpriteHeaderV2(Sprite* sprite, uint8_t* data, uint32_t* ofs, uint32_t* size, uint32_t lofs, uint32_t tofs, uint16_t* link) {
	uint32_t offset = 0;
	memcpy(&sprite->Group, data + offset, sizeof(int16_t));
	offset += sizeof(int16_t);

	memcpy(&sprite->Number, data + offset, sizeof(int16_t));
	offset += sizeof(int16_t);

	// Read the sprite size
	memcpy(&sprite->Size[0], data + offset, sizeof(int16_t));
	offset += sizeof(int16_t);
	memcpy(&sprite->Size[1], data + offset, sizeof(int16_t));
	offset += sizeof(int16_t);

	// Read the sprite offset
	memcpy(&sprite->Offset[0], data + offset, sizeof(int16_t));
	offset += sizeof(int16_t);
	memcpy(&sprite->Offset[1], data + offset, sizeof(int16_t));
	offset += sizeof(int16_t);

	// Read the link to the next sprite header
	memcpy(link, data + offset, sizeof(uint16_t));
	offset += sizeof(uint16_t);

	char format;
	memcpy(&format, data + offset, sizeof(char));
	offset += sizeof(char);
	sprite->rle = -format;

	// Read color depth
	memcpy(&sprite->coldepth, data + offset, sizeof(uint8_t));
	offset += sizeof(uint8_t);

	// Read ofs
	memcpy(ofs, data + offset, sizeof(uint32_t));
	offset += sizeof(uint32_t);

	// Read size
	memcpy(size, data + offset, sizeof(uint32_t));
	offset += sizeof(uint32_t);

	uint16_t tmp;
	memcpy(&tmp, data + offset, sizeof(uint16_t));
	offset += sizeof(uint16_t);
	sprite->palidx = tmp;

	// Read tmp
	memcpy(&tmp, data + offset, sizeof(uint16_t));
	offset += sizeof(uint16_t);

	if ((tmp & 1) == 0) {
		*ofs += lofs;
	} else {
		*ofs += tofs;
	}

	// Print sprite header information
	// printf("Sprite v2 (%d,%d) size=%d coldepth=%d\n", sprite->Group, sprite->Number, *size, sprite->coldepth);
	// printf("Sprite Size: %d x %d\n", sprite->Size[0], sprite->Size[1]);
	// printf("Sprite Offset: %d x %d\n", sprite->Offset[0], sprite->Offset[1]);
	// printf("Sprite Link: %d\n", *link);
	// printf("Sprite Format: %d\n", format);

	return offset;
}

uint8_t* Rle8Decode(Sprite* s, uint8_t* srcPx, int srcLen) {
	if (srcLen == 0) {
		fprintf(stderr, "Warning RLE8 data length is zero\n");
		return NULL;
	}

	size_t dstLen = s->Size[0] * s->Size[1];
	uint8_t* dstPx = (uint8_t*) malloc(dstLen);
	if (!dstPx) {
		fprintf(stderr, "Error allocating memory for RLE decoded data\n");
		return NULL;
	}
	long i = 0, j = 0;
	// Decode the RLE data
	while (j < dstLen) {
		long n = 1;
		uint8_t d = srcPx[i];
		if (i < (srcLen - 1)) {
			i++;
		}
		if ((d & 0xc0) == 0x40) {
			n = d & 0x3f;
			d = srcPx[i];
			if (i < (srcLen - 1)) {
				i++;
			}
		}
		for (; n > 0; n--) {
			if (j < dstLen) {
				dstPx[j] = d;
				j++;
			}
		}
	}
	return dstPx;
}

uint8_t* Rle5Decode(Sprite* s, uint8_t* srcPx, size_t srcLen) {
	if (srcLen == 0) {
		fprintf(stderr, "Warning RLE5 data length is zero\n");
		return NULL;
	}

	int dstLen = s->Size[0] * s->Size[1];
	uint8_t* dstPx = (uint8_t*) malloc(dstLen);
	if (!dstPx) {
		fprintf(stderr, "Error allocating memory for RLE decoded data\n");
		return NULL;
	}

	size_t i = 0, j = 0;
	while (j < dstLen) {
		int rl = (int) srcPx[i];
		if (i < srcLen - 1) {
			i++;
		}
		int dl = (int) (srcPx[i] & 0x7f);
		uint8_t c = 0;
		if (srcPx[i] >> 7 != 0) {
			if (i < srcLen - 1) {
				i++;
			}
			c = srcPx[i];
		}
		if (i < srcLen - 1) {
			i++;
		}
		while (1) {
			if (j < dstLen) {
				dstPx[j] = c;
				j++;
			}
			rl--;
			if (rl < 0) {
				dl--;
				if (dl < 0) {
					break;
				}
				c = srcPx[i] & 0x1f;
				rl = (int) (srcPx[i] >> 5);
				if (i < srcLen - 1) {
					i++;
				}
			}
		}
	}

	return dstPx;
}

uint8_t* Lz5Decode(Sprite* s, uint8_t* srcPx, size_t srcLen) {
	if (srcLen == 0) {
		fprintf(stderr, "Warning LZ5 data length is zero\n");
		return NULL;
	}

	int dstLen = s->Size[0] * s->Size[1];
	uint8_t* dstPx = (uint8_t*) malloc(dstLen);
	if (!dstPx) {
		fprintf(stderr, "Error allocating memory for LZ5 decoded data\n");
		return NULL;
	}

	// Decode the LZ5 data
	long i = 0, j = 0, n = 0;
	uint8_t ct = srcPx[i], cts = 0, rb = 0, rbc = 0;
	if (i < srcLen - 1) {
		i++;
	}

	while (j < dstLen) {
		int d = (int) srcPx[i];
		if (i < srcLen - 1) {
			i++;
		}

		if (ct & (1 << cts)) {
			if ((d & 0x3f) == 0) {
				d = (d << 2 | (int) srcPx[i]) + 1;
				if (i < srcLen - 1) {
					i++;
				}
				n = (int) srcPx[i] + 2;
				if (i < srcLen - 1) {
					i++;
				}
			} else {
				rb |= (uint8_t) ((d & 0xc0) >> rbc);
				rbc += 2;
				n = (int) (d & 0x3f);
				if (rbc < 8) {
					d = (int) srcPx[i] + 1;
					if (i < srcLen - 1) {
						i++;
					}
				} else {
					d = (int) rb + 1;
					rb = rbc = 0;
				}
			}
			for (;;) {
				if (j < dstLen) {
					dstPx[j] = dstPx[j - d];
					j++;
				}
				n--;
				if (n < 0) {
					break;
				}
			}
		} else {
			if ((d & 0xe0) == 0) {
				n = (int) srcPx[i] + 8;
				if (i < srcLen - 1) {
					i++;
				}
			} else {
				n = d >> 5;
				d &= 0x1f;
			}
			while (n-- > 0 && j < dstLen) {
				dstPx[j] = (uint8_t) d;
				j++;
			}
		}
		cts++;
		if (cts >= 8) {
			ct = srcPx[i];
			cts = 0;
			if (i < srcLen - 1) {
				i++;
			}
		}
	}

	return dstPx;
}

// Custom PNG decompression function for LodePNG, using zlib.
static unsigned zlibDecompress(unsigned char** out, size_t* outsize, const unsigned char* in,
	size_t insize, const LodePNGDecompressSettings* /*settings*/) {
	int status = Z_OK;

	uLongf outdatasize = insize;
	size_t sizemultiplier = 0;
	unsigned char* outdata = out != nullptr ? *out : nullptr;

	while (true) {
		// Enough size to hold the decompressed data, hopefully.
		outdatasize = insize << (++sizemultiplier);

		// LodePNG uses malloc, realloc, and free.
		// Since version 2014-08-23, LodePNG passes in an existing pointer in
		// the 'out' argument that it expects to be realloc'd. Not doing so can
		// result in a memory leak.
		if (outdata != nullptr)
			outdata = (unsigned char*) realloc(outdata, outdatasize);
		else
			outdata = (unsigned char*) malloc(outdatasize);

		if (!outdata)
			return 83; // "Memory allocation failed" error code for LodePNG.

		// Use zlib to decompress the PNG data.
		status = uncompress(outdata, &outdatasize, in, insize);

		// If the out buffer was big enough, break out of the loop.
		if (status != Z_BUF_ERROR)
			break;

		// Otherwise delete the out buffer and try again with a larger size...
		free(outdata);
		outdata = nullptr;
	}

	if (status != Z_OK) {
		free(outdata);
		return 10000; // "Unknown error code" for LodePNG.
	}

	if (out != nullptr)
		*out = outdata;

	if (outsize != nullptr)
		*outsize = outdatasize;

	return 0; // Success.
}

uint8_t* PngDecode(Sprite* s, uint8_t* data, uint32_t datasize) {
	lodepng::State state;
	unsigned int width = 0, height = 0;

	unsigned status = lodepng_inspect(&width, &height, &state, data, datasize);
	if (status) {
		fprintf(stderr, "Error inspecting PNG data: %s\n", lodepng_error_text(status));
		return NULL;
	}

	state.decoder.zlibsettings.custom_zlib = zlibDecompress;
	state.info_raw.colortype = LCT_RGBA;

	if (state.info_png.color.bitdepth == 16)
		state.info_raw.bitdepth = 16;
	else
		state.info_raw.bitdepth = 8;

	u_int8_t* dstPx;
	status = lodepng_decode(&dstPx, &width, &height, &state, data, datasize);

	if (status != 0) {
		fprintf(stderr, "Could not decode PNG image(% s)", lodepng_error_text(status));
		return NULL;
	}
	s->Size[0] = width;
	s->Size[1] = height;
	return dstPx;
}

uint32_t readSpriteDataV2(Sprite* s, uint8_t* data, uint64_t offset, uint32_t datasize, Sff* sff) {
	uint8_t* px = NULL;
	uint32_t p_offset = offset;

	if (s->rle > 0) {
		fprintf(stderr, "Error: invalid compression format %d\n", -s->rle);
		return 0;
	}

	if (s->rle == 0) {
		px = (uint8_t*) malloc(datasize);
		if (!px) {
			fprintf(stderr, "Error allocating memory for sprite data\n");
			return 0;
		}
		// Read sprite data
		p_offset = offset;
		memcpy(px, data + p_offset, datasize);
		p_offset += datasize;
	} else {
		size_t srcLen;
		uint8_t* srcPx = NULL;
		p_offset = offset + 4;
		int format = -s->rle;
		int rc;

		if (2 <= format && format <= 4) {
			if (datasize < 4) {
				datasize = 4;
			}
			srcLen = datasize - 4;
			srcPx = (uint8_t*) malloc(srcLen);
			if (!srcPx) {
				fprintf(stderr, "Error allocating memory for sprite data\n");
				return 0;
			}
			memcpy(srcPx, data + p_offset, srcLen);
			p_offset += srcLen;

			sff->palette_usage[s->palidx]++;
		}

		s->data = NULL;
		sff->format_usage[format]++;
		switch (format) {
		case 2:
			px = Rle8Decode(s, srcPx, srcLen);
			free(srcPx);
			if (px) {
				s->data = px;
			} else {
				fprintf(stderr, "Error decoding RLE8 sprite data\n");
				return 0;
			}
			break;
		case 3:
			px = Rle5Decode(s, srcPx, srcLen);
			free(srcPx);
			if (px) {
				s->data = px;
			} else {
				fprintf(stderr, "Error decoding RLE5 sprite data\n");
				return 0;
			}
			break;
		case 4:
			px = Lz5Decode(s, srcPx, srcLen);
			free(srcPx);
			if (px) {
				s->data = px;
			} else {
				fprintf(stderr, "Error decoding LZ5 sprite data\n");
				return 0;
			}
			break;

		case 10: // NOT OK
			px = PngDecode(s, data + p_offset, datasize);
			if (px) {
				s->data = px;
				// sff->palette_usage[s->palidx]++;
			} else {
				fprintf(stderr, "Error decoding PNG10 sprite data\n");
				return 0;
			}
			break;
		case 11:
			px = PngDecode(s, data + p_offset, datasize);
			if (px) {
				s->data = px;
				sff->palette_usage[-1]++;
			} else {
				fprintf(stderr, "Error decoding PNG11 sprite data\n");
				return 0;
			}
			break;
		case 12:
			px = PngDecode(s, data + p_offset, datasize);
			if (px) {
				s->data = px;
				sff->palette_usage[-1]++;
			} else {
				fprintf(stderr, "Error decoding PNG12 sprite data\n");
				return 0;
			}
			break;
		default:
			fprintf(stderr, "Format not supported\n");
			return 0;
		}
	}
	return p_offset;
}

int loadSff(Sff* sff, uint8_t* data) {
	bool character = true;
	// Copy filename to sff structure
	// strncpy(sff->filename, filename, sizeof(sff->filename) - 1);

	// Read the header
	uint32_t lofs, tofs, last_offset;
	if (!(last_offset = readSffHeader(sff, data, &lofs, &tofs))) {
		return -1;
	}

	printf("SFF version: %d.%d.%d.%d\n", sff->header.Ver0, sff->header.Ver1, sff->header.Ver2, sff->header.Ver3);
	printf("FirstSpriteHeaderOffset: %d\n", sff->header.FirstSpriteHeaderOffset);
	printf("NumberOfSprites: %d\n", sff->header.NumberOfSprites);
	printf("FirstPaletteHeaderOffset: %d\n", sff->header.FirstPaletteHeaderOffset);
	printf("NumberOfPalettes: %d\n", sff->header.NumberOfPalettes);

	if (sff->header.Ver0 != 1) {
		// Allocate memory for palettes
		std::map<std::array<int, 2>, int> uniquePals;
		sff->palList.numPalettes = 0;
		for (int i = 0; i < sff->header.NumberOfPalettes && i < MAX_PAL_NO; i++) {
			last_offset = sff->header.FirstPaletteHeaderOffset + i * 16;
			int16_t gn[3];
			memcpy(gn, data + last_offset, sizeof(gn));
			last_offset += sizeof(gn);
			// printf("Palette %d: Group %d, Number %d, ColNumber %d\n", i, gn[0], gn[1], gn[2]);

			uint16_t link;
			memcpy(&link, data + last_offset, sizeof(uint16_t));
			last_offset += sizeof(uint16_t);
			// printf("Palette link: %d\n", link);

			uint32_t ofs, siz;
			memcpy(&ofs, data + last_offset, sizeof(uint32_t));
			last_offset += sizeof(uint32_t);
			memcpy(&siz, data + last_offset, sizeof(uint32_t));
			last_offset += sizeof(uint32_t);
			// printf("Palette offset: %d, size: %d\n", ofs, siz);

			// Check if the palette is unique
			std::array<int, 2> key = { gn[0], gn[1] };
			if (uniquePals.find(key) == uniquePals.end()) {
				// Load palette data
				memcpy(sff->palList.palettes[i], data + lofs + ofs, sizeof(uint32_t) * 256);


				// Store the palette in the uniquePals map
				uniquePals[key] = i;
				sff->palList.paletteMap[i] = sff->palList.numPalettes;
				sff->palList.numPalettes++;
			} else {
				// If the palette is not unique, use the existing one
				printf("Palette %d(%d,%d) is not unique, using palette %d\nIncomplete code\n", i, gn[0], gn[1], uniquePals[key]);
			}
		}
	}

	// // Allocate memory for sprites
	sff->sprites = (Sprite**) malloc(sff->header.NumberOfSprites * sizeof(Sprite*));
	Sprite* prev = NULL;
	sff->numLinkedSprites = 0;
	long shofs = sff->header.FirstSpriteHeaderOffset;
	for (int i = 0; i < sff->header.NumberOfSprites; i++) {
		uint32_t xofs, size;
		uint16_t indexOfPrevious;
		uint8_t ps;
		sff->sprites[i] = newSprite();
		switch (sff->header.Ver0) {
		case 1:
			if (!(last_offset = readSpriteHeaderV1(sff->sprites[i], &data[shofs], &xofs, &size, &indexOfPrevious, &ps))) {
				return -1;
			}
			break;
		case 2:
			if (!(last_offset = readSpriteHeaderV2(sff->sprites[i], &data[shofs], &xofs, &size, lofs, tofs, &indexOfPrevious))) {
				printf("readSpriteHeaderV2(%d: %d,%d) palidx=%d size=%d indexOfPrevious=%d last_offset=%u\n", i, sff->sprites[i]->Group, sff->sprites[i]->Number, sff->sprites[i]->palidx, size, indexOfPrevious, last_offset);
				return -1;
			}
			// printf("readSpriteHeaderV2(%d: %d,%d) xofs=%d size=%d lofs=%d tofs=%d indexOfPrevious=%d\n", i, sff->sprites[i]->Group, sff->sprites[i]->Number, xofs, size, lofs, tofs, indexOfPrevious);
			// printf("readSpriteHeaderV2(%d: %d,%d) palidx=%d size=%d indexOfPrevious=%d\n", i, sff->sprites[i]->Group, sff->sprites[i]->Number, sff->sprites[i]->palidx, size, indexOfPrevious);
			break;
		}

		if (size == 0) {
			sff->numLinkedSprites++;
			if (indexOfPrevious < i) {
				Sprite* dst = sff->sprites[i];
				Sprite* src = sff->sprites[indexOfPrevious];
				spriteCopy(dst, src);
				// printf("Info: Sprite[%d] use prev Sprite[%d]\n", i, indexOfPrevious);
			} else {
				printf("Warning: Sprite %d has no size\n", i);
				sff->sprites[i]->palidx = 0;
			}
		} else {
			switch (sff->header.Ver0) {
			case 1:
				// if (sff->sprites[i]->Group == 0 && sff->sprites[i]->Number == 0) {
				// 	character = false;
				// }
				// printf("Sprite[%d] (%d,%d) ", i, sff->sprites[i]->Group, sff->sprites[i]->Number);
				// memcpy(&ps, &data[shofs + last_offset], sizeof(uint8_t));
				if (readSpriteDataV1(sff->sprites[i], data, sff, shofs + 32, size, xofs, prev, &sff->palettes, character, ps != 0 && prev != NULL) != 0) {
					return -1;
				}
				// printf("Sprite[%d] (%d,%d) : ps=%d pal=%d\n", i, sff->sprites[i]->Group, sff->sprites[i]->Number, ps, sff->sprites[i]->palidx);
				break;
			case 2:
				if (!readSpriteDataV2(sff->sprites[i], data, xofs, size, sff)) {
					return -1;
				}
				break;
			}

			// if use previous sprite Group 9000 and Number 0 only (fix for SFF v1)
			if (sff->sprites[i]->Group == 9000) {
				if (sff->sprites[i]->Number == 0) {
					prev = sff->sprites[i];
				}
			} else {
				prev = sff->sprites[i];
			}
		}

		if (sff->header.Ver0 == 1) {
			shofs = xofs;
		} else {
			shofs += 28;
		}

		// Set default opt_palidx for sprite with Group 0 and Number 0
		// if (sff->sprites[i]->Group == 0 && sff->sprites[i]->Number == 0 && opt_palidx == 0) {
		// 	opt_palidx = sff->sprites[i]->palidx;
		// }
	}
	// if SFF == v1 then update total palette
	if (sff->header.Ver0 == 1) {
		sff->header.NumberOfPalettes = sff->palettes.size();
	}
	return 0;
}
