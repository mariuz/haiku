/* 
** Copyright 2004, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
** Distributed under the terms of the OpenBeOS License.
*/
#ifndef PROBE_VIEW_H
#define PROBE_VIEW_H


#include "DataEditor.h"

#include <View.h>
#include <String.h>
#include <Path.h>


class BScrollView;

class HeaderView;
class DataView;


class ProbeView : public BView {
	public:
		ProbeView(BRect rect, entry_ref *ref, const char *attribute = NULL);
		virtual ~ProbeView();

		virtual void DetachedFromWindow();
		virtual void AttachedToWindow();
		virtual void AllAttached();
		virtual void MessageReceived(BMessage *message);

		void AddFileMenuItems(BMenu *menu, int32 index);

		void UpdateSizeLimits();

	private:
		DataEditor		fEditor;
		HeaderView		*fHeaderView;
		DataView		*fDataView;
		BScrollView		*fScrollView;
};

#endif	/* PROBE_VIEW_H */
