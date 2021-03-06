#pragma once

#ifndef _USE_AERO
#include "../FreeImage/FreeImagePlus.h"

class internalImage : public fipImage
{
public:
	BOOL rescale(unsigned new_width, unsigned new_height)
	{
		return fipImage::rescale(new_width, new_height, FILTER_BILINEAR);
	}

	BOOL loadU(const wchar_t* lpszPathName)
	{
		BOOL rc = fipImage::loadU(lpszPathName, 0);
		if( rc ) fipImage::convertTo32Bits();
		return rc;
	}

	BOOL loadFromMemory(BYTE *data, DWORD size_in_bytes)
	{
		fipMemoryIO memIO(data, size_in_bytes);
		BOOL rc = fipImage::loadFromMemory(memIO);
		if( rc ) fipImage::convertTo32Bits();
		return rc;
	}

	void CreateDIBitmap(const CDC& dc, CBitmap& bmpTemplate)
	{
		bmpTemplate.CreateDIBitmap(
			dc,
			fipImage::getInfoHeader(),
			CBM_INIT,
			fipImage::accessPixels(),
			fipImage::getInfo(),
			DIB_RGB_COLORS);
	}
};

#else

class internalImage
{
public:
	unsigned getWidth() const
	{
		return bitmap.get() ? bitmap->GetWidth() : 0;
	}
  unsigned getHeight() const
	{
		return bitmap.get() ? bitmap->GetHeight() : 0;
	}
  BOOL loadU(const wchar_t* lpszPathName)
	{
		CComPtr<IStream> stream;
		if( ::SHCreateStreamOnFileEx(lpszPathName,
		                             STGM_READ | STGM_SHARE_DENY_WRITE,
		                             0,
		                             FALSE,
		                             nullptr,
		                             &stream) != S_OK )
			return FALSE;

		bitmap.reset(Gdiplus::Bitmap::FromStream(stream));

		return TRUE;
	}
	BOOL rescale(unsigned new_width, unsigned new_height)
	{
		std::unique_ptr<Gdiplus::Bitmap> newbmp(new Gdiplus::Bitmap(new_width, new_height, bitmap->GetPixelFormat()));
		Gdiplus::Graphics graphics(newbmp.get());
		if( graphics.DrawImage(bitmap.get(), 0, 0, new_width, new_height) == Gdiplus::Status::Ok )
		{
			bitmap.reset(newbmp.release());
			return TRUE;
		}
		return FALSE;
	}
	void CreateDIBitmap(const CDC& dc, CBitmap& bmpTemplate)
	{
		CBitmap img;
		if( bitmap->GetHBITMAP(Gdiplus::Color::MakeARGB(0, 0, 0, 0), &img.m_hBitmap) == Gdiplus::Status::Ok )
		{
			DIBSECTION ds;
			GetObject(img.m_hBitmap, sizeof(DIBSECTION), &ds);

			//make sure compression is BI_RGB
			ds.dsBmih.biCompression = BI_RGB;

			//Convert DIB to DDB
			bmpTemplate.CreateDIBitmap(
				dc,
				&ds.dsBmih,
				CBM_INIT,
				ds.dsBm.bmBits,
				reinterpret_cast<BITMAPINFO*>(&ds.dsBmih),
				DIB_RGB_COLORS);
		}
	}
	BOOL loadFromMemory(BYTE *data, DWORD size_in_bytes)
	{
		CComPtr<IStream> stream;
		stream.Attach(::SHCreateMemStream(data, size_in_bytes));

		bitmap.reset(Gdiplus::Bitmap::FromStream(stream));

		return TRUE;
	}

private:
	std::shared_ptr<Gdiplus::Bitmap> bitmap;
};
#endif

//////////////////////////////////////////////////////////////////////////////

enum ImagePosition
{
  /* new names like Win7 walppaper settings */
  imagePositionCenter      = 0,
  imagePositionStretch     = 1,
  imagePositionTile        = 2,
  imagePositionFit         = 3,
  imagePositionFill        = 4,

  /* old names */
  /*
  imgPosCenter             = 0,
  imgPosFit                = 1,
  imgPosTile               = 2,
  imgPosFitWithAspectRatio = 3
  */
};

//////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////

struct ImageData
{
	ImageData()
	: strFilename(L"")
	, bRelative(false)
	, bExtend(false)
	, imagePosition(imagePositionCenter)
	, crBackground(RGB(0, 0, 0))
	, crTint(RGB(0, 0, 0))
	, byTintOpacity(0)
	, strCopyright()
	, strCopyrightLink()
	{
	}

	ImageData(const wstring& filename, bool relative, bool extend, ImagePosition position, COLORREF background, COLORREF tint, BYTE tintOpacity)
	: strFilename(filename)
	, bRelative(relative)
	, bExtend(extend)
	, imagePosition(position)
	, crBackground(background)
	, crTint(tint)
	, byTintOpacity(tintOpacity)
	, strCopyright()
	, strCopyrightLink()
	{
	}

	ImageData(const ImageData& other)
	: strFilename(other.strFilename)
	, bRelative(other.bRelative)
	, bExtend(other.bExtend)
	, imagePosition(other.imagePosition)
	, crBackground(other.crBackground)
	, crTint(other.crTint)
	, byTintOpacity(other.byTintOpacity)
	, strCopyright(other.strCopyright)
	, strCopyrightLink(other.strCopyrightLink)
	{
	}

	ImageData& operator=(const ImageData& other)
	{
		strFilename		= other.strFilename;
		bRelative		= other.bRelative;
		bExtend			= other.bExtend;
		imagePosition	= other.imagePosition;
		crBackground	= other.crBackground;
		crTint			= other.crTint;
		byTintOpacity	= other.byTintOpacity;
		strCopyright     = other.strCopyright;
		strCopyrightLink = other.strCopyrightLink;

		return *this;
	}

	bool operator==(const ImageData& other) const
	{
		if (strFilename != other.strFilename)		return false;
		if (bRelative != other.bRelative)			return false;
		if (bExtend != other.bExtend)				return false;
		if (imagePosition != other.imagePosition)	return false;
		if (crBackground != other.crBackground)		return false;
		if (crTint != other.crTint)					return false;
		if (byTintOpacity != other.byTintOpacity)	return false;

		return true;
	}

	wstring				strFilename;

	bool				bRelative;
	bool				bExtend;
	ImagePosition		imagePosition;

	COLORREF			crBackground;

	COLORREF			crTint;
	BYTE				byTintOpacity;

	std::wstring strCopyright;
	std::wstring strCopyrightLink;

};

//////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////

struct BackgroundImage
{
	BackgroundImage(const ImageData& data)
	: imageData(data)
	, dwOriginalImageWidth(0)
	, dwOriginalImageHeight(0)
	, dwImageWidth(0)
	, dwImageHeight(0)
	, bWallpaper(false)
	, originalImage()
	, image()
	, dcImage()
	, updateCritSec()
	{
	}

	ImageData			imageData;

	DWORD				dwOriginalImageWidth;
	DWORD				dwOriginalImageHeight;
	DWORD				dwImageWidth;
	DWORD				dwImageHeight;

	bool				bWallpaper;

	std::shared_ptr<internalImage> originalImage;

	CBitmap				image;
	CDC					dcImage;

	CriticalSection		updateCritSec;
};

struct MonitorEnumData
{
	MonitorEnumData(const CDC& dcTempl, std::shared_ptr<BackgroundImage>& img)
	: bkImage(img)
	, dcTemplate(dcTempl)
	{
	}

	std::shared_ptr<BackgroundImage>&	bkImage;
	const CDC&                        dcTemplate;
};

//////////////////////////////////////////////////////////////////////////////

typedef vector<std::shared_ptr<BackgroundImage> >	Images;

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////

class ImageHandler
{
	public:

		ImageHandler();
		~ImageHandler();

	public:

		std::shared_ptr<BackgroundImage> GetImage(const ImageData& imageData);
		std::shared_ptr<BackgroundImage> GetDesktopImage(ImageData& imageData);
		std::shared_ptr<BackgroundImage> GetBingImage(ImageData& imageData);
		void ReloadDesktopImages();

		void UpdateImageBitmap(const CDC& dc, const CRect& clientRect, std::shared_ptr<BackgroundImage>& bkImage);
		static inline bool IsWin8(void) { return m_win8; }

	private:

		static bool GetDesktopImageData(ImageData& imageData);
		static bool GetBingImageData(ImageData& imageData);
		static bool LoadImage(std::shared_ptr<BackgroundImage>& bkImage);
		static bool LoadImageFromContent(std::shared_ptr<BackgroundImage>& bkImage, const std::vector<char>& content);

		static void CalcRescale(DWORD& dwNewWidth, DWORD& dwNewHeight, std::shared_ptr<BackgroundImage>& bkImage);
		static void PaintRelativeImage(const CDC& dc, CBitmap&	bmpTemplate, std::shared_ptr<BackgroundImage>& bkImage, DWORD& dwDisplayWidth, DWORD& dwDisplayHeight);
		static void CreateRelativeImage(const CDC& dc, std::shared_ptr<BackgroundImage>& bkImage);
		static void CreateImage(const CDC& dc, const CRect& clientRect, std::shared_ptr<BackgroundImage>& bkImage);

		static void PaintTemplateImage(const CDC& dcTemplate, int nOffsetX, int nOffsetY, DWORD dwSrcWidth, DWORD dwSrcHeight, DWORD dwDstWidth, DWORD dwDstHeight, std::shared_ptr<BackgroundImage>& bkImage);
		static void TileTemplateImage(const CDC& dcTemplate, int nOffsetX, int nOffsetY, std::shared_ptr<BackgroundImage>& bkImage);

		static void TintImage(const CDC& dc, std::shared_ptr<BackgroundImage>& bkImage);

		// called by the ::EnumDisplayMonitors to create background for each display
		static BOOL CALLBACK MonitorEnumProc(HMONITOR /*hMonitor*/, HDC /*hdcMonitor*/, LPRECT lprcMonitor, LPARAM lpData);
		static BOOL CALLBACK MonitorEnumProcWin8(HMONITOR /*hMonitor*/, HDC /*hdcMonitor*/, LPRECT lprcMonitor, LPARAM lpData);
		static bool CheckWin8(void);

		static void LoadDesktopWallpaperWin8(MonitorEnumData* pEnumData);

	private:

		Images	m_images;
		static bool	m_win8;
};

//////////////////////////////////////////////////////////////////////////////
