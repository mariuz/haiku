//------------------------------------------------------------------------------
//	Copyright (c) 2001-2002, OpenBeOS
//
//	Permission is hereby granted, free of charge, to any person obtaining a
//	copy of this software and associated documentation files (the "Software"),
//	to deal in the Software without restriction, including without limitation
//	the rights to use, copy, modify, merge, publish, distribute, sublicense,
//	and/or sell copies of the Software, and to permit persons to whom the
//	Software is furnished to do so, subject to the following conditions:
//
//	The above copyright notice and this permission notice shall be included in
//	all copies or substantial portions of the Software.
//
//	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
//	IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
//	FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
//	AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
//	LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
//	FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
//	DEALINGS IN THE SOFTWARE.
//
//	File Name:		RegistrarDefs.h
//	Author(s):		Ingo Weinhold (bonefish@users.sf.net)
//	Description:	API classes - registrar interface.
//------------------------------------------------------------------------------

#ifndef REGISTRAR_DEFS_H
#define REGISTRAR_DEFS_H

#include <Errors.h>
#include <Roster.h>

// names
extern const char *kRegistrarSignature;
extern const char *kRosterThreadName;
extern const char *kRosterPortName;
extern const char *kRAppLooperPortName;

// message constants
enum {
	// replies
	B_REG_SUCCESS							= 'rgsu',
	B_REG_ERROR								= 'rger',
	B_REG_RESULT							= 'rgrz',
	// general requests
	B_REG_GET_MIME_MESSENGER				= 'rgmm',
	B_REG_GET_CLIPBOARD_MESSENGER			= 'rgcm',
	// roster requests
	B_REG_ADD_APP							= 'rgaa',
	B_REG_COMPLETE_REGISTRATION				= 'rgcr',
	B_REG_IS_APP_PRE_REGISTERED				= 'rgip',
	B_REG_REMOVE_PRE_REGISTERED_APP			= 'rgrp',
	B_REG_REMOVE_APP						= 'rgra',
	B_REG_SET_THREAD_AND_TEAM				= 'rgtt',
	B_REG_SET_SIGNATURE						= 'rgss',
	B_REG_GET_APP_INFO						= 'rgai',
	B_REG_GET_APP_LIST						= 'rgal',
	B_REG_ACTIVATE_APP						= 'rgac',
	B_REG_BROADCAST							= 'rgbc',
	B_REG_START_WATCHING					= 'rgwa',
	B_REG_STOP_WATCHING						= 'rgsw',
	// MIME requests
	B_REG_MIME_SET_PARAM					= 'rgsp',
	B_REG_MIME_DELETE_PARAM					= 'rgdp',
	B_REG_MIME_START_WATCHING				= 'rgwb',
	B_REG_MIME_STOP_WATCHING				= 'rgwe',
	B_REG_MIME_INSTALL						= 'rgin',
	B_REG_MIME_DELETE						= 'rgdl',
	B_REG_MIME_GET_INSTALLED_TYPES			= 'rgit',
	B_REG_MIME_GET_INSTALLED_SUPERTYPES		= 'rgis',
	B_REG_MIME_GET_SUPPORTING_APPS			= 'rgsa',
	B_REG_MIME_GET_ASSOCIATED_TYPES			= 'rgat',
	B_REG_MIME_SNIFF						= 'rgsn',
	B_REG_MIME_UPDATE_MIME_INFO				= 'rgup',
	B_REG_MIME_CREATE_APP_META_MIME			= 'rgca',
	B_REG_MIME_UPDATE_THREAD_FINISHED		= 'rgtf',
	// message runner requests
	B_REG_REGISTER_MESSAGE_RUNNER			= 'rgrr',
	B_REG_UNREGISTER_MESSAGE_RUNNER			= 'rgru',
	B_REG_SET_MESSAGE_RUNNER_PARAMS			= 'rgrx',
	B_REG_GET_MESSAGE_RUNNER_INFO			= 'rgri',
	// internal registrar messages
	B_REG_ROSTER_SANITY_EVENT				= 'rgir',
	// clipboard handler requests
	B_REG_ADD_CLIPBOARD					= 'rgca',
	B_REG_GET_CLIPBOARD_COUNT				= 'rgcc',
	B_REG_CLIPBOARD_START_WATCHING			= 'rgcw',
	B_REG_CLIPBOARD_STOP_WATCHING			= 'rgcx',
	B_REG_DOWNLOAD_CLIPBOARD				= 'rgcd',
	B_REG_UPLOAD_CLIPBOARD				= 'rgcu',
};

// B_REG_MIME_SET_PARAM "which" constants 
enum {
	B_REG_MIME_APP_HINT				= 'rgmh',
	B_REG_MIME_ATTR_INFO			= 'rgma',
	B_REG_MIME_DESCRIPTION			= 'rgmd',
	B_REG_MIME_FILE_EXTENSIONS		= 'rgmf',
	B_REG_MIME_ICON					= 'rgmi',
	B_REG_MIME_ICON_FOR_TYPE		= 'rgm4',
	B_REG_MIME_PREFERRED_APP		= 'rgmp',
	B_REG_MIME_SNIFFER_RULE			= 'rgmr',
	B_REG_MIME_SUPPORTED_TYPES		= 'rgms',
};

// type constants
enum {
	B_REG_APP_INFO_TYPE				= 'rgai',	// app_info
};

// error constants
#define B_REGISTRAR_ERROR_BASE		(B_ERRORS_END + 1)

enum {
	B_REG_ALREADY_REGISTERED		= B_REGISTRAR_ERROR_BASE,
		// A team tries to register a second time.
	B_REG_APP_NOT_REGISTERED,
	B_REG_APP_NOT_PRE_REGISTERED,
};

// misc constants
enum {
	B_REG_DEFAULT_APP_FLAGS			= B_MULTIPLE_LAUNCH | B_ARGV_ONLY
									  | _B_APP_INFO_RESERVED1_,
	B_REG_APP_LOOPER_PORT_CAPACITY	= 100,
};

// structs

// a flat app_info -- to be found in B_REG_APP_INFO_TYPE message fields
struct flat_app_info {
	app_info	info;
	char		ref_name[B_FILE_NAME_LENGTH + 1];
};

#endif	// REGISTRAR_DEFS_H

