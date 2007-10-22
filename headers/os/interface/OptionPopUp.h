/*
 * Copyright 2001-2007, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */

#ifndef _OPTION_POP_UP_H
#define _OPTION_POP_UP_H

#include <OptionControl.h>


class BMenuField;
class BOptionPopUp : public BOptionControl {
	public:
		BOptionPopUp(BRect frame, const char *name, const char *label, 
				BMessage *message, uint32 resizeMask = B_FOLLOW_LEFT | B_FOLLOW_TOP,
				uint32 flags = B_WILL_DRAW);
		BOptionPopUp(BRect frame, const char *name, const char *label, 
				BMessage *message, bool fixed,
				uint32 resizeMask = B_FOLLOW_LEFT | B_FOLLOW_TOP,
				uint32 flags = B_WILL_DRAW);
		~BOptionPopUp();

		BMenuField*		MenuField();

		virtual	bool		GetOptionAt(int32 index, const char **outName,
								int32 *outValue);
		virtual	void		RemoveOptionAt(int32 index);
		virtual	int32		CountOptions() const;
		virtual	status_t 	AddOptionAt(const char *name, int32 value,
							int32 index);

		virtual	void		AllAttached();
		virtual	void		MessageReceived(BMessage *message);
		virtual	void		SetLabel(const char *text);
		virtual	void		SetValue(int32 value);
		virtual	void		SetEnabled(bool enabled);
		virtual	void		GetPreferredSize(float *width, float *height);
		virtual	void		ResizeToPreferred();
		
		virtual	int32		SelectedOption(const char **outName = 0,
							int32 * outValue = 0) const;
	private:

		BOptionPopUp();
		BOptionPopUp(const BOptionPopUp &clone);
		BOptionPopUp &operator=(const BOptionPopUp &clone);


		virtual	status_t 	_Reserved_OptionControl_0(void *, ...);
		virtual	status_t 	_Reserved_OptionControl_1(void *, ...);
		virtual	status_t 	_Reserved_OptionControl_2(void *, ...);
		virtual	status_t 	_Reserved_OptionControl_3(void *, ...);

		virtual	status_t 	_Reserved_OptionPopUp_0(void *, ...);
		virtual	status_t 	_Reserved_OptionPopUp_1(void *, ...);
		virtual	status_t 	_Reserved_OptionPopUp_2(void *, ...);
		virtual	status_t 	_Reserved_OptionPopUp_3(void *, ...);
		virtual	status_t 	_Reserved_OptionPopUp_4(void *, ...);
		virtual	status_t 	_Reserved_OptionPopUp_5(void *, ...);
		virtual	status_t	_Reserved_OptionPopUp_6(void *, ...);
		virtual	status_t	_Reserved_OptionPopUp_7(void *, ...);


		BMenuField*	fMenuField;
		uint32		_reserved_menu_control_[8];
};

#endif /* _OPTION_POP_UP_H */

