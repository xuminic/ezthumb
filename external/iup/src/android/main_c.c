#include "iup.h"
#include "iup_varg.h"

#include "iup_config.h"
#include <stddef.h>
#include <stdio.h>


void MyPrintf(const char* fmt, ...)
{
	va_list ap;	
	va_start(ap, fmt);
	IupLogV("DEBUG", fmt, ap);
	va_end(ap);
}

int OnButtonCallback()
{
	MyPrintf("OnButtonCallback()\n");

	char temp_string[1024];
	static int button_count = 0;
	snprintf(temp_string, 1024, "Iup Button %d", button_count);
	Ihandle* button = IupButton(temp_string, "");

	IupSetCallback(button, "ACTION", (Icallback)OnButtonCallback);
	Ihandle* dialog = IupDialog(button);
	snprintf(temp_string, 1024, "Iup Activity Title %d", button_count);

	IupSetStrAttribute(dialog, "TITLE", temp_string);
	


	IupShow(dialog);

	button_count++;

	return IUP_DEFAULT;
}
Ihandle *timer1, *timer2;

int timer_cb(Ihandle *n)
{
	if(n == timer1)
	{
		static int counter = 0;
		IupLog("DEBUG", "timer 1 called\n");
		if(counter > 5)
		{
			IupLog("DEBUG", "killing timer 1\n");
			IupDestroy(timer1);
			timer1 = NULL;
		}
		counter++;

	}

	if(n == timer2)
	{
		IupLog("DEBUG", "timer 2 called\n");
		IupDestroy(timer2);
		timer2 = NULL;
		return IUP_CLOSE;

	}

	return IUP_DEFAULT;
}


void IupExitPoint()
{
	IupClose();
}

// For Android, this name is hardcoded
void IupEntryPoint()
{
/*
	{
		int ret_val;
		const char* config_value;
		Ihandle* config_file = IupConfig();
		IupSetStrAttribute(config_file, "APP_NAME", "TestApp");
		ret_val = IupConfigLoad(config_file);

		if(ret_val == 0)
		{
			const char* config_value = IupConfigGetVariableStrDef(config_file, "Group1", "Key1", "");
			MyPrintf("config value is %s\n", config_value);
		}
		else
		{
			MyPrintf("config file not found\n");
		}
		IupConfigSetVariableStr(config_file, "Group1", "Key1", "Value1");
		IupConfigSave(config_file);
		config_value = IupConfigGetVariableStrDef(config_file, "Group1", "Key1", "");
		MyPrintf("retrieved saved config value is %s\n", config_value);

		IupDestroy(config_file);
		config_file = NULL;
	}
*/
	{
		IupSetFunction("EXIT_CB", (Icallback)IupExitPoint);
	}

//	Ihandle* label = IupLabel("Iup Label");

#if 1
	Ihandle* text_view_result = IupMultiLine(NULL);
//	Ihandle* text_view_result = IupText(NULL);
	//  IupSetAttribute(text_view_result, "SIZE", "500x100");
	IupSetAttribute(text_view_result, "VISIBLECOLUMNS", "40");
	IupSetAttribute(text_view_result, "VISIBLELINES", "10");
	IupSetAttribute(text_view_result, "EXPAND", "HORIZONTAL");
	IupSetAttribute(text_view_result, "READONLY", "YES");
	IupSetAttribute(text_view_result, "VALUE", "Velcome tu zee vonderful wurld");

	#endif

	Ihandle* button = IupButton("Iup Button", "");
	IupSetCallback(button, "ACTION", (Icallback)OnButtonCallback);
	Ihandle* button2 = IupButton("Iup Button 2", "");
	Ihandle* button3 = IupButton("Iup Button 3", "");
	Ihandle* button4 = IupButton("Iup Button 4", "");

//	Ihandle* vb=IupVbox(text_view_result, button, NULL);
//	Ihandle* vb=IupVbox(button, text_view_result, NULL);
//	Ihandle* vb=IupVbox(button, button2, NULL);
	Ihandle* hb1=IupHbox(button, button2, NULL);
	Ihandle* hb2=IupHbox(button3, button4, NULL);
//	Ihandle* vb=IupVbox(hb1, hb2, NULL);
	Ihandle* vb=IupVbox(hb1, text_view_result, hb2, NULL);

//	IupSetAttribute(vb, "GAP", "10");
//	IupSetAttribute(vb, "MARGIN", "10x10");
//	IupSetAttribute(vb, "ALIGNMENT", "ACENTER");

	Ihandle* dialog = IupDialog(vb);
	//	IupMap(dialog);
	IupSetAttribute(dialog, "TITLE", "Iup Activity Title");
//	IupSetAttribute(dialog, "RASTERSIZE", "1024x1920");

	timer1 = IupTimer();
	timer2 = IupTimer();

	IupSetAttribute(timer1, "TIME", "4000");
	IupSetAttribute(timer1, "RUN", "YES");
	IupSetCallback(timer1, "ACTION_CB", (Icallback)timer_cb);

	IupSetAttribute(timer2, "TIME", "10000");
	IupSetAttribute(timer2, "RUN", "YES");
	IupSetCallback(timer2, "ACTION_CB", (Icallback)timer_cb);



	IupShow(dialog);
}


// IMPORTANT: This is never called on Android and does nothing. But a good cross-platform IUP app will always have this for the other platforms.
// You should not modify this template.
// Everything reconverges at IupEntryPoint().
int main(int argc, char* argv[])
{
	IupOpen(&argc, &argv);
	IupSetFunction("ENTRY_POINT", (Icallback)IupEntryPoint);
	IupMainLoop();
	return 0;
}


