/*
 * Copyright 2005, Haiku Inc.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Axel Dörfler, axeld@pinc-software.de
 */
#ifndef WORKSPACES_LAYER_H
#define WORKSPACES_LAYER_H


#include "Layer.h"

class WindowLayer;


class WorkspacesLayer : public Layer {
	public:
		WorkspacesLayer(BRect frame, const char* name, int32 token,
			uint32 resize, uint32 flags, DrawingEngine* driver);
		virtual ~WorkspacesLayer();

		virtual	void Draw(const BRect& updateRect);

	private:
		void _GetGrid(int32& columns, int32& rows);
		BRect _WorkspaceAt(int32 i);
		BRect _WindowFrame(const BRect& workspaceFrame,
					const BRect& screenFrame, const BRect& windowFrame);

		void _DrawWindow(const BRect& workspaceFrame,
					const BRect& screenFrame, WindowLayer* window,
					BRegion& backgroundRegion, bool active);
		void _DrawWorkspace(int32 index);

		void _DarkenColor(RGBColor& color) const;
};

#endif	// WORKSPACES_LAYER_H
