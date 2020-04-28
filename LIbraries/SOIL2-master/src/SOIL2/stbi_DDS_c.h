
///	DDS file support, does decoding, _not_ direct uploading
///	(use SOIL for that ;-)

#include "image_DXT.h"

#include <cstdint>

static int stbi__dds_test(stbi__context *s)
{
	//	check the magic number
	if (stbi__get8(s) != 'D') {
		stbi__rewind(s);
		return 0;
	}

	if (stbi__get8(s) != 'D') {
		stbi__rewind(s);
		return 0;
	}

	if (stbi__get8(s) != 'S') {
		stbi__rewind(s);
		return 0;
	}

	if (stbi__get8(s) != ' ') {
		stbi__rewind(s);
		return 0;
	}

	//	check header size
	if (stbi__get32le(s) != 124) {
		stbi__rewind(s);
		return 0;
	}

	// Also rewind because the loader needs to read the header
	stbi__rewind(s);

	return 1;
}
#ifndef STBI_NO_STDIO

int      stbi__dds_test_filename        		(char const *filename)
{
   int r;
   FILE *f = fopen(filename, "rb");
   if (!f) return 0;
   r = stbi__dds_test_file(f);
   fclose(f);
   return r;
}

int      stbi__dds_test_file        (FILE *f)
{
   stbi__context s;
   int r,n = ftell(f);
   stbi__start_file(&s,f);
   r = stbi__dds_test(&s);
   fseek(f,n,SEEK_SET);
   return r;
}
#endif

int      stbi__dds_test_memory      (stbi_uc const *buffer, int len)
{
   stbi__context s;
   stbi__start_mem(&s,buffer, len);
   return stbi__dds_test(&s);
}

int      stbi__dds_test_callbacks      (stbi_io_callbacks const *clbk, void *user)
{
   stbi__context s;
   stbi__start_callbacks(&s, (stbi_io_callbacks *) clbk, user);
   return stbi__dds_test(&s);
}

//	helper functions
int stbi_convert_bit_range( int c, int from_bits, int to_bits )
{
	int b = (1 << (from_bits - 1)) + c * ((1 << to_bits) - 1);
	return (b + (b >> from_bits)) >> from_bits;
}
void stbi_rgb_888_from_565( unsigned int c, int *r, int *g, int *b )
{
	*r = stbi_convert_bit_range( (c >> 11) & 31, 5, 8 );
	*g = stbi_convert_bit_range( (c >> 05) & 63, 6, 8 );
	*b = stbi_convert_bit_range( (c >> 00) & 31, 5, 8 );
}
void stbi_decode_DXT1_block(
			unsigned char uncompressed[16*4],
			unsigned char compressed[8] )
{
	int next_bit = 4*8;
	int i, r, g, b;
	int c0, c1;
	unsigned char decode_colors[4*4];
	//	find the 2 primary colors
	c0 = compressed[0] + (compressed[1] << 8);
	c1 = compressed[2] + (compressed[3] << 8);
	stbi_rgb_888_from_565( c0, &r, &g, &b );
	decode_colors[0] = r;
	decode_colors[1] = g;
	decode_colors[2] = b;
	decode_colors[3] = 255;
	stbi_rgb_888_from_565( c1, &r, &g, &b );
	decode_colors[4] = r;
	decode_colors[5] = g;
	decode_colors[6] = b;
	decode_colors[7] = 255;
	if( c0 > c1 )
	{
		//	no alpha, 2 interpolated colors
		decode_colors[8] = (2*decode_colors[0] + decode_colors[4]) / 3;
		decode_colors[9] = (2*decode_colors[1] + decode_colors[5]) / 3;
		decode_colors[10] = (2*decode_colors[2] + decode_colors[6]) / 3;
		decode_colors[11] = 255;
		decode_colors[12] = (decode_colors[0] + 2*decode_colors[4]) / 3;
		decode_colors[13] = (decode_colors[1] + 2*decode_colors[5]) / 3;
		decode_colors[14] = (decode_colors[2] + 2*decode_colors[6]) / 3;
		decode_colors[15] = 255;
	} else
	{
		//	1 interpolated color, alpha
		decode_colors[8] = (decode_colors[0] + decode_colors[4]) / 2;
		decode_colors[9] = (decode_colors[1] + decode_colors[5]) / 2;
		decode_colors[10] = (decode_colors[2] + decode_colors[6]) / 2;
		decode_colors[11] = 255;
		decode_colors[12] = 0;
		decode_colors[13] = 0;
		decode_colors[14] = 0;
		decode_colors[15] = 0;
	}
	//	decode the block
	for( i = 0; i < 16*4; i += 4 )
	{
		int idx = ((compressed[next_bit>>3] >> (next_bit & 7)) & 3) * 4;
		next_bit += 2;
		uncompressed[i+0] = decode_colors[idx+0];
		uncompressed[i+1] = decode_colors[idx+1];
		uncompressed[i+2] = decode_colors[idx+2];
		uncompressed[i+3] = decode_colors[idx+3];
	}
	//	done
}
void stbi_decode_DXT23_alpha_block(
			unsigned char uncompressed[16*4],
			unsigned char compressed[8] )
{
	int i, next_bit = 0;
	//	each alpha value gets 4 bits
	for( i = 3; i < 16*4; i += 4 )
	{
		uncompressed[i] = stbi_convert_bit_range(
				(compressed[next_bit>>3] >> (next_bit&7)) & 15,
				4, 8 );
		next_bit += 4;
	}
}
void stbi_decode_DXT45_alpha_block(
			unsigned char uncompressed[16*4],
			unsigned char compressed[8] )
{
	int i, next_bit = 8*2;
	unsigned char decode_alpha[8];
	//	each alpha value gets 3 bits, and the 1st 2 bytes are the range
	decode_alpha[0] = compressed[0];
	decode_alpha[1] = compressed[1];
	if( decode_alpha[0] > decode_alpha[1] )
	{
		//	6 step intermediate
		decode_alpha[2] = (6*decode_alpha[0] + 1*decode_alpha[1]) / 7;
		decode_alpha[3] = (5*decode_alpha[0] + 2*decode_alpha[1]) / 7;
		decode_alpha[4] = (4*decode_alpha[0] + 3*decode_alpha[1]) / 7;
		decode_alpha[5] = (3*decode_alpha[0] + 4*decode_alpha[1]) / 7;
		decode_alpha[6] = (2*decode_alpha[0] + 5*decode_alpha[1]) / 7;
		decode_alpha[7] = (1*decode_alpha[0] + 6*decode_alpha[1]) / 7;
	} else
	{
		//	4 step intermediate, pluss full and none
		decode_alpha[2] = (4*decode_alpha[0] + 1*decode_alpha[1]) / 5;
		decode_alpha[3] = (3*decode_alpha[0] + 2*decode_alpha[1]) / 5;
		decode_alpha[4] = (2*decode_alpha[0] + 3*decode_alpha[1]) / 5;
		decode_alpha[5] = (1*decode_alpha[0] + 4*decode_alpha[1]) / 5;
		decode_alpha[6] = 0;
		decode_alpha[7] = 255;
	}
	for( i = 3; i < 16*4; i += 4 )
	{
		int idx = 0, bit;
		bit = (compressed[next_bit>>3] >> (next_bit&7)) & 1;
		idx += bit << 0;
		++next_bit;
		bit = (compressed[next_bit>>3] >> (next_bit&7)) & 1;
		idx += bit << 1;
		++next_bit;
		bit = (compressed[next_bit>>3] >> (next_bit&7)) & 1;
		idx += bit << 2;
		++next_bit;
		uncompressed[i] = decode_alpha[idx & 7];
	}
	//	done
}
void stbi_decode_DXT_color_block(
			unsigned char uncompressed[16*4],
			unsigned char compressed[8] )
{
	int next_bit = 4*8;
	int i, r, g, b;
	int c0, c1;
	unsigned char decode_colors[4*3];
	//	find the 2 primary colors
	c0 = compressed[0] + (compressed[1] << 8);
	c1 = compressed[2] + (compressed[3] << 8);
	stbi_rgb_888_from_565( c0, &r, &g, &b );
	decode_colors[0] = r;
	decode_colors[1] = g;
	decode_colors[2] = b;
	stbi_rgb_888_from_565( c1, &r, &g, &b );
	decode_colors[3] = r;
	decode_colors[4] = g;
	decode_colors[5] = b;
	//	Like DXT1, but no choicees:
	//	no alpha, 2 interpolated colors
	decode_colors[6] = (2*decode_colors[0] + decode_colors[3]) / 3;
	decode_colors[7] = (2*decode_colors[1] + decode_colors[4]) / 3;
	decode_colors[8] = (2*decode_colors[2] + decode_colors[5]) / 3;
	decode_colors[9] = (decode_colors[0] + 2*decode_colors[3]) / 3;
	decode_colors[10] = (decode_colors[1] + 2*decode_colors[4]) / 3;
	decode_colors[11] = (decode_colors[2] + 2*decode_colors[5]) / 3;
	//	decode the block
	for( i = 0; i < 16*4; i += 4 )
	{
		int idx = ((compressed[next_bit>>3] >> (next_bit & 7)) & 3) * 3;
		next_bit += 2;
		uncompressed[i+0] = decode_colors[idx+0];
		uncompressed[i+1] = decode_colors[idx+1];
		uncompressed[i+2] = decode_colors[idx+2];
	}
	//	done
}

static int stbi__dds_info( stbi__context *s, int *x, int *y, int *comp, int *iscompressed ) {
	int flags,is_compressed,has_alpha;
	DDS_header header={0};

	if( sizeof( DDS_header ) != 128 )
	{
		return 0;
	}

	stbi__getn( s, (stbi_uc*)(&header), 128 );

	if( header.dwMagic != (('D' << 0) | ('D' << 8) | ('S' << 16) | (' ' << 24)) ) {
	   stbi__rewind( s );
	   return 0;
	}
	if( header.dwSize != 124 ) {
	   stbi__rewind( s );
	   return 0;
	}
	flags = DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH | DDSD_PIXELFORMAT;
	if( (header.dwFlags & flags) != flags ) {
	   stbi__rewind( s );
	   return 0;
	}
	if( header.sPixelFormat.dwSize != 32 ) {
	   stbi__rewind( s );
	   return 0;
	}
	flags = DDPF_FOURCC | DDPF_RGB;
	if( (header.sPixelFormat.dwFlags & flags) == 0 ) {
	   stbi__rewind( s );
	   return 0;
	}
	if( (header.sCaps.dwCaps1 & DDSCAPS_TEXTURE) == 0 ) {
	   stbi__rewind( s );
	   return 0;
	}

	is_compressed = (header.sPixelFormat.dwFlags & DDPF_FOURCC) / DDPF_FOURCC;
	has_alpha = (header.sPixelFormat.dwFlags & DDPF_ALPHAPIXELS) / DDPF_ALPHAPIXELS;

	*x = header.dwWidth;
	*y = header.dwHeight;

	if ( !is_compressed ) {
		*comp = 3;

		if ( has_alpha )
			*comp = 4;
	}
	else
		*comp = 4;

	if ( iscompressed )
		*iscompressed = is_compressed;

	return 1;
}

int stbi__dds_info_from_memory (stbi_uc const *buffer, int len, int *x, int *y, int *comp, int *iscompressed)
{
	stbi__context s;
	stbi__start_mem(&s,buffer, len);
	return stbi__dds_info( &s, x, y, comp, iscompressed );
}

int stbi__dds_info_from_callbacks (stbi_io_callbacks const *clbk, void *user, int *x, int *y, int *comp, int *iscompressed)
{
	stbi__context s;
	stbi__start_callbacks(&s, (stbi_io_callbacks *) clbk, user);
	return stbi__dds_info( &s, x, y, comp, iscompressed );
}

#ifndef STBI_NO_STDIO
int stbi__dds_info_from_path(char const *filename,     int *x, int *y, int *comp, int *iscompressed)
{
   int res;
   FILE *f = fopen(filename, "rb");
   if (!f) return 0;
   res = stbi__dds_info_from_file( f, x, y, comp, iscompressed );
   fclose(f);
   return res;
}

int stbi__dds_info_from_file(FILE *f,                  int *x, int *y, int *comp, int *iscompressed)
{
   stbi__context s;
   int res;
   long n = ftell(f);
   stbi__start_file(&s, f);
   res = stbi__dds_info(&s, x, y, comp, iscompressed);
   fseek(f, n, SEEK_SET);
   return res;
}
#endif

static void * stbi__dds_load(stbi__context *s, int *x, int *y, int *comp, int* faces, int* mipmaps, int* bit_depth, int req_comp)
{
	//	all variables go up front
	stbi_uc *dds_data = NULL;
	stbi_uc block[16*4];
	stbi_uc compressed[8];
	int flags, DXT_family;
	int has_alpha, has_mipmap;
	int is_compressed, cubemap_faces;
	int block_pitch, num_blocks;
	DDS_header header={0};
	int i, sz, cf;
	//	load the header
	if( sizeof( DDS_header ) != 128 )
	{
		return NULL;
	}
	stbi__getn( s, (stbi_uc*)(&header), 128 );
	//	and do some checking
	if( header.dwMagic != (('D' << 0) | ('D' << 8) | ('S' << 16) | (' ' << 24)) ) return NULL;
	if( header.dwSize != 124 ) return NULL;
	flags = DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH | DDSD_PIXELFORMAT;
	if( (header.dwFlags & flags) != flags ) return NULL;
	/*	According to the MSDN spec, the dwFlags should contain
		DDSD_LINEARSIZE if it's compressed, or DDSD_PITCH if
		uncompressed.  Some DDS writers do not conform to the
		spec, so I need to make my reader more tolerant	*/
	if( header.sPixelFormat.dwSize != 32 ) return NULL;
	flags = DDPF_FOURCC | DDPF_RGB;
	if( (header.sPixelFormat.dwFlags & flags) == 0 ) return NULL;
	if( (header.sCaps.dwCaps1 & DDSCAPS_TEXTURE) == 0 ) return NULL;
	//	get the image data
	s->img_x = header.dwWidth;
	s->img_y = header.dwHeight;
	s->img_n = 4;
	is_compressed = (header.sPixelFormat.dwFlags & DDPF_FOURCC) / DDPF_FOURCC;
	
	has_alpha = (header.sPixelFormat.dwFlags & DDPF_ALPHAPIXELS) / DDPF_ALPHAPIXELS;
	has_mipmap = (header.sCaps.dwCaps1 & DDSCAPS_MIPMAP) && (header.dwMipMapCount > 1);
	cubemap_faces = (header.sCaps.dwCaps2 & DDSCAPS2_CUBEMAP) / DDSCAPS2_CUBEMAP;
	/*	I need cubemaps to have square faces	*/
	cubemap_faces &= (s->img_x == s->img_y);
	cubemap_faces *= 5;
	cubemap_faces += 1;
	/*	let the user know what's going on	*/
	*x = s->img_x;
	*y = s->img_y;
	*faces = cubemap_faces;
	*mipmaps = header.dwMipMapCount;
	*comp = s->img_n;

	if (is_compressed)
	{
		//	note: header.sPixelFormat.dwFourCC is something like (('D'<<0)|('X'<<8)|('T'<<16)|('1'<<24))
		DXT_family = 1 + (header.sPixelFormat.dwFourCC >> 24) - '1';
		if (DXT_family <= 0)
		{
			is_compressed = false; 
			s->img_n = 4;
			*comp = 4;
		}
	}
	int bitPerPixel = header.dwPitchOrLinearSize / header.dwWidth;
	if (is_compressed)
	{
		// bytes after decompressing
		bitPerPixel = 32;
	}
	*bit_depth = bitPerPixel;

	int pixCount = 0;

	/*	is this uncompressed?	*/
	if( is_compressed )
	{
		/*	compressed	*/
		//	note: header.sPixelFormat.dwFourCC is something like (('D'<<0)|('X'<<8)|('T'<<16)|('1'<<24))
		DXT_family = 1 + (header.sPixelFormat.dwFourCC >> 24) - '1';
		if( (DXT_family < 1) || (DXT_family > 5) ) return NULL;
		/*	check the expected size...oops, nevermind...
			those non-compliant writers leave
			dwPitchOrLinearSize == 0	*/
		//	passed all the tests, get the RAM for decoding
		int mipDataSize = 0;
		for (int mip = 1; mip < (int)header.dwMipMapCount; mip++)
		{
			int mx = s->img_x >> mip;
			int my = s->img_y >> mip;
			mipDataSize += mx * my * 4 * cubemap_faces;
		}
		sz = (s->img_x)*(s->img_y)*4*cubemap_faces + mipDataSize;
		pixCount = sz / 4;
		dds_data = (unsigned char*)malloc( sz );
		/*	do this once for each face	*/
		for( cf = 0; cf < cubemap_faces; ++ cf )
		{
			int mipOffset = 0;
			for (int mip = 0; mip < (int)header.dwMipMapCount; mip++)
			{
				int mipW = s->img_x >> mip;
				int mipH = s->img_y >> mip;

				block_pitch = (mipW + 3) >> 2;
				num_blocks = block_pitch * ((mipH + 3) >> 2);

				//	now read and decode all the blocks
				for (i = 0; i < num_blocks; ++i)
				{
					//	where are we?
					int bx, by, bw = 4, bh = 4;
					int ref_x = 4 * (i % block_pitch);
					int ref_y = 4 * (i / block_pitch);
					//	get the next block's worth of compressed data, and decompress it
					if (DXT_family == 1)
					{
						//	DXT1
						stbi__getn(s, compressed, 8);
						stbi_decode_DXT1_block(block, compressed);
					}
					else if (DXT_family < 4)
					{
						//	DXT2/3
						stbi__getn(s, compressed, 8);
						stbi_decode_DXT23_alpha_block(block, compressed);
						stbi__getn(s, compressed, 8);
						stbi_decode_DXT_color_block(block, compressed);
					}
					else
					{
						//	DXT4/5
						stbi__getn(s, compressed, 8);
						stbi_decode_DXT45_alpha_block(block, compressed);
						stbi__getn(s, compressed, 8);
						stbi_decode_DXT_color_block(block, compressed);
					}
					//	is this a partial block?
					if (ref_x + 4 > (int)mipW)
					{
						bw = mipW - ref_x;
					}
					if (ref_y + 4 > (int)mipH)
					{
						bh = mipH - ref_y;
					}
					//	now drop our decompressed data into the buffer
					for (by = 0; by < bh; ++by)
					{
						int idx = 4 * ((ref_y + by + cf * mipW) * mipW + ref_x) + mipOffset;
						for (bx = 0; bx < bw * 4; ++bx)
						{

							dds_data[idx + bx] = block[by * 16 + bx];
						}
					}
				}

				int block_size = 16;
				if (DXT_family == 1)
				{
					block_size = 8;
				}

				mipOffset += num_blocks * block_size;
			}
		}/* per cubemap face */
	} else
	{
		/*	uncompressed	*/
		DXT_family = 0;
		if (bitPerPixel == 32)
		{
			s->img_n = 3;
		}
		if( has_alpha )
		{
			s->img_n = 4;
		}
		*comp = s->img_n;

		int perFacePipDataSize = 0;
		for (int mip = 1; mip < (int)header.dwMipMapCount; mip++)
		{
			int mx = s->img_x >> mip;
			int my = s->img_y >> mip;
			perFacePipDataSize += mx * my * s->img_n;
		}
		sz = s->img_x * s->img_y * s->img_n * cubemap_faces + perFacePipDataSize * cubemap_faces;
		pixCount = sz / s->img_n;

		int bytePerChannel = bitPerPixel / s->img_n / 8;

		dds_data = (unsigned char*)malloc( sz * bytePerChannel);
		/*	do this once for each face	*/
		for( cf = 0; cf < cubemap_faces; ++ cf )
		{
			/*	read the main image for this face	*/
			stbi__getn( s, 
				&dds_data[cf * (s->img_x * s->img_y * s->img_n + perFacePipDataSize) * bytePerChannel],
				(s->img_x * s->img_y * s->img_n + perFacePipDataSize) * bytePerChannel
			);
		}

		/*	data was BGR, I need it RGB	*/
		/*	64 and 128 bpp dds don't have BGR format*/
		if (bytePerChannel == 1)
		{
			uint8_t* pData = reinterpret_cast<uint8_t*>(dds_data); 
			for (i = 0; i < sz; i += s->img_n)
			{
				uint8_t temp = pData[i];
				pData[i] = pData[i + 2];
				pData[i + 2] = temp;
			}
		}
	}
	/*	finished decompressing into RGBA,
		adjust the y size if we have a cubemap
		note: sz is already up to date	*/
	//s->img_y *= cubemap_faces;
	*y = s->img_y;
	//	did the user want something else, or
	//	see if all the alpha values are 255 (i.e. no transparency)
	has_alpha = 0;
	if( s->img_n == 4)
	{
		if (bitPerPixel != 32)
		{
			has_alpha = 1;
		}
		else
		{
			for (i = 3; (i < sz) && (has_alpha == 0); i += 4)
			{
				has_alpha |= (dds_data[i] < 255);
			}
		}
	}

	if( (req_comp <= 4) && (req_comp >= 1) && bitPerPixel == 32)
	{
		//	user has some requirements, meet them
		if( req_comp != s->img_n )
		{
			dds_data = stbi__convert_format( dds_data, s->img_n, req_comp, 1, pixCount);
			*comp = req_comp;
		}
	} else
	{
		//	user had no requirements, only drop to RGB is no alpha
		if( (has_alpha == 0) && (s->img_n == 4) )
		{
			dds_data = stbi__convert_format( dds_data, 4, 3, 1, pixCount);
			*comp = 3;
		}
	}
	//	OK, done
	return dds_data;
}

#ifndef STBI_NO_STDIO
void *stbi__dds_load_from_file   (FILE *f, int *x, int *y, int *comp, int *faces, int *mips, int* bit_depth, int req_comp)
{
	stbi__context s;
	stbi__start_file(&s,f);
	return stbi__dds_load(&s,x,y, comp, faces, mips, bit_depth, req_comp);
}

void *stbi__dds_load_from_path             (const char *filename,           int *x, int *y, int *comp, int *faces, int *mips, int* bit_depth, int req_comp)
{
   void *data;
   FILE *f = fopen(filename, "rb");
   if (!f) return NULL;
   data = stbi__dds_load_from_file(f,x,y,comp, faces, mips, bit_depth, req_comp);
   fclose(f);
   return data;
}
#endif

void *stbi__dds_load_from_memory (stbi_uc const *buffer, int len, int *x, int *y, int *comp, int *faces, int *mips, int* bit_depth, int req_comp)
{
	stbi__context s;
   stbi__start_mem(&s,buffer, len);
   return stbi__dds_load(&s,x,y,comp, faces, mips, bit_depth, req_comp);
}

void *stbi__dds_load_from_callbacks (stbi_io_callbacks const *clbk, void *user, int *x, int *y, int *comp, int *faces, int *mips, int* bit_depth, int req_comp)
{
	stbi__context s;
   stbi__start_callbacks(&s, (stbi_io_callbacks *) clbk, user);
   return stbi__dds_load(&s,x,y, comp, faces, mips, bit_depth, req_comp);
}
