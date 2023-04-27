package br.pucrio.tecgraf.iup;

import java.lang.Object;
import android.util.Log;
//import android.app.Activity;
import android.content.Context;
import android.os.Looper;
import android.os.Handler;


public final class IupPostMessage
{
	public native static void OnMainThreadCallback(long ihandle_ptr, long message_data_ptr, String usr_str, long usr_int, double usr_double);

	// Needs the Activity for runOnUiThread
	// Might be able to use Looper.getMainLooper which only requires a Context
	public static void postMessage(Context app_context, final long ih, final long message_data_ptr, final String usr_str, final long usr_int, final double usr_double)
	{
		Runnable callback_runnable = new Runnable()
			{
				@Override
				public void run()
				{
					// Call back into C to trigger Finished callback
					OnMainThreadCallback(ih, message_data_ptr, usr_str, usr_int, usr_double);
				}
			}
		;

//		main_activity.runOnUiThread(callback_runnable);
		Handler ui_handler = new Handler(Looper.getMainLooper());
		ui_handler.post(callback_runnable);
	}

}


