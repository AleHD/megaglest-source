// ==============================================================
//	This file is part of Glest Shared Library (www.glest.org)
//
//	Copyright (C) 2001-2008 Martiño Figueroa
//
//	You can redistribute this code and/or modify it under 
//	the terms of the GNU General Public License as published 
//	by the Free Software Foundation; either version 2 of the 
//	License, or (at your option) any later version
// ==============================================================

#ifndef _SHARED_GRAPHICS_TEXTURE_H_
#define _SHARED_GRAPHICS_TEXTURE_H_

#include "data_types.h"
#include "pixmap.h"
#include <string>
#include "leak_dumper.h"

using std::string;
using Shared::Platform::uint8;

struct SDL_Surface;

namespace Shared{ namespace Graphics{

class TextureParams;


// =====================================================
//	class Texture
// =====================================================

class Texture {
public:
	static const int defaultSize;
	static const int defaultComponents;
	static bool useTextureCompression;

	enum WrapMode{
		wmRepeat,
		wmClamp,
		wmClampToEdge
	};

	enum Filter{
		fBilinear,
		fTrilinear
	};

	enum Format{
		fAuto,
		fAlpha,
		fLuminance,
		fRgb,
		fRgba
	};

protected:
	string path;
	bool mipmap;
	WrapMode wrapMode;
	bool pixmapInit;
	Format format;

	bool inited;
	bool forceCompressionDisabled;
	int textureSystemId;

public:
	Texture();
	virtual ~Texture(){};
	
	bool getMipmap() const			{return mipmap;}
	WrapMode getWrapMode() const	{return wrapMode;}
	bool getPixmapInit() const		{return pixmapInit;}
	Format getFormat() const		{return format;}
	bool getInited() const 			{return inited;}

	int getTextureSystemId() const { return textureSystemId; }
	void setTextureSystemId(int id) { textureSystemId = id; }

	void setMipmap(bool mipmap)			{this->mipmap= mipmap;}
	void setWrapMode(WrapMode wrapMode)	{this->wrapMode= wrapMode;}
	void setPixmapInit(bool pixmapInit)	{this->pixmapInit= pixmapInit;}
	void setFormat(Format format)		{this->format= format;}

	virtual void init(Filter filter= fBilinear, int maxAnisotropy= 1)=0;
	virtual void end(bool deletePixelBuffer=true)=0;
	virtual string getPath() const = 0;
	virtual void deletePixels() = 0;
	virtual std::size_t getPixelByteCount() const = 0;

	virtual void reseInitState() { inited = false; }

	virtual void setForceCompressionDisabled(bool value) { forceCompressionDisabled = value;}
	virtual bool getForceCompressionDisabled() const {return forceCompressionDisabled;}

	virtual uint32 getCRC() = 0;

};

// =====================================================
//	class Texture1D
// =====================================================

class Texture1D: public Texture {
protected:
	Pixmap1D pixmap;

public:
	void load(const string &path);

	Pixmap1D *getPixmap()				{return &pixmap;}
	const Pixmap1D *getPixmap() const	{return &pixmap;}
	virtual string getPath() const;
	virtual void deletePixels();
	virtual std::size_t getPixelByteCount() const {return pixmap.getPixelByteCount();}

	virtual int getTextureWidth() const {return pixmap.getW();}
	virtual int getTextureHeight() const {return -1;}

	virtual uint32 getCRC() { return pixmap.getCRC()->getSum(); }
};

// =====================================================
//	class Texture2D
// =====================================================

class Texture2D: public Texture {
protected:
	Pixmap2D pixmap;

public:
	void load(const string &path);

	Pixmap2D *getPixmap()			{return &pixmap;}
	const Pixmap2D *getPixmapConst() const	{return &pixmap;}
	virtual string getPath() const;
	virtual void deletePixels();
	virtual std::size_t getPixelByteCount() const {return pixmap.getPixelByteCount();}

	virtual int getTextureWidth() const {return pixmap.getW();}
	virtual int getTextureHeight() const {return pixmap.getH();}

	virtual uint32 getCRC() { return pixmap.getCRC()->getSum(); }

	std::pair<SDL_Surface*,unsigned char*> CreateSDLSurface(bool newPixelData) const;
};

// =====================================================
//	class Texture3D
// =====================================================

class Texture3D: public Texture {
protected:
	Pixmap3D pixmap;

public:
	void loadSlice(const string &path, int slice);

	Pixmap3D *getPixmap()				{return &pixmap;}
	const Pixmap3D *getPixmap() const	{return &pixmap;}
	virtual string getPath() const;
	virtual void deletePixels();
	virtual std::size_t getPixelByteCount() const {return pixmap.getPixelByteCount();}

	virtual int getTextureWidth() const {return pixmap.getW();}
	virtual int getTextureHeight() const {return pixmap.getH();}

	virtual uint32 getCRC() { return pixmap.getCRC()->getSum(); }
};

// =====================================================
//	class TextureCube
// =====================================================

class TextureCube: public Texture{
protected:
	PixmapCube pixmap;

public:
	void loadFace(const string &path, int face);

	PixmapCube *getPixmap()				{return &pixmap;}
	const PixmapCube *getPixmap() const	{return &pixmap;}
	virtual string getPath() const;
	virtual void deletePixels();
	virtual std::size_t getPixelByteCount() const {return pixmap.getPixelByteCount();}

	virtual int getTextureWidth() const {return -1;}
	virtual int getTextureHeight() const {return -1;}

	virtual uint32 getCRC() { return pixmap.getCRC()->getSum(); }
};

}}//end namespace

#endif
