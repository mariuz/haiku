/*
 * Copyright 2006-2010, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef	_GROUP_VIEW_H
#define	_GROUP_VIEW_H


#include <GroupLayout.h>
#include <View.h>


class BGroupView : public BView {
public:
								BGroupView(
									enum orientation orientation = B_HORIZONTAL,
									float spacing = 0.0f);
								BGroupView(const char* name,
									enum orientation orientation = B_HORIZONTAL,
									float spacing = 0.0f);
	virtual						~BGroupView();

	virtual	void				SetLayout(BLayout* layout);

			BGroupLayout*		GroupLayout() const;
};


#endif	// _GROUP_VIEW_H
