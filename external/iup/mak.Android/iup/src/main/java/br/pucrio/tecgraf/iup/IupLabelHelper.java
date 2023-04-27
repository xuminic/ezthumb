package br.pucrio.tecgraf.iup;

import android.graphics.Bitmap;
import android.support.v7.view.ContextThemeWrapper;
import android.support.v7.widget.AppCompatEditText;
import android.support.v7.widget.AppCompatTextView;
import android.view.View;
import java.lang.Object;
import android.content.Context;
import android.view.View;
//import android.app.Activity;
import android.util.Log;
import android.widget.EditText;
import android.widget.ImageView;
import android.widget.TextView;

import br.pucrio.tecgraf.iup.IupApplication;
import br.pucrio.tecgraf.iup.IupCommon;

public final class IupLabelHelper
{
	// value must be final in order to access in inner class
	public static TextView createLabelText(final long ihandle_ptr)
	{
		//Context context = (Context)IupApplication.getIupApplication();
		// I discovered the hard way that passing the application context to the EditView or AppCompatEditView
		// does not apply the theme.
		// In my isolated cases, passing the Activity context instead of the Application context avoids the problem.
		// However, because Android defers the creation of the Activity to some time later,
		// and Iup's routines need to keep chugging along in one uninterrupted stream,
		// I don't have the Activity to pass at this point.
		// (I tried startActivity in AsyncTask, but the onCreate still gets deferred, even if I sleep.)
		// Fortunately, ContextThemeWrapper will let us grab the theme.
		// TODO: Is the hardcoded R.style.AppTheme going to be a problem for people who want to customize their themes?
		// Maybe we make this a string we can read?
		// The other more complicated idea I had was to keep a list with the temporary ViewGroup,
		// and whenever a widget gets added that needs a proper theme, we add to the list so when the we finally get the Activity,
		// we can go through the list and create/copy/replace/destroy the widgets with a newly created copy with the Activity as the context.
		// That will be painful, so I'm glad we can do this.
		//ContextThemeWrapper theme_context = new ContextThemeWrapper(context, R.style.AppTheme);
		ContextThemeWrapper theme_context = IupCommon.getContextThemeWrapper();
		TextView text_view = new AppCompatTextView(theme_context);

		/*
//		String attrib_string = IupCommon.iupAttribGet(ihandle_ptr, "TITLE");

//		android.util.Log.w("IupButtonHelper::createButton", "attrib_string: " + attrib_string);

		if(null != attrib_string)
		{
			new_button.setText(attrib_string);
		}
*/




		return text_view;
	}

	public static void setText(final long ihandle_ptr, TextView text_view, String the_text)
	{
		text_view.setText(the_text);
	}

	public static String getText(final long ihandle_ptr, TextView text_view)
	{
		return text_view.getText().toString();
	}


	public static ImageView createLabelImage(final long ihandle_ptr)
	{
//		Context context = (Context)IupApplication.getIupApplication();
		ContextThemeWrapper theme_context = IupCommon.getContextThemeWrapper();
		ImageView image_view = new ImageView(theme_context);
		image_view.setScaleType(ImageView.ScaleType.CENTER_INSIDE);
//		image_view.setScaleType(ImageView.ScaleType.FIT_CENTER);
		image_view.setAdjustViewBounds(true);
//		image_view.getLayoutParams().width = 40;
//		image_view.getLayoutParams().height = 40;
//		image_view.requestLayout();
		return image_view;
	}

	public static void setImage(final long ihandle_ptr, ImageView image_view, Bitmap bitmap_image)
	{
		image_view.setImageBitmap(bitmap_image);
		if(null == bitmap_image)
		{
			return;
		}
		// FIXME: I don't know what the IUP policy is about setting the ImageView size
		image_view.getLayoutParams().width = bitmap_image.getWidth();
		image_view.getLayoutParams().height = bitmap_image.getHeight();
		image_view.requestLayout();
	}
	
}

