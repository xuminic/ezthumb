package br.pucrio.tecgraf.iup;

import android.content.Context;
import android.app.Activity;
import android.content.pm.ActivityInfo;
import android.content.res.Configuration;
//import android.graphics.Color;
import android.os.Build;
//import android.support.v4.view.OnApplyWindowInsetsListener;
//import android.support.v4.view.ViewCompat;
//import android.support.v4.view.WindowInsetsCompat;
import android.support.v7.app.AppCompatActivity;
import android.view.View;
import android.os.Bundle;
//import android.content.res.AssetManager;
import android.util.Log;

import android.view.ViewGroup;
import android.view.ViewTreeObserver;
import android.view.Window;
import android.view.WindowInsets;
import android.view.WindowManager;
import android.widget.RelativeLayout;

import android.content.Intent;

import br.pucrio.tecgraf.iup.IupCommon;


// <sigh>: Android's default Activity transitions look terrible and nonsensical.
// Using res/anim/*.xml files, and overridePendingTransition, we fix can this
// (e.g. slide left and right which makes more sense with a left-facing back button).
// But it only works with the R.java resource system.
// But because we are (correctly) treating Iup as a separate library/package from the main app,
// the R.java namespace is wrong and we must the final app's package namespace,
// which is something we can't know.
// The new AAR libraries for Android might solve this, but everybody is going to have to spend
// time getting their build systems to properly build Iup and then import Iup into their projects.
// For now, comment out the overridingPendingTransitions until we have the infrastructure to support it.
//import net.playcontrol.MyBlurrrIupProject.R;


public class IupActivity extends AppCompatActivity
{
	private Configuration previousConfig;

	/* A native method that is implemented by the
	 * 'hello-jni' native library, which is packaged
	 * with this application.
	 */
	//    public static native boolean doStaticActivityInit();
	public native void doPause();
	public native void doResume();
	public native void doDestroy();

	public static void setWindowFlag(Activity activity, final int bits, boolean on) {
		Window win = activity.getWindow();
		WindowManager.LayoutParams winParams = win.getAttributes();
		if (on) {
			winParams.flags |= bits;
		} else {
			winParams.flags &= ~bits;
		}
		win.setAttributes(winParams);
	}
	/** Called when the activity is first created. */
	@Override
	public void onCreate(Bundle savedInstanceState)
	{

		super.onCreate(savedInstanceState);
		//		setContentView(R.layout.main);

		/* This will pass the HelloAndroidBlurrr activity class
		 * which Blurrr will capture to initialize the Blurrr_RWops system behind the scenes.
		 */
		//        HelloAndroidBlurrr.doStaticActivityInit();


		/* Once upon a time, we needed to pass the AssetManager to Blurrr.
		 * But that has been changed to conform to SDL's new behavior which
		 * requires the Activity class instead.
		 * The asset manager fetched here is no longer used (though Init still initializes Blurrr),
		 * but the code is left here as an example because this pattern is generally useful in Android.
		 */
		Log.i("HelloAndroidIupActivity", "calling doInit");
		Intent the_intent = getIntent();

		long ihandle_ptr = the_intent.getLongExtra("Ihandle", 0);

		// We need to swap the pointers around.
		Object view_group_object = IupCommon.getObjectFromIhandle(ihandle_ptr);
		if(view_group_object instanceof ViewGroup)
		{
			Log.i("HelloAndroidIupActivity", "swapping view group and activity");
			ViewGroup view_group = (ViewGroup)view_group_object;
			setContentView(view_group);
			IupCommon.releaseIhandle(ihandle_ptr);

			IupCommon.retainIhandle(this, ihandle_ptr);

			final ViewGroup final_view_group = view_group;


/*
			// I can't get this to work/trigger.
			if (Build.VERSION.SDK_INT >= 21)
			{

				view_group.setOnApplyWindowInsetsListener(new OnApplyWindowInsetsListener()
				{
					@Override
					public WindowInsets onApplyWindowInsets(View v, WindowInsets insets)
					{
						final int statusBarSize = insets.getSystemWindowInsetTop();
						Log.i("HelloAndroidIupActivity", "onApplyWindowInsets: " + statusBarSize);
						return insets;
					}
				});
			}
			*/
/*
			// https://learnpainless.com/android/material/make-fully-android-transparent-status-bar
			//make translucent statusBar on kitkat devices
			if (Build.VERSION.SDK_INT >= 19 && Build.VERSION.SDK_INT < 21) {
				setWindowFlag(this, WindowManager.LayoutParams.FLAG_TRANSLUCENT_STATUS, true);
			}
			if (Build.VERSION.SDK_INT >= 19) {
				getWindow().getDecorView().setSystemUiVisibility(View.SYSTEM_UI_FLAG_LAYOUT_STABLE | View.SYSTEM_UI_FLAG_LAYOUT_FULLSCREEN);
			}
			//make fully Android Transparent Status bar
			if (Build.VERSION.SDK_INT >= 21) {
				setWindowFlag(this, WindowManager.LayoutParams.FLAG_TRANSLUCENT_STATUS, false);
				getWindow().setStatusBarColor(Color.TRANSPARENT);
			}
*/



			// Problem: Our initial layout doesn't work because the view's width and height report 0 at this stage.
			// We need to force a Iup re-layout after the view gets a legitimate value.
			// We can use a ViewTreeObserver to get a callback to trigger this at the right time.

			// Problem: The observer doesn't always immediately give us the final rotated callback and sometimes calls back before the rotation is finished.
			// So we need to check to make sure the rotation is finished.


			ViewTreeObserver view_observer = view_group.getViewTreeObserver();
			final int from_width = view_group.getWidth();
			final int from_height = view_group.getHeight();
			final long final_ihandle_ptr = ihandle_ptr;
			view_observer.addOnGlobalLayoutListener(new ViewTreeObserver.OnGlobalLayoutListener()
			{

				@Override
				public void onGlobalLayout()
				{
					int to_width = final_view_group.getWidth();
					int to_height = final_view_group.getHeight();
					// Problem: The observer doesn't always immediately give us the final rotated callback and sometimes calls back before the rotation is finished.
					// So we need to check to make sure the rotation is finished.
					if(to_width != from_width && to_height != from_height)
					{
						Log.i("HelloAndroidIupActivity", "onCreate <w,h>: " + to_width + ", " + to_height);
						IupCommon.doResize(final_ihandle_ptr, 0, 0, to_width, to_height);
						if(Build.VERSION.SDK_INT < Build.VERSION_CODES.JELLY_BEAN)
						{
							final_view_group.getViewTreeObserver().removeGlobalOnLayoutListener(this);
						}else
						{
							final_view_group.getViewTreeObserver().removeOnGlobalLayoutListener(this);
						}
					}
				}
			});


		}
		else
		{
			Log.e("IupActivity", "Expected to have a ViewGroup in onCreate() but did not get one.");

			// Not sure what to do, but we can try to retain this and pretend things are fine until we trigger an exception.
			IupCommon.retainIhandle(this, ihandle_ptr);
		}








		// Deal with IUP properties
		String attrib_string = IupCommon.iupAttribGet(ihandle_ptr, "TITLE");
		if(null != attrib_string)
		{
			this.setTitle(attrib_string);
		}



		//AssetManager java_asset_manager = this.getAssets();
		//doInit(java_asset_manager, this);
		//IupEntry(this);
		Log.i("HelloAndroidIupActivity", "finished calling doInit");
	}


	/** Called when the activity is about to be paused. */
	@Override
	protected void onPause()
	{
		Log.i("HelloAndroidIupActivity", "calling onPause");
		
//		doPause();
		super.onPause();
	}

	@Override
	protected void onResume()
	{
		Log.i("HelloAndroidIupActivity", "calling onResume");
		
		super.onResume();
		previousConfig = new Configuration(getResources().getConfiguration());

//		doResume();
	}

	/** Called when the activity is about to be destroyed. */
	// I think the CLOSE_CB needs to go here.
	// We need to handle the case where the OS just kills an Activity for low RAM reasons.
	// This is akin to a user closing a window on a whim, but it is not possible to reject.
	@Override
	protected void onDestroy()
	{
		Log.i("HelloAndroidIupActivity", "calling onDestroy");
//		doDestroy();


		Intent the_intent = getIntent();
 		long ihandle_ptr = the_intent.getLongExtra("Ihandle", 0);

 		// In case the user directly invoked unMap (by manually destroying their dialog),
		// then we already cleaned up in unMap.
 		if(ihandle_ptr != 0)
		{
			// In case Android decides to kill the Activity, the developer needs to know it got destroyed.
			// At the very least, the developer should know to clean up their Dialog ih pointer.
			// This is analogous to a user closing a window on the desktop, except that the developer cannot refuse the action.
			IupCommon.handleIupCallback(ihandle_ptr, "CLOSE_CB");

			IupCommon.releaseIhandle(ihandle_ptr);
		}

		super.onDestroy();

		Log.i("HelloAndroidIupActivity", "finished calling onDestroy");		
	}

	@Override
	public void onConfigurationChanged(Configuration new_config)
	{
		super.onConfigurationChanged(new_config);
		int diff_config = new_config.diff(previousConfig);

		// Make sure this is a orientation/size change
		if(((diff_config & ActivityInfo.CONFIG_ORIENTATION) != 0)
			|| ((diff_config & ActivityInfo.CONFIG_SCREEN_SIZE) != 0)
		)
		{


			Intent the_intent = getIntent();
			final long ihandle_ptr = the_intent.getLongExtra("Ihandle", 0);

			Object parent_widget = IupCommon.getObjectFromIhandle(ihandle_ptr);
			if(parent_widget instanceof android.app.Activity)
			{
				// Design assumption: We've added a RelativeLayout as the content view to the Activity.
				// We'll add the widget to that view.
				Activity parent_activity = (Activity) parent_widget;
				// TODO: Consider making a method in our IupActivity that returns the root view instead of fishing like this.
				final ViewGroup view_group = (ViewGroup) parent_activity.getWindow().getDecorView();

				// Problem: If we ask for the size now, it is still the pre-rotated size.
				// Some Android OS's and devices have different view width/heights (e.g. thicker title bars) between portrait and landscape
				// so we can't just swap the values. (I remember one of the first Android 3.0 tablets introduced this problem.)
				// So it seems that the best way to do this is to use an observer to wait for the rotation to finish
				// and then do the callback & re-layout.


				ViewTreeObserver view_observer = view_group.getViewTreeObserver();
				final int from_width = view_group.getWidth();
				final int from_height = view_group.getHeight();

				view_observer.addOnGlobalLayoutListener(new ViewTreeObserver.OnGlobalLayoutListener()
				{

					@Override
					public void onGlobalLayout()
					{
						int to_width = view_group.getWidth();
						int to_height = view_group.getHeight();
						// Problem: The observer doesn't always immediately give us the final rotated callback and sometimes calls back before the rotation is finished.
						// So we need to check to make sure the rotation is finished.
						if(to_width != from_width && to_height != from_height)
						{
							Log.i("HelloAndroidIupActivity", "onConfigurationChanged <w,h>: " + to_width + ", " + to_height);
							IupCommon.doResize(ihandle_ptr, 0, 0, to_width, to_height);
							if(Build.VERSION.SDK_INT < Build.VERSION_CODES.JELLY_BEAN)
							{
								view_group.getViewTreeObserver().removeGlobalOnLayoutListener(this);
							}else
							{
								view_group.getViewTreeObserver().removeOnGlobalLayoutListener(this);
							}
						}
					}
				});



			}
		}

		previousConfig = new Configuration(new_config);

	}

/*
	@Override
	protected void onStart()
	{
		Log.i("HelloAndroidIupActivity", "calling onStart");
		
		super.onStart();
				Log.i("HelloAndroidIupActivity", "calling doInit");

		AssetManager java_asset_manager = this.getAssets();
		doInit(java_asset_manager);
		Log.i("HelloAndroidIupActivity", "finished calling doInit");
	}

	@Override
	protected void onStop()
	{
		Log.i("HelloAndroidIupActivity", "calling onStop");
		doDestroy();
		
		super.onStop();
		Log.i("HelloAndroidIupActivity", "finished calling onStop");
		
	}
*/


	native protected void OnActivityResult(int request_code, int result_code, Intent intent_data);
	// Things like in-app-purchases need to hook into this callback so we must expose it for IupAndroid users.
	@Override
	protected void onActivityResult(int request_code, int result_code, Intent intent_data)
	{
		super.onActivityResult(request_code, result_code, intent_data);
		// Pass on the activity result to the helper for handling
		this.OnActivityResult(request_code, result_code, intent_data);
	}

	@Override
	public void finish()
	{
		super.finish();
		// <sigh>: Android's default Activity transitions look terrible and nonsensical.
		// Using res/anim/*.xml files, and overridePendingTransition, we can fix this
		// (e.g. slide left and right which makes more sense with a left-facing back button).
		// But it only works with the R.java resource system.
		// But because we are (correctly) treating Iup as a separate library/package from the main app,
		// the R.java namespace is wrong and we must the final app's package namespace,
		// which is something we can't know.
		// The new AAR libraries for Android might solve this, but everybody is going to have to spend
		// time getting their build systems to properly build Iup and then import Iup into their projects.
		// For now, comment out the overridingPendingTransitions until we have the infrastructure to support it.
		overridePendingTransition(R.anim.iup_slide_from_left, R.anim.iup_slide_to_right);
	}

    public void myClickHandler(View the_view)
	{
		switch(the_view.getId())
		{
			default:
			{
				break;
			}
		}
	} 

/*	
	public void HelloAndroidBlurrr_MyJavaPlaybackFinishedCallbackTriggeredFromNDK(int which_channel, int channel_source, boolean finished_naturally) 
	{
		Log.i("HelloAndroidIupActivity", "HelloAndroidBlurrr_MyJavaPlaybackFinishedCallbackTriggeredFromNDK: channel:" + which_channel + ", source:" + channel_source + ", finishedNaturally:" + finished_naturally);
	}
*/


	/* 
	 * These Static methods are intended for C calling back into Java to do things.
	 */

	public static ViewGroup createActivity(final Activity parent_activity, long ihandle_ptr)
	{
		Log.i("HelloAndroidIupActivity", "createActivity");		
		final Intent the_intent = new Intent(parent_activity, IupActivity.class);
		the_intent.putExtra("Ihandle", ihandle_ptr);
        parent_activity.startActivity(the_intent);



		// <sigh>: Android's default Activity transitions look terrible and nonsensical.
		// Using res/anim/*.xml files, and overridePendingTransition, we can fix this
		// (e.g. slide left and right which makes more sense with a left-facing back button).
		// But it only works with the R.java resource system.
		// But because we are (correctly) treating Iup as a separate library/package from the main app,
		// the R.java namespace is wrong and we must the final app's package namespace,
		// which is something we can't know.
		// The new AAR libraries for Android might solve this, but everybody is going to have to spend
		// time getting their build systems to properly build Iup and then import Iup into their projects.
		// For now, comment out the overridingPendingTransitions until we have the infrastructure to support it.
		parent_activity.overridePendingTransition(R.anim.iup_slide_from_right, R.anim.iup_slide_to_left);

		RelativeLayout root_view = new RelativeLayout(parent_activity);
		return root_view;
		
	}

	public static void unMapActivity(Object activity_or_viewgroup, long ihandle_ptr)
	{
		// Because of the ViewGroup dance/trick I do, I need to check the type
		if(activity_or_viewgroup instanceof Activity)
		{
			// Do I need to call finish() for the case where IupDestroy() is called
			// but the Activity has not been popped from the navigation stack?
			Activity the_activity = (Activity)activity_or_viewgroup;
			the_activity.finish();

			// We are going to unref the pointer from C and clear the ih->handle field.
			// We are not going to wait for onDestroy() because in onDestroy(), it is ambiguous about how we got there and what is cleaned up.
			// (See comments where we clear the Ihandle in the Intent.)
			IupCommon.releaseIhandle(ihandle_ptr);

			// It is unclear when onDestroy() gets invoked.
			// Additionally, when in onDestroy() it is hard to know whether Android killed the Activity, or if the developer called this function to manually destroy the Activity.
			// So we should clear the Ihandle pointer value here to NULL, so onDestroy doesn't try to use a dangling pointer if we came from here (rather than Android just killing the Activity).
			// Note: This will prevent the CLOSE_CB from being invoked, but I think that is okay for this case.
			// Note: I am uncertain if this should be set after or before calling finish().
			Intent the_intent = the_activity.getIntent();
//			the_intent.putExtra("Ihandle", 0);
			the_intent.removeExtra("Ihandle");



			Log.i("HelloAndroidIupActivity", "calling finish() in unMapActivity");
			
		}

	}
}


