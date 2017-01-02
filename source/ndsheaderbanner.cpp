#include "ndsheaderbanner.h"
#include <stdio.h>

u32 colorConvert(u32 iconPixel, u16 indice) {
	u8 a = 255;
	if(!indice) {
		a = 0;
	}
	u8 b = iconPixel & 0x1F;
	u8 g = (iconPixel >> 5) & 0x1F;
	u8 r = (iconPixel >> 10) & 0x1F;
	
	return RGBA8(r, g, b, a) ;
}

sf2d_texture* grabIcon(FILE* ndsFile) {
	sNDSHeader NDSHeader;
	sNDSBanner myBanner;
	
	u32 textureData[1024];	
	
	fseek ( ndsFile , 0 , SEEK_SET );
	
	fread(&NDSHeader,1,sizeof(NDSHeader),ndsFile);
	
	fseek ( ndsFile , NDSHeader.bannerOffset , SEEK_SET );
	
	fread(&myBanner,1,sizeof(myBanner),ndsFile);	
	
	//u8 icon[512];			//!< 32*32 icon of the game with 4 bit per pixel paletted
	//u16 palette[16];      // color 0 is transparent, ABGR
	
	for(int i=0; i<512; i++) {
		u8 twopixel = myBanner.icon[i];
		u32 firstPixel = twopixel & 0x00FF;
		u32 secondPixel = (twopixel & 0xFF00) >> 4;
		textureData[2*i] = colorConvert(myBanner.palette[firstPixel], firstPixel);
		textureData[2*i+1] =  colorConvert(myBanner.palette[secondPixel], firstPixel);
	}
  
    fseek ( ndsFile , 0 , SEEK_SET );
  
    return sf2d_create_texture_mem_RGBA8(&textureData, 32, 32, TEXFMT_RGBA8, SF2D_PLACE_RAM);
}


