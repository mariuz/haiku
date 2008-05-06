/*
 * Copyright 2004-2007, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include "smp.h"
#include "video.h"

#include <boot/menu.h>
#include <boot/platform/generic/text_menu.h>
#include <safemode.h>


void
platform_add_menus(Menu *menu)
{
	MenuItem *item;

	switch (menu->Type()) {
		case MAIN_MENU:
			menu->AddItem(item = new(nothrow) MenuItem("Select fail-safe video mode", video_mode_menu()));
			item->SetTarget(video_mode_hook);
			break;
		case SAFE_MODE_MENU:
			menu->AddItem(item = new(nothrow) MenuItem("Use fail-safe video mode"));
			item->SetType(MENU_ITEM_MARKABLE);
			item->SetData(B_SAFEMODE_FAIL_SAFE_VIDEO_MODE);
			item->SetHelpText("The system will use VESA mode and won't try to open any video graphics driver");

			smp_add_safemode_menus(menu);

			menu->AddItem(item = new(nothrow) MenuItem("Don't call the BIOS"));
			item->SetType(MENU_ITEM_MARKABLE);

			menu->AddItem(item = new(nothrow) MenuItem("Disable APM"));
			item->SetType(MENU_ITEM_MARKABLE);
			item->SetData(B_SAFEMODE_DISABLE_APM);
			item->SetHelpText("This overrides the APM setting in the kernel settings file");

			menu->AddItem(item = new(nothrow) MenuItem("Disable ACPI"));
			item->SetType(MENU_ITEM_MARKABLE);
			item->SetData(B_SAFEMODE_DISABLE_ACPI);
			item->SetHelpText("This overrides the ACPI setting in the kernel settings file");
			break;
		default:
			break;
	}
}


void 
platform_update_menu_item(Menu *menu, MenuItem *item)
{
	platform_generic_update_text_menu_item(menu, item);
}


void
platform_run_menu(Menu *menu)
{
	platform_generic_run_text_menu(menu);
}

