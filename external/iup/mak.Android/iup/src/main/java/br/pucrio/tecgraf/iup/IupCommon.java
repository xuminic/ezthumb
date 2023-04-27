package br.pucrio.tecgraf.iup;
import java.lang.Object;
import android.app.Activity;
import android.content.Context;
import android.content.ContextWrapper;
import android.support.v7.view.ContextThemeWrapper;
import android.util.Log;
import android.view.View;
import android.view.ViewGroup;
import android.webkit.WebView;
import android.widget.RelativeLayout;

// Can we use this?
// Android 8.0 @FastNative & @CriticalNative
// https://source.android.com/devices/tech/dalvik/improvements#faster-native-methods
// import dalvik.annotation.optimization.FastNative;


public final class IupCommon
{

    public native static void RetainIhandle(Object the_widget, long ihandle_ptr);
    public static void retainIhandle(Object the_widget, long ihandle_ptr)
	{
		RetainIhandle(the_widget, ihandle_ptr);
    }


    public native static void ReleaseIhandle(long ihandle_ptr);
    public static void releaseIhandle(long ihandle_ptr)
	{
		ReleaseIhandle(ihandle_ptr);
    }


	public native static Object GetObjectFromIhandle(long ihandle_ptr);
	public static Object getObjectFromIhandle(long ihandle_ptr)
	{
		return GetObjectFromIhandle(ihandle_ptr);
	}

	// http://grepcode.com/file/repository.grepcode.com/java/ext/com.google.android/android/4.3_r1/android/support/v7/app/MediaRouteButton.java#MediaRouteButton.getActivity%28%29
	public static Activity getActivity(View the_view)
	{
		// Gross way of unwrapping the Activity 
		Context context = the_view.getContext();
		while(context instanceof ContextWrapper)
		{
			if(context instanceof Activity)
			{
			Log.i("Java IupCommon getActivity", "found Activity");
				
				return (Activity)context;
			}
			context = ((ContextWrapper)context).getBaseContext();
		}
			Log.i("Java IupCommon getActivity", "didn't find Activity");
		return null;
	}



public static void addWidgetToParent(Object parent_widget, Object child_widget)
{
View child_view = null;
// TODO: Support fragments?
		if(child_widget instanceof android.view.View)
		{
			child_view = (View)child_widget;
		}
		else
		{
			Log.e("Java IupCommon addWidgetToParent", "child_widget is not a supported type");
			return;
		}
		

		if((parent_widget instanceof android.app.Activity))
		{

			// Design assumption: We've added a RelativeLayout as the content view to the Activity.
			// We'll add the widget to that view.
			Activity parent_activity = (Activity)parent_widget;
			// TODO: Consider making a method in our IupActivity that returns the root view instead of fishing like this.
			ViewGroup parent_view = (ViewGroup)parent_activity.getWindow().getDecorView();


			ViewGroup.LayoutParams vg_layout_params = new ViewGroup.LayoutParams(
				ViewGroup.LayoutParams.WRAP_CONTENT,
				ViewGroup.LayoutParams.WRAP_CONTENT
			);
			RelativeLayout.LayoutParams layout_params = new RelativeLayout.LayoutParams(vg_layout_params);
			//Log.d("Java IupCommon addWidgetToParent", "need to remove Margin setting (Activity)");

			// TODO: Remove this. Setting these margins is how IUP will set the positions during layout.
//			layout_params.leftMargin = 5;
//			layout_params.topMargin = 5;

			
			parent_view.addView(child_view, layout_params);

		}
		else if((parent_widget instanceof android.view.ViewGroup))
		{

			ViewGroup parent_view = (ViewGroup)parent_widget;

			ViewGroup.LayoutParams vg_layout_params = new ViewGroup.LayoutParams(
				ViewGroup.LayoutParams.WRAP_CONTENT,
				ViewGroup.LayoutParams.WRAP_CONTENT
			);
			RelativeLayout.LayoutParams layout_params = new RelativeLayout.LayoutParams(vg_layout_params);

			//Log.d("Java IupCommon addWidgetToParent", "need to remove Margin setting (ViewGroup)");

			// TODO: Remove this. Setting these margins is how IUP will set the positions during layout.
//			layout_params.leftMargin = 20;
//			layout_params.topMargin = 20;
			Log.d("Java IupCommon addWidgetToParent", "child_view class name is: " + child_view.getClass().getName());


			parent_view.addView(child_view, layout_params);
			Log.d("Java IupCommon addWidgetToParent", "adding to viewgroup");


		}
		else
		{
			Log.e("Java IupCommon addWidgetToParent", "parent_widget is unsupported type");
		}


	}

	public static void removeWidgetFromParent(long ihandle_ptr)
	{
		View child_view = null;
		Object child_widget = getObjectFromIhandle(ihandle_ptr);

// TODO: Support fragments?
		if(child_widget instanceof android.view.View)
		{
			child_view = (View)child_widget;
		}
		else
		{
			Log.e("Java IupCommon addWidgetToParent", "child_widget is not a supported type");
			return;
		}

		ViewGroup parent_view_group = (ViewGroup)child_view.getParent();
		if(null != parent_view_group)
		{
			parent_view_group.removeView(child_view);
		}
	}


	public static void setWidgetPosition(Object the_widget, int x, int y, int width, int height)
	{

		if(the_widget instanceof android.view.View)
		{
			View the_view = (View)the_widget;
			RelativeLayout.LayoutParams layout_params = (RelativeLayout.LayoutParams)the_view.getLayoutParams();
			Log.d("Java IupCommon setWidgetPosition", "child_view class name is: " + the_view.getClass().getName());
			Log.d("Java IupCommon setWidgetPosition", "old leftMargin: " + layout_params.leftMargin);
			Log.d("Java IupCommon setWidgetPosition", "old topMargin: " + layout_params.topMargin);
			Log.d("Java IupCommon setWidgetPosition", "old width: " + layout_params.width);
			Log.d("Java IupCommon setWidgetPosition", "old height: " + layout_params.height);
			Log.d("Java IupCommon setWidgetPosition", "x: " + x + " y: " + y);
			Log.d("Java IupCommon setWidgetPosition", "w: " + width + " h: " + height);

			// TODO: Remove this. Setting these margins is how IUP will set the positions during layout.
			layout_params.leftMargin = x;
			layout_params.topMargin = y;
			layout_params.width = width;
			layout_params.height = height;

			// BUG: Seems like I must re-set the layout_params because when I didn't do this, ProgressBar did not correctly resize when going from Portrait to Landscape.
			// Other things seemed to work and Landscape to Portrait worked with ProgressBar.
			the_view.setLayoutParams(layout_params);


		}
		else
		{
			Log.e("Java IupCommon setWidgetPosition", "the_widget is unsupported type");
		}
/*
		{
			View the_view = (View)the_widget;
			RelativeLayout.LayoutParams layout_params = (RelativeLayout.LayoutParams)the_view.getLayoutParams();
			Log.d("Java IupCommon setWidgetPosition", "new leftMargin: " + layout_params.leftMargin);
			Log.d("Java IupCommon setWidgetPosition", "new topMargin: " + layout_params.topMargin);
			Log.d("Java IupCommon setWidgetPosition", "new width: " + layout_params.width);
			Log.d("Java IupCommon setWidgetPosition", "new height: " + layout_params.height);
		}
*/
	}

//	@FastNative
    public native static String nativeIupAttribGet(long ihandle_ptr, String key_string);
    public static String iupAttribGet(long ihandle_ptr, String key_string)
	{
		return nativeIupAttribGet(ihandle_ptr, key_string);
    }
//	@FastNative
	public native static void nativeIupAttribSet(long ihandle_ptr, String key_string, String value_string);
    public static void iupAttribSet(long ihandle_ptr, String key_string, String value_string)
	{
		nativeIupAttribSet(ihandle_ptr, key_string, value_string);
    }

//	@FastNative
	public native static int nativeIupAttribGetInt(long ihandle_ptr, String key_string);
    public static int iupAttribGetInt(long ihandle_ptr, String key_string)
	{
		return nativeIupAttribGetInt(ihandle_ptr, key_string);
    }
//	@FastNative
	public native static void nativeIupAttribSetInt(long ihandle_ptr, String key_string, int value_int);
    public static void iupAttribSetInt(long ihandle_ptr, String key_string, int value_int)
	{
		nativeIupAttribSetInt(ihandle_ptr, key_string, value_int);
    }


	/* IUP returns -1 through -4 for callbacks. I also return -15 if no callback is registered. */
	public native static int HandleIupCallback(long ihandle_ptr, String key_string);
	public static int handleIupCallback(long ihandle_ptr, String key_string)
	{
		return HandleIupCallback(ihandle_ptr, key_string);
	}

	public native static int DoResize(long ihandle_ptr, int x, int y, int width, int height);
	public static int doResize(long ihandle_ptr, int x, int y, int width, int height)
	{
		return DoResize(ihandle_ptr, x, y, width, height);
	}



	public static void setActive(Object the_widget, boolean is_active)
	{
		if(the_widget instanceof android.view.View)
		{
			View the_view = (View)the_widget;
			the_view.setEnabled(is_active);
		}
		else
		{
			Log.e("Java IupCommon setActive", "the_widget is unsupported/unimplemented type");
		}
	}
	public static boolean isActive(Object the_widget)
	{
		if(null == the_widget)
		{
			Log.e("Java IupCommon isActive", "the_widget is null");
			return true; // assume the widget is null because it is being initialized and thus active by default?
		}
		else if(the_widget instanceof android.view.View)
		{
			View the_view = (View)the_widget;
			return the_view.isEnabled();
		}
		else
		{
			Log.e("Java IupCommon isActive", "the_widget is unsupported/unimplemented type");
			return true;
		}
	}

	// TODO: Is the Android Spinner class what we need?
	// I discovered the hard way that passing the application context to the EditView or AppCompatEditView
	// does not apply the theme.
	// In my isolated cases, passing the Activity context instead of the Application context avoids the problem.
	// However, because Android defers the creation of the Activity to some time later,
	// and Iup's routines need to keep chugging along in one uninterrupted stream,
	// I don't have the Activity to pass at this point.
	// (I tried startActivity in AsyncTask, but the onCreate still gets deferred, even if I sleep.)
	// Fortunately, ContextThemeWrapper will let us grab the theme.
	// TODO: Is the hardcoded R.style.AppTheme going to be a problem for people who want to customize their themes?
	// (This requires that I bundle the styles.xml and colors.xml in the iup.aar.)
	// Maybe we make this a string we can read?
	// The other more complicated idea I had was to keep a list with the temporary ViewGroup,
	// and whenever a widget gets added that needs a proper theme, we add to the list so when the we finally get the Activity,
	// we can go through the list and create/copy/replace/destroy the widgets with a newly created copy with the Activity as the context.
	// That will be painful, so I'm glad we can do this.
	private static ContextThemeWrapper s_contextThemeWrapper = null;
	public static ContextThemeWrapper getContextThemeWrapper()
	{
		if(null != s_contextThemeWrapper)
		{
			return s_contextThemeWrapper;
		}
		Context context = (Context)IupApplication.getIupApplication();
		s_contextThemeWrapper = new ContextThemeWrapper(context, R.style.AppTheme);
		return s_contextThemeWrapper;
	}
}

