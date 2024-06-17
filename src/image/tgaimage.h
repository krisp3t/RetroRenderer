#pragma once
#include <cstdint>
#include <vector>
#include <fstream>

#pragma pack(push, 1)
struct TGAHeader {
	enum DataType {
		NO_DATA = 0,
		COLORMAP = 1,
		TRUECOLOR = 2,
		MONOCHROME = 3,
		RLE_COLORMAP = 9, // Run-length encoded color-mapped image
		RLE_TRUECOLOR = 10,
		RLE_MONOCHROME = 11
	};

	std::uint8_t	idlength;
	std::uint8_t	colormaptype;	// whether colormap is included
	std::uint8_t	datatypecode;	// compression and color types
	std::uint16_t	colormaporigin;	// index of first color map entry
	std::uint16_t	colormaplength; // number of entries in color map
	std::uint8_t	colormapdepth;	// number of bits per entry
	std::uint16_t	x_origin;
	std::uint16_t	y_origin;
	std::uint16_t	width;
	std::uint16_t	height;
	std::uint8_t	bitsperpixel;
	std::uint8_t	imagedescriptor; // bits 3-0 alpha channel depth, bits 5-4 pixel ordering

	// we ignore developer, extension area and footer
};
#pragma pack(pop)

struct TGAColor {
	TGAColor() : bgra{ 0, 0, 0, 0 } {}
	TGAColor(std::uint8_t bpp): bgra{ 0, 0, 0, 0 }, bytespp(bpp) {}
	TGAColor(unsigned char R, unsigned char G, unsigned char B, unsigned char A) : bgra{ B, G, R, A } {}

	std::uint8_t	bgra[4] = { 0, 0, 0, 0 }; // blue, green, red, alpha
	std::uint8_t	bytespp = 4;
	std::uint8_t& operator[](const int i) { return bgra[i]; }
};

struct TGAImage {
	enum Format {
		GRAYSCALE = 1,
		RGB = 3,
		RGBA = 4
	};

	TGAImage() = default;
	TGAImage(const int w, const int h, const int bpp);

	bool read_tga_file(const std::string filename);
	bool write_tga_file(const std::string filename, const bool vflip = true, const bool rle = true) const;
	void flip_horizontally();
	void flip_vertically();
	TGAColor get(const int x, const int y) const;
	void set(const int x, const int y, const TGAColor& c);
	int width() const;
	int height() const;

private:
	std::vector<std::uint8_t> data;
	int w = 0;
	int h = 0;
	std::uint8_t bpp = 0;
	
	/**
	 * Loads RLE encoded data from file
	 * RLE encoding is form of data compression where runs of data are stored as single data value and count
	 * @param in input file stream
	 * @return true if successful, false otherwise
	 */
	bool load_rle_data(std::ifstream& in);
	bool unload_rle_data(std::ofstream& out) const;
};


