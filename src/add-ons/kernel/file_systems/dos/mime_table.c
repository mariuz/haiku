/*
	Copyright 1999-2001, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
	
	extended: 2001-12-11 by Marcus Overhagen
*/

struct ext_mime {
	char *extension;
	char *mime;
};

struct ext_mime mimes[] = {
	{ "gz", "application/x-gzip" },
	{ "hqx", "application/x-binhex40" },
	{ "lha", "application/x-lharc" },
	{ "lzh", "application/x-lharc" },
	{ "pcl", "application/x-pcl" },
	{ "pdf", "application/pdf" },
	{ "ps", "application/postscript" },
	{ "sit", "application/x-stuff-it" },
	{ "tar", "application/x-tar" },
	{ "tgz", "application/x-gzip" },
	{ "uue", "application/x-uuencode" },
	{ "z", "application/x-compress" },
	{ "zip", "application/zip" },
	{ "zoo", "application/x-zoo" },
	{ "pkg", "application/x-scode-UPkg" },
	{ "vdwn", "application/x-scode-UPkg" },
	{ "proj", "application/x-mw-project" },
	{ "swf", "application/x-shockwave-flash" },
	{ "clp", "application/x-codeliege-project" },

	{ "aif", "audio/x-aiff" },
	{ "aifc", "audio/x-aifc" },
	{ "aiff", "audio/x-aiff" },
	{ "au", "audio/basic" },
	{ "mid", "audio/x-midi" },
	{ "midi", "audio/x-midi" },
	{ "mod", "audio/mod" },
	{ "ra", "audio/x-real-audio" },
	{ "wav", "audio/x-wav" },
	{ "mp2", "audio/x-mpeg" }, 
	{ "mp3", "audio/x-mpeg" }, 
	{ "ogg", "audio/x-vorbis" }, 
	{ "asf", "application/x-asf" }, 
	{ "riff", "application/x-riff" }, 

	{ "bmp", "image/x-bmp" },
	{ "fax", "image/g3fax" },
	{ "gif", "image/gif" },
	{ "iff", "image/x-iff" },
	{ "jpg", "image/jpeg" },
	{ "jpeg", "image/jpeg" },
	{ "pbm", "image/x-portable-bitmap" },
	{ "pcx", "image/x-pcx" },
	{ "pgm", "image/x-portable-graymap" },
	{ "png", "image/png" },
	{ "ppm", "image/x-portable-pixmap" },
	{ "rgb", "image/x-rgb" },
	{ "tga", "image/x-targa" },
	{ "tif", "image/tiff" },
	{ "tiff", "image/tiff" },
	{ "xbm", "image/x-xbitmap" },

	{ "txt", "text/plain" },
	{ "ini", "text/plain" },
	{ "log", "text/plain" },
	{ "bat", "text/plain" },
	{ "doc", "text/plain" },
	{ "cfg", "text/plain" },
	{ "inf", "text/plain" },
	{ "htm", "text/html" },
	{ "html", "text/html" },
	{ "rtf", "text/rtf" },
	{ "c", "text/x-source-code" },
	{ "cc", "text/x-source-code" },
	{ "c++", "text/x-source-code" },
	{ "h", "text/x-source-code" },
	{ "h++", "text/x-source-code" },
	{ "hh", "text/x-source-code" },
	{ "hpp", "text/x-source-code" },
	{ "pl", "text/x-source-code" },
	{ "py", "text/x-source-code" },
	{ "cxx", "text/x-source-code" },
	{ "cpp", "text/x-source-code" },
	{ "S", "text/x-source-code" },
	{ "asm", "text/x-source-code" },
	{ "bas", "text/x-source-code" },
	{ "pas", "text/x-source-code" },
	{ "java", "text/x-source-code" },

	{ "avi", "video/x-msvideo" },
	{ "mov", "video/quicktime" },
	{ "mpg", "video/mpeg" },
	{ "mpeg", "video/mpeg" },
	{ "rm", "application/vnd.rn-realmedia" },
	{ "rn", "application/vnd.rn-realmedia" },

	{ 0, 0 }
};
