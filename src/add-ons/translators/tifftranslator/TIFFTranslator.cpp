/*****************************************************************************/
// TIFFTranslator
// Written by Michael Wilber, OBOS Translation Kit Team
//
// TIFFTranslator.cpp
//
// This BTranslator based object is for opening and writing 
// TIFF images.
//
//
// Copyright (c) 2003 OpenBeOS Project
//
// Permission is hereby granted, free of charge, to any person obtaining a
// copy of this software and associated documentation files (the "Software"),
// to deal in the Software without restriction, including without limitation
// the rights to use, copy, modify, merge, publish, distribute, sublicense, 
// and/or sell copies of the Software, and to permit persons to whom the 
// Software is furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included 
// in all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
// OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL 
// THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING 
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
// DEALINGS IN THE SOFTWARE.
/*****************************************************************************/

#include <string.h>
#include <stdio.h>
#include "TIFFTranslator.h"
#include "TIFFView.h"
#include "TiffIfd.h"
#include "StreamBuffer.h"

// The input formats that this translator supports.
translation_format gInputFormats[] = {
	{
		B_TRANSLATOR_BITMAP,
		B_TRANSLATOR_BITMAP,
		BBT_IN_QUALITY,
		BBT_IN_CAPABILITY,
		"image/x-be-bitmap",
		"Be Bitmap Format (TIFFTranslator)"
	},
	{
		B_TIFF_FORMAT,
		B_TRANSLATOR_BITMAP,
		TIFF_IN_QUALITY,
		TIFF_IN_CAPABILITY,
		"image/tiff",
		"TIFF Image"
	}
};

// The output formats that this translator supports.
translation_format gOutputFormats[] = {
	{
		B_TRANSLATOR_BITMAP,
		B_TRANSLATOR_BITMAP,
		BBT_OUT_QUALITY,
		BBT_OUT_CAPABILITY,
		"image/x-be-bitmap",
		"Be Bitmap Format (TIFFTranslator)"
	},
	{
		B_TIFF_FORMAT,
		B_TRANSLATOR_BITMAP,
		TIFF_OUT_QUALITY,
		TIFF_OUT_CAPABILITY,
		"image/tiff",
		"TIFF Image"
	}
};

// ---------------------------------------------------------------
// make_nth_translator
//
// Creates a TIFFTranslator object to be used by BTranslatorRoster
//
// Preconditions:
//
// Parameters: n,		The translator to return. Since
//						TIFFTranslator only publishes one
//						translator, it only returns a
//						TIFFTranslator if n == 0
//
//             you, 	The image_id of the add-on that
//						contains code (not used).
//
//             flags,	Has no meaning yet, should be 0.
//
// Postconditions:
//
// Returns: NULL if n is not zero,
//          a new TIFFTranslator if n is zero
// ---------------------------------------------------------------
BTranslator *
make_nth_translator(int32 n, image_id you, uint32 flags, ...)
{
	if (!n)
		return new TIFFTranslator();
	else
		return NULL;
}

// ---------------------------------------------------------------
// Constructor
//
// Sets up the version info and the name of the translator so that
// these values can be returned when they are requested.
//
// Preconditions:
//
// Parameters:
//
// Postconditions:
//
// Returns:
// ---------------------------------------------------------------
TIFFTranslator::TIFFTranslator()
	:	BTranslator()
{
	strcpy(fName, "TIFF Images");
	sprintf(fInfo, "TIFF image translator v%d.%d.%d %s",
		TIFF_TRANSLATOR_VERSION / 100, (TIFF_TRANSLATOR_VERSION / 10) % 10,
		TIFF_TRANSLATOR_VERSION % 10, __DATE__);
}

// ---------------------------------------------------------------
// Destructor
//
// Does nothing
//
// Preconditions:
//
// Parameters:
//
// Postconditions:
//
// Returns:
// ---------------------------------------------------------------
TIFFTranslator::~TIFFTranslator()
{
}

// ---------------------------------------------------------------
// TranslatorName
//
// Returns the short name of the translator.
//
// Preconditions:
//
// Parameters:
//
// Postconditions:
//
// Returns: a const char * to the short name of the translator
// ---------------------------------------------------------------	
const char *
TIFFTranslator::TranslatorName() const
{
	return fName;
}

// ---------------------------------------------------------------
// TranslatorInfo
//
// Returns a more verbose name for the translator than the one
// TranslatorName() returns. This usually includes version info.
//
// Preconditions:
//
// Parameters:
//
// Postconditions:
//
// Returns: a const char * to the verbose name of the translator
// ---------------------------------------------------------------
const char *
TIFFTranslator::TranslatorInfo() const
{
	return fInfo;
}

// ---------------------------------------------------------------
// TranslatorVersion
//
// Returns the integer representation of the current version of
// this translator.
//
// Preconditions:
//
// Parameters:
//
// Postconditions:
//
// Returns:
// ---------------------------------------------------------------
int32 
TIFFTranslator::TranslatorVersion() const
{
	return TIFF_TRANSLATOR_VERSION;
}

// ---------------------------------------------------------------
// InputFormats
//
// Returns a list of input formats supported by this translator.
//
// Preconditions:
//
// Parameters:	out_count,	The number of input formats
//							support is returned here.
//
// Postconditions:
//
// Returns: the list of input formats and the number of input
// formats through the out_count parameter, if out_count is NULL,
// NULL is returned
// ---------------------------------------------------------------
const translation_format *
TIFFTranslator::InputFormats(int32 *out_count) const
{
	if (out_count) {
		*out_count = sizeof(gInputFormats) /
			sizeof(translation_format);
		return gInputFormats;
	} else
		return NULL;
}

// ---------------------------------------------------------------
// OutputFormats
//
// Returns a list of output formats supported by this translator.
//
// Preconditions:
//
// Parameters:	out_count,	The number of output formats
//							support is returned here.
//
// Postconditions:
//
// Returns: the list of output formats and the number of output
// formats through the out_count parameter, if out_count is NULL,
// NULL is returned
// ---------------------------------------------------------------	
const translation_format *
TIFFTranslator::OutputFormats(int32 *out_count) const
{
	if (out_count) {
		*out_count = sizeof(gOutputFormats) /
			sizeof(translation_format);
		return gOutputFormats;
	} else
		return NULL;
}

// ---------------------------------------------------------------
// identify_bits_header
//
// Determines if the data in inSource is in the
// B_TRANSLATOR_BITMAP ('bits') format. If it is, it returns 
// info about the data in inSource to outInfo and pheader.
//
// Preconditions:
//
// Parameters:	inSource,	The source of the image data
//
//				outInfo,	Information about the translator
//							is copied here
//
//				amtread,	Amount of data read from inSource
//							before this function was called
//
//				read,		Pointer to the data that was read
// 							in before this function was called
//
//				pheader,	The bits header is copied here after
//							it is read in from inSource
//
// Postconditions:
//
// Returns: B_NO_TRANSLATOR,	if the data does not look like
//								bits format data
//
// B_ERROR,	if the header data could not be converted to host
//			format
//
// B_OK,	if the data looks like bits data and no errors were
//			encountered
// ---------------------------------------------------------------
status_t 
identify_bits_header(BPositionIO *inSource, translator_info *outInfo,
	ssize_t amtread, uint8 *read, TranslatorBitmap *pheader = NULL)
{
	TranslatorBitmap header;
		
	memcpy(&header, read, amtread);
		// copy portion of header already read in
	// read in the rest of the header
	ssize_t size = sizeof(TranslatorBitmap) - amtread;
	if (inSource->Read(
		(reinterpret_cast<uint8 *> (&header)) + amtread, size) != size)
		return B_NO_TRANSLATOR;
		
	// convert to host byte order
	if (swap_data(B_UINT32_TYPE, &header, sizeof(TranslatorBitmap),
		B_SWAP_BENDIAN_TO_HOST) != B_OK)
		return B_ERROR;
		
	// check if header values are reasonable
	if (header.colors != B_RGB32 &&
		header.colors != B_RGB32_BIG &&
		header.colors != B_RGBA32 &&
		header.colors != B_RGBA32_BIG &&
		header.colors != B_RGB24 &&
		header.colors != B_RGB24_BIG &&
		header.colors != B_RGB16 &&
		header.colors != B_RGB16_BIG &&
		header.colors != B_RGB15 &&
		header.colors != B_RGB15_BIG &&
		header.colors != B_RGBA15 &&
		header.colors != B_RGBA15_BIG &&
		header.colors != B_CMAP8 &&
		header.colors != B_GRAY8 &&
		header.colors != B_GRAY1 &&
		header.colors != B_CMYK32 &&
		header.colors != B_CMY32 &&
		header.colors != B_CMYA32 &&
		header.colors != B_CMY24)
		return B_NO_TRANSLATOR;
	if (header.rowBytes * (header.bounds.Height() + 1) != header.dataSize)
		return B_NO_TRANSLATOR;
			
	if (outInfo) {
		outInfo->type = B_TRANSLATOR_BITMAP;
		outInfo->group = B_TRANSLATOR_BITMAP;
		outInfo->quality = BBT_IN_QUALITY;
		outInfo->capability = BBT_IN_CAPABILITY;
		strcpy(outInfo->name, "Be Bitmap Format (TIFFTranslator)");
		strcpy(outInfo->MIME, "image/x-be-bitmap");
	}
	
	if (pheader) {
		pheader->magic = header.magic;
		pheader->bounds = header.bounds;
		pheader->rowBytes = header.rowBytes;
		pheader->colors = header.colors;
		pheader->dataSize = header.dataSize;
	}
	
	return B_OK;
}

// If this TIFF has unsupported features,
// return B_NO_TRANSLATOR, otherwise, 
// return B_OK
status_t
check_tiff_fields(TiffIfd &ifd, TiffDetails *pdetails)
{
	TiffDetails dtls;
	memset(&dtls, 0, sizeof(TiffDetails));
	
	try {
		// Extra Samples are not yet supported
		if (ifd.HasField(TAG_EXTRA_SAMPLES))
			return B_NO_TRANSLATOR;
			
		// Only the default values are supported for the
		// following fields.  HasField is called so that
		// if a field is not present, a TiffIfdFieldNotFoundException
		// will not be thrown when GetUint is called.
		// (If they are not present, that is fine, but if
		// they are present and have a non-default value,
		// that is a problem)	
		if (ifd.HasField(TAG_FILL_ORDER) &&
			ifd.GetUint(TAG_FILL_ORDER) != 1)
			return B_NO_TRANSLATOR;
		if (ifd.HasField(TAG_ORIENTATION) &&
			ifd.GetUint(TAG_ORIENTATION) != 1)
			return B_NO_TRANSLATOR;
		if (ifd.HasField(TAG_PLANAR_CONFIGURATION) &&
			ifd.GetUint(TAG_PLANAR_CONFIGURATION) != 1)
			return B_NO_TRANSLATOR;
		if (ifd.HasField(TAG_SAMPLE_FORMAT) &&
			ifd.GetUint(TAG_SAMPLE_FORMAT) != 1)
			return B_NO_TRANSLATOR;
			
		// Copy fields useful to TIFFTranslator
		dtls.width			= ifd.GetUint(TAG_IMAGE_WIDTH);
		dtls.height			= ifd.GetUint(TAG_IMAGE_HEIGHT);
		dtls.compression	= ifd.GetUint(TAG_COMPRESSION);
		dtls.interpretation	= ifd.GetUint(TAG_PHOTO_INTERPRETATION);
		
		if (dtls.compression != COMPRESSION_NONE &&
			dtls.compression != COMPRESSION_PACKBITS)
			return B_NO_TRANSLATOR;
		
		// Currently, only uncompressed 
		// RGB or CMYK images are supported
		if (dtls.interpretation == PHOTO_RGB) {
			if (ifd.GetUint(TAG_SAMPLES_PER_PIXEL) != 3)
				return B_NO_TRANSLATOR;
			if (ifd.GetCount(TAG_BITS_PER_SAMPLE) != 3 ||
				ifd.GetUint(TAG_BITS_PER_SAMPLE, 1) != 8 ||
				ifd.GetUint(TAG_BITS_PER_SAMPLE, 2) != 8 ||
				ifd.GetUint(TAG_BITS_PER_SAMPLE, 3) != 8)
				return B_NO_TRANSLATOR;
				
			dtls.imageType = TIFF_RGB;
			
		} else if (dtls.interpretation == PHOTO_SEPARATED) {
			// CMYK (default ink set)
			// is the only ink set supported
			if (ifd.HasField(TAG_INK_SET) &&
				ifd.GetUint(TAG_INK_SET) != INK_SET_CMYK)
				return B_NO_TRANSLATOR;
				
			if (ifd.GetUint(TAG_SAMPLES_PER_PIXEL) != 4)
				return B_NO_TRANSLATOR;
			if (ifd.GetCount(TAG_BITS_PER_SAMPLE) != 4 ||
				ifd.GetUint(TAG_BITS_PER_SAMPLE, 1) != 8 ||
				ifd.GetUint(TAG_BITS_PER_SAMPLE, 2) != 8 ||
				ifd.GetUint(TAG_BITS_PER_SAMPLE, 3) != 8 ||
				ifd.GetUint(TAG_BITS_PER_SAMPLE, 4) != 8)
				return B_NO_TRANSLATOR;
				
			dtls.imageType = TIFF_CMYK;
			
		} else
			return B_NO_TRANSLATOR;
		
		if (!ifd.HasField(TAG_ROWS_PER_STRIP))
			dtls.rowsPerStrip = DEFAULT_ROWS_PER_STRIP;
		else
			dtls.rowsPerStrip = ifd.GetUint(TAG_ROWS_PER_STRIP);
		dtls.stripsPerImage =
			(dtls.height + dtls.rowsPerStrip - 1) / dtls.rowsPerStrip;
			
		if (ifd.GetCount(TAG_STRIP_OFFSETS) != dtls.stripsPerImage ||
			ifd.GetCount(TAG_STRIP_BYTE_COUNTS) != dtls.stripsPerImage)
			return B_NO_TRANSLATOR;
		
		printf("width: %d\nheight: %d\ncompression: %d\ninterpretation: %d\n",
			dtls.width, dtls.height, dtls.compression,
			dtls.interpretation);
		
		// return read in details if output 
		// pointer is supplied
		if (pdetails) {
			ifd.GetUintArray(TAG_STRIP_OFFSETS, &dtls.pstripOffsets);
			ifd.GetUintArray(TAG_STRIP_BYTE_COUNTS, &dtls.pstripByteCounts);
			
			memcpy(pdetails, &dtls, sizeof(TiffDetails));
		}
			
	} catch (TiffIfdException) {
	
		printf("-- Caught TiffIfdException --\n");
				
		delete[] dtls.pstripOffsets;
		dtls.pstripOffsets = NULL;
		delete[] dtls.pstripByteCounts;
		dtls.pstripByteCounts = NULL;

		return B_NO_TRANSLATOR;
	}
		
	return B_OK;
}

status_t
identify_tiff_header(BPositionIO *inSource, translator_info *outInfo,
	ssize_t amtread, uint8 *read, swap_action swp, TiffDetails *pdetails = NULL)
{
	if (amtread != 4)
		return B_ERROR;
		
	uint32 firstIFDOffset = 0;
	ssize_t nread = inSource->Read(&firstIFDOffset, 4);
	if (nread != 4) {
		printf("Unable to read first IFD offset\n");
		return B_NO_TRANSLATOR;
	}
	if (swap_data(B_UINT32_TYPE, &firstIFDOffset, sizeof(uint32), swp) != B_OK) {
		printf("swap_data() error\n");
		return B_ERROR;
	}
	printf("First IFD: 0x%.8lx\n", firstIFDOffset);
	TiffIfd ifd(firstIFDOffset, *inSource, swp);
	
	status_t initcheck;
	initcheck = ifd.InitCheck();
	printf("Init Check: %d\n",
		static_cast<int>(initcheck));
		
	// Read in some fields in order to determine whether or not
	// this particular TIFF image is supported by this translator
	
	// Check the required fields
	if (initcheck == B_OK) {
		if (outInfo) {
			outInfo->type = B_TIFF_FORMAT;
			outInfo->group = B_TRANSLATOR_BITMAP;
			outInfo->quality = TIFF_IN_QUALITY;
			outInfo->capability = TIFF_IN_CAPABILITY;
			strcpy(outInfo->MIME, "image/tiff");
			strcpy(outInfo->name, "TIFF Image");
		}
		
		return check_tiff_fields(ifd, pdetails);
	}
		
	if (initcheck != B_ERROR && initcheck != B_NO_MEMORY)
		// return B_NO_TRANSLATOR if unexpected data was encountered
		return B_NO_TRANSLATOR;
	else
		// return B_ERROR or B_NO_MEMORY
		return initcheck;
}

// ---------------------------------------------------------------
// Identify
//
// Examines the data from inSource and determines if it is in a
// format that this translator knows how to work with.
//
// Preconditions:
//
// Parameters:	inSource,	where the data to examine is
//
//				inFormat,	a hint about the data in inSource,
//							it is ignored since it is only a hint
//
//				ioExtension,	configuration settings for the
//								translator (not used)
//
//				outInfo,	information about what data is in
//							inSource and how well this translator
//							can handle that data is stored here
//
//				outType,	The format that the user wants
//							the data in inSource to be
//							converted to
//
// Postconditions:
//
// Returns: B_NO_TRANSLATOR,	if this translator can't handle
//								the data in inSource
//
// B_ERROR,	if there was an error converting the data to the host
//			format
//
// B_BAD_VALUE, if the settings in ioExtension are bad
//
// B_OK,	if this translator understood the data and there were
//			no errors found
//
// Other errors if BPositionIO::Read() returned an error value
// ---------------------------------------------------------------
status_t
TIFFTranslator::Identify(BPositionIO *inSource,
	const translation_format *inFormat, BMessage *ioExtension,
	translator_info *outInfo, uint32 outType)
{
	if (!outType)
		outType = B_TRANSLATOR_BITMAP;
	if (outType != B_TRANSLATOR_BITMAP && outType != B_TIFF_FORMAT)
		return B_NO_TRANSLATOR;
	
	// Convert the magic numbers to the various byte orders so that
	// I won't have to convert the data read in to see whether or not
	// it is a supported type
	uint32 nbits = B_TRANSLATOR_BITMAP;
	if (swap_data(B_UINT32_TYPE, &nbits, sizeof(uint32),
		B_SWAP_HOST_TO_BENDIAN) != B_OK)
		return B_ERROR;
	
	// Read in the magic number and determine if it
	// is a supported type
	uint8 ch[4];
	if (inSource->Read(ch, 4) != 4)
		return B_NO_TRANSLATOR;
		
	// Read settings from ioExtension
	bool bheaderonly = false, bdataonly = false;
	if (ioExtension) {
		if (ioExtension->FindBool(B_TRANSLATOR_EXT_HEADER_ONLY, &bheaderonly))
			// if failed, make sure bool is default value
			bheaderonly = false;
		if (ioExtension->FindBool(B_TRANSLATOR_EXT_DATA_ONLY, &bdataonly))
			// if failed, make sure bool is default value
			bdataonly = false;
			
		if (bheaderonly && bdataonly)
			// can't both "only write the header" and "only write the data"
			// at the same time
			return B_BAD_VALUE;
	}
	
	uint32 n32ch;
	memcpy(&n32ch, ch, sizeof(uint32));
	// if B_TRANSLATOR_BITMAP type	
	if (n32ch == nbits)
		return identify_bits_header(inSource, outInfo, 4, ch);
		
	// Might be TIFF image
	else {
		// TIFF Byte Order / Magic
		const uint8 kleSig[] = { 0x49, 0x49, 0x2a, 0x00 };
		const uint8 kbeSig[] = { 0x4d, 0x4d, 0x00, 0x2a };
		
		swap_action swp;
		if (memcmp(ch, kleSig, 4) == 0) {
			swp = B_SWAP_LENDIAN_TO_HOST;
			printf("Byte Order: little endian\n");
		} else if (memcmp(ch, kbeSig, 4) == 0) {
			swp = B_SWAP_BENDIAN_TO_HOST;
			printf("Byte Order: big endian\n");		
		} else {
			// If not a TIFF or a Be Bitmap image
			printf("Invalid byte order value\n");
			return B_NO_TRANSLATOR;
		}
		
		return identify_tiff_header(inSource, outInfo, 4, ch, swp);
	}
}

void
tiff_to_bits(uint8 *ptiff, uint32 tifflen, uint8 *pbits, uint16 type)
{
	uint8 *ptiffend = ptiff + tifflen;
	
	switch (type) {
		case TIFF_RGB:
			while (ptiff < ptiffend) {
				pbits[2] = ptiff[0];
				pbits[1] = ptiff[1];
				pbits[0] = ptiff[2];

				ptiff += 3;
				pbits += 4;
			}
			break;
			
		case TIFF_CMYK:
		{
			while (ptiff < ptiffend) {
				int32 comp;
				comp = 255 - ptiff[0] - ptiff[3];
				pbits[2] = (comp < 0) ? 0 : comp;

				comp = 255 - ptiff[1] - ptiff[3];
				pbits[1] = (comp < 0) ? 0 : comp;
				
				comp = 255 - ptiff[2] - ptiff[3];
				pbits[0] = (comp < 0) ? 0 : comp;
				
				ptiff += 4;
				pbits += 4;
			}
			break;
		}
	}
}

// unpack the Packbits compressed data from
// pstreambuf and put it into tiffbuffer
ssize_t
unpack(StreamBuffer *pstreambuf, uint8 *tiffbuffer, uint32 tiffbufferlen)
{
	uint32 tiffwrit = 0, read = 0, len;
	while (tiffwrit < tiffbufferlen) {
	
		uint8 uctrl;
		if (pstreambuf->Read(&uctrl, 1) != 1)
			return B_ERROR;
		read++;
			
		int32 control;
		control = static_cast<signed char>(uctrl);
				
		// copy control + 1 bytes literally
		if (control >= 0 && control <= 127) {
		
			len = control + 1;
			if (tiffwrit + len > tiffbufferlen)
				return B_ERROR;
			if (pstreambuf->Read(tiffbuffer + tiffwrit, len) !=
				static_cast<ssize_t>(len))
				return B_ERROR;
			read += len;
				
			tiffwrit += len;
		
		// copy the next byte (-control) + 1 times
		} else if (control >= -127 && control <= -1) {
		
			uint8 byt;
			if (pstreambuf->Read(&byt, 1) != 1)
				return B_ERROR;
			read++;
				
			len = (-control) + 1;
			if (tiffwrit + len > tiffbufferlen)
				return B_ERROR;
			memset(tiffbuffer + tiffwrit, byt, len);
			
			tiffwrit += len;			
		}
	}
	
	return read;
}

status_t
translate_from_tiff(BPositionIO *inSource, ssize_t amtread, uint8 *read,
	swap_action swp, uint32 outType, BPositionIO *outDestination)
{
	// Can only output to bits for now
	if (outType != B_TRANSLATOR_BITMAP)
		return B_NO_TRANSLATOR;

	status_t result;
	
	TiffDetails details;
	result = identify_tiff_header(inSource, NULL,
		amtread, read, swp, &details);
	if (result == B_OK) {
		// If the TIFF is supported by this translator
		
		TranslatorBitmap bitsHeader;
		bitsHeader.magic = B_TRANSLATOR_BITMAP;
		bitsHeader.bounds.left = 0;
		bitsHeader.bounds.top = 0;
		bitsHeader.bounds.right = details.width - 1;
		bitsHeader.bounds.bottom = details.height - 1;
		bitsHeader.rowBytes = 4 * details.width;
		bitsHeader.colors = B_RGB32;
		bitsHeader.dataSize = bitsHeader.rowBytes * details.height;
		
		// write out Be's Bitmap header
		if (swap_data(B_UINT32_TYPE, &bitsHeader,
			sizeof(TranslatorBitmap), B_SWAP_HOST_TO_BENDIAN) != B_OK)
			return B_ERROR;
		outDestination->Write(&bitsHeader, sizeof(TranslatorBitmap));
		
		uint32 inbufferlen, outbufferlen;
		outbufferlen = 4 * details.width;
		switch (details.imageType) {
			case TIFF_RGB:
				inbufferlen = 3 * details.width;
				break;
				
			case TIFF_CMYK:
				inbufferlen = 4 * details.width;
				break;
				
			default:
				// just to be safe
				return B_NO_TRANSLATOR;
		}
		
		uint8 *inbuffer = new uint8[inbufferlen],
			*outbuffer = new uint8[outbufferlen];
		if (!inbuffer || !outbuffer)
			return B_NO_MEMORY;
		// set all channels to 0xff so that I won't have
		// to set the alpha byte to 0xff every time
		memset(outbuffer, 0xff, outbufferlen);
		
		// buffer for making reading compressed data
		// fast and convenient
		StreamBuffer *pstreambuf = NULL;
		if (details.compression == COMPRESSION_PACKBITS) {
			pstreambuf = new StreamBuffer(inSource, 2048, false);
			if (pstreambuf->InitCheck() != B_OK)
				return B_NO_MEMORY;
		}
			
		for (uint32 i = 0; i < details.stripsPerImage; i++) {
			uint32 read = 0;
			
			// If Packbits compression, prepare streambuffer
			// for reading
			if (details.compression == COMPRESSION_PACKBITS &&
				!pstreambuf->Seek(details.pstripOffsets[i]))
				return B_NO_TRANSLATOR;
				
			// Read / Write one line at a time for each
			// line in the strip
			while (read < details.pstripByteCounts[i]) {
			
				ssize_t ret = 0;
				if (details.compression == COMPRESSION_NONE) {
					ret = inSource->ReadAt(details.pstripOffsets[i] + read,
						inbuffer, inbufferlen);
					if (ret != static_cast<ssize_t>(inbufferlen))
						break;
						
				} else if (details.compression == COMPRESSION_PACKBITS) {
					ret = unpack(pstreambuf, inbuffer, inbufferlen);
					if (ret < 1)
						break;
				}
					
				read += ret;
				tiff_to_bits(inbuffer, inbufferlen, outbuffer,
					details.imageType);
				outDestination->Write(outbuffer, outbufferlen);
			}
			// If while loop was broken...
			if (read < details.pstripByteCounts[i])
				break;
		}
		
		// Clean up
		if (pstreambuf) {
			delete pstreambuf;
			pstreambuf = NULL;
		}
		
		delete[] inbuffer;
		inbuffer = NULL;
		delete[] outbuffer;
		outbuffer = NULL;
		
		delete[] details.pstripOffsets;
		details.pstripOffsets = NULL;
		delete[] details.pstripByteCounts;
		details.pstripByteCounts = NULL;
		
		return B_OK;
		
	} else
		return result;
}

// ---------------------------------------------------------------
// Translate
//
// Translates the data in inSource to the type outType and stores
// the translated data in outDestination.
//
// Preconditions:
//
// Parameters:	inSource,	the data to be translated
// 
//				inInfo,	hint about the data in inSource (not used)
//
//				ioExtension,	configuration options for the
//								translator
//
//				outType,	the type to convert inSource to
//
//				outDestination,	where the translated data is
//								put
//
// Postconditions:
//
// Returns: B_BAD_VALUE, if the options in ioExtension are bad
//
// B_NO_TRANSLATOR, if this translator doesn't understand the data
//
// B_ERROR, if there was an error allocating memory or converting
//          data
//
// B_OK, if all went well
// ---------------------------------------------------------------
status_t
TIFFTranslator::Translate(BPositionIO *inSource,
		const translator_info *inInfo, BMessage *ioExtension,
		uint32 outType, BPositionIO *outDestination)
{
	if (!outType)
		outType = B_TRANSLATOR_BITMAP;
	if (outType != B_TRANSLATOR_BITMAP && outType != B_TIFF_FORMAT)
		return B_NO_TRANSLATOR;
	
	// Convert the magic numbers to the various byte orders so that
	// I won't have to convert the data read in to see whether or not
	// it is a supported type
	uint32 nbits = B_TRANSLATOR_BITMAP;
	if (swap_data(B_UINT32_TYPE, &nbits, sizeof(uint32),
		B_SWAP_HOST_TO_BENDIAN) != B_OK)
		return B_ERROR;
	
	// Read in the magic number and determine if it
	// is a supported type
	uint8 ch[4];
	inSource->Seek(0, SEEK_SET);
	if (inSource->Read(ch, 4) != 4)
		return B_NO_TRANSLATOR;
		
	// Read settings from ioExtension
	bool bheaderonly = false, bdataonly = false;
	if (ioExtension) {
		if (ioExtension->FindBool(B_TRANSLATOR_EXT_HEADER_ONLY, &bheaderonly))
			// if failed, make sure bool is default value
			bheaderonly = false;
		if (ioExtension->FindBool(B_TRANSLATOR_EXT_DATA_ONLY, &bdataonly))
			// if failed, make sure bool is default value
			bdataonly = false;
			
		if (bheaderonly && bdataonly)
			// can't both "only write the header" and "only write the data"
			// at the same time
			return B_BAD_VALUE;
	}
	
	uint32 n32ch;
	memcpy(&n32ch, ch, sizeof(uint32));
	// if B_TRANSLATOR_BITMAP type	
	if (n32ch == nbits)
		return B_NO_TRANSLATOR;
			// Not implemented yet
		
	// Might be TIFF image
	else {
		// TIFF Byte Order / Magic
		const uint8 kleSig[] = { 0x49, 0x49, 0x2a, 0x00 };
		const uint8 kbeSig[] = { 0x4d, 0x4d, 0x00, 0x2a };
		
		swap_action swp;
		if (memcmp(ch, kleSig, 4) == 0) {
			swp = B_SWAP_LENDIAN_TO_HOST;
			printf("Byte Order: little endian\n");
		} else if (memcmp(ch, kbeSig, 4) == 0) {
			swp = B_SWAP_BENDIAN_TO_HOST;
			printf("Byte Order: big endian\n");		
		} else {
			// If not a TIFF or a Be Bitmap image
			printf("Invalid byte order value\n");
			return B_NO_TRANSLATOR;
		}
		
		return translate_from_tiff(inSource, 4, ch, swp, outType,
			outDestination);
	}
}

// ---------------------------------------------------------------
// MakeConfigurationView
//
// Makes a BView object for configuring / displaying info about
// this translator. 
//
// Preconditions:
//
// Parameters:	ioExtension,	configuration options for the
//								translator
//
//				outView,		the view to configure the
//								translator is stored here
//
//				outExtent,		the bounds of the view are
//								stored here
//
// Postconditions:
//
// Returns: B_BAD_VALUE if outView or outExtent is NULL,
//			B_NO_MEMORY if the view couldn't be allocated,
//			B_OK if no errors
// ---------------------------------------------------------------
status_t
TIFFTranslator::MakeConfigurationView(BMessage *ioExtension, BView **outView,
	BRect *outExtent)
{
	if (!outView || !outExtent)
		return B_BAD_VALUE;

	TIFFView *view = new TIFFView(BRect(0, 0, 225, 175),
		"TIFFTranslator Settings", B_FOLLOW_ALL, B_WILL_DRAW);
	if (!view)
		return B_NO_MEMORY;

	*outView = view;
	*outExtent = view->Bounds();

	return B_OK;
}
