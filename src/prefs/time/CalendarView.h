#ifndef CALENDAR_VIEW_H
#define CALENDAR_VIEW_H

#include <Control.h>
#include <View.h>

class TDay: public BControl {
	public:
		TDay(BRect frame, int day);
		virtual ~TDay();
		virtual void Draw(BRect updaterect);
		virtual void AttachedToWindow();
		virtual void MouseDown(BPoint where);
		virtual void SetValue(int32 value);
		virtual void MakeFocus(bool focused);
		
		void SetTo(BRect frame, int day);
		void SetTo(int day, bool selected = false);

		int Day() 
			{ return f_day; }
	protected:
 		virtual void Stroke3dFrame(BRect frame, rgb_color light, rgb_color dark, bool inset);
	private:
		int f_day;
};

class TCalendarView: public BView {
	public:
		TCalendarView(BRect frame, const char *name, uint32 resizingmode, uint32 flags);
		virtual ~TCalendarView();
		virtual void AttachedToWindow();
		virtual void Draw(BRect bounds);
		virtual void MessageReceived(BMessage *message);
		
		void SetTo(int32, int32, int32);
	protected:
		virtual void InitView();
		virtual void DispatchMessage();
	private:
		void InitDates();
		void CalcFlags();
		
		TDay *f_cday;
		BRect f_dayrect;
		int f_firstday;
		int f_month;
		int f_day;
		int f_year;
};

#endif // CALENDAR_VIEW_H
