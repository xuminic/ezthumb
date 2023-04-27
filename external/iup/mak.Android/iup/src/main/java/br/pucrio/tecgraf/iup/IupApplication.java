package br.pucrio.tecgraf.iup;

import java.lang.ref.WeakReference;
import android.content.Context;
import android.app.Application;
import android.app.Activity;
import android.view.View;
import android.os.Bundle;
//import android.content.res.AssetManager;
import android.util.Log;


public class IupApplication extends Application
{

    private static IupApplication s_sharedInstance;
	public static IupApplication getIupApplication()
	{
        return s_sharedInstance;
    }

	private WeakReference<Activity> currentActivity;
	public Activity getCurrentActivity()
	{
		return currentActivity.get();
	}

	/* A native method that is implemented by the
     * 'hello-jni' native library, which is packaged
     * with this application.
     */
//    public native boolean doInit(IupApplication my_app);


    /** Called when the activity is first created. */
    @Override
    public void onCreate()
    {
		super.onCreate();
		s_sharedInstance = this;
        registerActivityLifecycleCallbacks(new IupActivityLifecycleHandler());
		

//		Log.i("HelloAndroidIupApplication", "calling doInit");
	//	doInit(this);
//		Log.i("HelloAndroidIupApplication", "finished calling doInit");
	}

	// Requires API 14 and replaces onLowMemory
	@Override
	public void onTrimMemory(int level)
	{
		// Not sure which 
		if(TRIM_MEMORY_COMPLETE == level)
		{
		}
		else if(TRIM_MEMORY_RUNNING_CRITICAL == level)
		{
		}
		else if(TRIM_MEMORY_RUNNING_LOW == level)
		{
		}
		else
		{
		}

	}
		
    public native void IupExitCallback();

	// http://stackoverflow.com/questions/3667022/checking-if-an-android-application-is-running-in-the-background/13809991#13809991
	private final class IupActivityLifecycleHandler implements ActivityLifecycleCallbacks
	{
		// I use four separate variables here. You can, of course, just use two and
		// increment/decrement them instead of using four and incrementing them all.
		private int resumedCount;
		private int pausedCount;
		private int startedCount;
		private int stoppedCount;
		private int activityCount;
		
		// Since there can be multiple Activities, 
		// this variable is used to save the last state so we only report 
		// actual transitions and suppress redundant checks.
		private boolean savedIsInBackground;

		@Override
		public void onActivityCreated(Activity activity, Bundle saved_instance_state)
		{
			activityCount += 1;
			android.util.Log.w("onActivityCreated", "count: " + activityCount);			
		}

		@Override
		public void onActivityDestroyed(Activity activity)
		{
			activityCount -= 1;
			android.util.Log.w("onActivityDestroyed", "count: " + activityCount);			
			// When the activity count goes to 0 here, I think we can infer that the application is quitting.
			// On my device, isApplicationInBackground() is true and the onActivityStopped transition already occurred.
			// Call quit callback here.
			if(0 == activityCount)
			{
				IupExitCallback();
			}
		}

		@Override
		public void onActivityResumed(Activity activity)
		{
			IupApplication.this.currentActivity = new WeakReference<Activity>(activity);
			
			resumedCount += 1;
			android.util.Log.w("onActivityResumed", "application is in foreground: " + (resumedCount > pausedCount));
			android.util.Log.w("onActivityResumed", "application is in background: " + isApplicationInBackground());
		}

		@Override
		public void onActivityPaused(Activity activity)
		{
			pausedCount += 1;
			android.util.Log.w("onActivityPaused", "application is in foreground: " + (resumedCount > pausedCount));
			android.util.Log.w("onActivityPaused", "application is in background: " + isApplicationInBackground());
			
		}

		@Override
		public void onActivitySaveInstanceState(Activity activity, Bundle outState)
		{
		}

		@Override
		public void onActivityStarted(Activity activity)
		{
			IupApplication.this.currentActivity = new WeakReference<Activity>(activity);


			startedCount += 1;
			android.util.Log.w("onActivityStarted", "application is visible: " + (startedCount > stoppedCount));
			android.util.Log.w("onActivityStarted", "application is in background: " + isApplicationInBackground());
			// NOTE: onActivityStarted seems to be the best place to check for isApplicationInBackground (becomes false)
			checkForBackgroundTransition();
		}

		@Override
		public void onActivityStopped(Activity activity)
		{
			stoppedCount += 1;
			android.util.Log.w("onActivityStopped", "application is visible: " + (startedCount > stoppedCount));
			android.util.Log.w("onActivityStopped", "application is in background: " + isApplicationInBackground());
			// NOTE: onActivityStopped seems to be the best place to check for isApplicationInBackground (becomes true)
			checkForBackgroundTransition();
		}

		public boolean isApplicationVisible()
		{
			return startedCount > stoppedCount;
		}

		public boolean isApplicationInForeground()
		{
			return resumedCount > pausedCount;
		}

		public boolean isApplicationInBackground()
		{
			return (!isApplicationVisible() && !isApplicationInForeground());
		}

		// Returns true if there was a transition.
		private boolean checkForBackgroundTransition()
		{
			boolean is_app_in_background = isApplicationInBackground();
			// We are trying to avoid multiple redundant callbacks since there can be multiple activities.
			if(is_app_in_background != savedIsInBackground)
			{
				savedIsInBackground = is_app_in_background;
				android.util.Log.w("checkForBackgroundTransition", "Application Background state has transitioned to: " + is_app_in_background);
				// TODO: Do background or resume callback here
				return true;
			}
			return false;
		}

	}

}

