/*

Media Windows Header by Sikosis

(C)2003

*/

#ifndef __MEDIAWINDOWS_H__
#define __MEDIAWINDOWS_H__

#include "Media.h"
#include "MediaViews.h"
#include "MediaListItem.h"

class BParameterWeb;
class BContinuousParameter;
class BMultiChannelControl;

class MediaView;
class IconView;
class AvailableViewArea;
class AudioSettingsView;
class AudioMixerView;

class MediaListItem;

class MediaWindow : public BWindow
{
	public:
    	MediaWindow(BRect frame);
	    ~MediaWindow();
    	virtual bool QuitRequested();
	    virtual void MessageReceived(BMessage *message);
	    virtual void FrameResized(float width, float height); 
	private:
		void InitWindow(void);
		
	    MediaView*				ptrMediaView;
	    IconView*  				ptrIconView;
	    AvailableViewArea*		ptrAvailableViewArea;
	    AudioSettingsView*		ptrAudioSettingsView;
	    AudioMixerView*         ptrAudioMixerView;
	    
	    BParameterWeb* 			mParamWeb;
		BMultiChannelControl* 	mGainControl;
		BContinuousParameter* 	mGainParam;
		float 					mScale, mOffset;
		int32 					mControlMin;
};

#endif
