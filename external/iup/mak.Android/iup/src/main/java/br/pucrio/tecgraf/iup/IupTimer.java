package br.pucrio.tecgraf.iup;

//import android.util.Log;
import android.os.Handler;
import android.os.SystemClock;


public final class IupTimer extends Handler
{
	private boolean isStarted = false;
	private long startTime = 0;
	private Runnable runnableCode = null;
	private long iHandlePtr = 0;
		
	public boolean isStarted()
	{
		return isStarted;
	}

	private void startElapsedTime()
	{
		// Don't use currentTimeMillis because it is not monotonic and may be changed by NTP, daylight savings, etc.
		// uptimeMillis() is monotonic. Stops counting when enters deep-sleep.
		// elapsedRealtime() is monotonic. Does not stop counting when enters deep-sleep.
		// Not sure which is best, but I'm picking uptimeMillis() thinking of a media/game timer for animation, 
		// where we wouldn't count deep-sleep interruptions since the app would 
		// be frozen and expected to resume where it left off (e.g. watching a movie).
		//startTime = System.currentTimeMillis();
		startTime = SystemClock.uptimeMillis();
	}

	private void stopElapsedTime()
	{
		// startTime = 0;
	}
	
	public long getElapsedTime()
	{
		long elapsed_time = SystemClock.uptimeMillis() - startTime;
		return elapsed_time;
	}

	public void startTimer(final long ihandle_ptr, final long interval_period)
	{
		if(isStarted)
		{
			return;
		}
		iHandlePtr = ihandle_ptr;

		Runnable runnable_code = new Runnable()
		{
			@Override
			public void run()
			{
				IupTimer.this.postDelayed(this, interval_period);
				IupCommon.iupAttribSetInt(ihandle_ptr, "ELAPSEDTIME", (int)getElapsedTime());
				int ret_val = IupCommon.handleIupCallback(ihandle_ptr, "ACTION_CB");
				
			}
		};



		runnableCode = runnable_code;
		startElapsedTime();
		postDelayed(runnable_code, interval_period);
		isStarted = true;
		
	}

	public void stopTimer()
	{
		removeCallbacks(runnableCode);
		stopElapsedTime();
		runnableCode = null;
		isStarted = false;
	}


}

