package br.pucrio.tecgraf.iup;
import java.lang.Object;
import android.content.Context;
import android.view.View;
import android.app.Activity;
import android.util.Log;
import android.os.Handler;
 
import br.pucrio.tecgraf.iup.IupApplication;
import br.pucrio.tecgraf.iup.IupCommon;
import br.pucrio.tecgraf.iup.IupTimer;

public final class IupTimerHelper
{
	// value must be final in order to access in inner class
	public static IupTimer createTimer(final long ihandle_ptr)
	{
		IupTimer timer_handler = new IupTimer();
		return timer_handler;
	}

	public static void startTimer(final long ihandle_ptr, IupTimer timer_handler, final long interval_period)
	{
		timer_handler.startTimer(ihandle_ptr, interval_period);
		

	}
	
	public static void stopTimer(final long ihandle_ptr, IupTimer timer_handler)
	{
		timer_handler.stopTimer();
	}

}


