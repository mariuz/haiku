/*****************************************************************************/
// print_server Background Application.
//
// Version: 1.0.0d1
//
// The print_server manages the communication between applications and the
// printer and transport drivers.
//
// Authors
//   Ithamar R. Adema
//   Michael Pfeiffer
//
// This application and all source files used in its construction, except 
// where noted, are licensed under the MIT License, and have been written 
// and are:
//
// Copyright (c) 2001 OpenBeOS Project
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

#ifndef PRINTSERVERAPP_H
#define PRINTSERVERAPP_H

#include "ResourceManager.h"
#include "Settings.h"

class PrintServerApp;

#include <Application.h>
#include <Bitmap.h>
#include <OS.h>
#include <String.h>

// global BLocker for synchronisation
extern BLocker* gLock;

/*****************************************************************************/
// PrintServerApp
//
// Application class for print_server.
/*****************************************************************************/
class Printer;
class PrintServerApp : public BApplication
{
	typedef BApplication Inherited;
public:
	PrintServerApp(status_t* err);
	~PrintServerApp();
		
	void Acquire();
	void Release();
	
	bool QuitRequested();
	void MessageReceived(BMessage* msg);
	void NotifyPrinterDeletion(Printer* printer);
	
		// Scripting support, see PrintServerApp.Scripting.cpp
	status_t GetSupportedSuites(BMessage* msg);
	void HandleScriptingCommand(BMessage* msg);
	Printer* GetPrinterFromSpecifier(BMessage* msg);
	BHandler* ResolveSpecifier(BMessage* msg, int32 index, BMessage* spec,
								int32 form, const char* prop);
private:
	bool OpenSettings(BFile& file, const char* name, bool forReading);
	void LoadSettings();
	void SaveSettings();

	status_t SetupPrinterList();

	void     HandleSpooledJobs();
	
	status_t SelectPrinter(const char* printerName);
	status_t CreatePrinter(	const char* printerName, const char* driverName,
							const char* connection, const char* transportName,
							const char* transportPath);

	void     RegisterPrinter(BDirectory* node);
	void     UnregisterPrinter(Printer* printer);
	void     HandleRemovedPrinter(BMessage* msg);
	
	status_t StoreDefaultPrinter();
	status_t RetrieveDefaultPrinter();
	
	status_t FindPrinterNode(const char* name, BNode& node);
	status_t FindPrinterDriver(const char* name, BPath& outPath);
	
	ResourceManager fResourceManager;
	Printer* fDefaultPrinter;
	BBitmap* fSelectedIconMini;
	BBitmap* fSelectedIconLarge;
	vint32   fReferences; 
	sem_id   fHasReferences;
	Settings*fSettings;
	bool     fUseConfigWindow;
	
		// "Classic" BeOS R5 support, see PrintServerApp.R5.cpp
	static status_t async_thread(void* data);
	void AsyncHandleMessage(BMessage* msg);
	void Handle_BeOSR5_Message(BMessage* msg);

};

#endif
