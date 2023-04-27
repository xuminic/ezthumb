package br.pucrio.tecgraf.iup;

import android.support.v7.view.ContextThemeWrapper;
import android.support.v7.widget.AppCompatEditText;
import android.text.InputType;
import android.text.method.KeyListener;
import android.view.View;
import java.lang.Object;
import android.content.Context;
import android.view.View;
//import android.app.Activity;
import android.util.Log;
import android.widget.EditText;

import br.pucrio.tecgraf.iup.IupApplication;
import br.pucrio.tecgraf.iup.IupCommon;

public final class IupTextHelper
{
	// value must be final in order to access in inner class
	public static EditText createMultiLineText(final long ihandle_ptr)
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

//		EditText text_view = new AppCompatEditText(context);
		EditText text_view = new AppCompatEditText(theme_context);

//		EditText text_view = new EditText(context);

		/*
//		String attrib_string = IupCommon.iupAttribGet(ihandle_ptr, "TITLE");

//		android.util.Log.w("IupButtonHelper::createButton", "attrib_string: " + attrib_string);

		if(null != attrib_string)
		{
			new_button.setText(attrib_string);
		}
*/

		//text_view.setInputType(InputType.TYPE_CLASS_TEXT | InputType.TYPE_TEXT_VARIATION_NORMAL | InputType.TYPE_TEXT_FLAG_MULTI_LINE);
		savedKeyListener = text_view.getKeyListener();
		IupTextHelper.setReadOnlyMultiLine(ihandle_ptr, text_view, false);



		return text_view;
	}

	public static EditText createSingleLineText(final long ihandle_ptr)
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

//		EditText text_view = new AppCompatEditText(context);
		EditText text_view = new AppCompatEditText(theme_context);
		/*
//		String attrib_string = IupCommon.iupAttribGet(ihandle_ptr, "TITLE");

//		android.util.Log.w("IupButtonHelper::createButton", "attrib_string: " + attrib_string);

		if(null != attrib_string)
		{
			new_button.setText(attrib_string);
		}
*/
		text_view.setMaxLines(1);

		//text_view.setInputType(InputType.TYPE_CLASS_TEXT | InputType.TYPE_TEXT_VARIATION_NORMAL);
		savedKeyListener = text_view.getKeyListener();
		IupTextHelper.setReadOnlySingleLine(ihandle_ptr, text_view, false);



		return text_view;
	}

	// FIXME: This is just a single line TextView.
	public static EditText createSpinnerText(final long ihandle_ptr)
	{
		//Context context = (Context)IupApplication.getIupApplication();
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
		// Maybe we make this a string we can read?
		// The other more complicated idea I had was to keep a list with the temporary ViewGroup,
		// and whenever a widget gets added that needs a proper theme, we add to the list so when the we finally get the Activity,
		// we can go through the list and create/copy/replace/destroy the widgets with a newly created copy with the Activity as the context.
		// That will be painful, so I'm glad we can do this.
		//ContextThemeWrapper theme_context = new ContextThemeWrapper(context, R.style.AppTheme);
		ContextThemeWrapper theme_context = IupCommon.getContextThemeWrapper();

//		EditText text_view = new AppCompatEditText(context);
		EditText text_view = new AppCompatEditText(theme_context);
		/*
//		String attrib_string = IupCommon.iupAttribGet(ihandle_ptr, "TITLE");

//		android.util.Log.w("IupButtonHelper::createButton", "attrib_string: " + attrib_string);

		if(null != attrib_string)
		{
			new_button.setText(attrib_string);
		}
*/
		text_view.setMaxLines(1);
		savedKeyListener = text_view.getKeyListener();
		text_view.setInputType(InputType.TYPE_CLASS_TEXT | InputType.TYPE_TEXT_VARIATION_NORMAL);




		return text_view;
	}
	public static void setText(final long ihandle_ptr, EditText text_view, String the_text)
	{
		text_view.setText(the_text);
	}

	public static String getText(final long ihandle_ptr, EditText text_view)
	{
		return text_view.getText().toString();
	}

	public static void setCueBanner(final long ihandle_ptr, EditText text_view, String the_text)
	{
		text_view.setHint(the_text);
	}


	// Read-only may not be supportable. There is not a built-in.
	// Hacks sound unreliable.
	// Complaint: that getting the input type doesn't return the real value. (Unverifed)
	// https://stackoverflow.com/questions/27948356/restoring-keylistener-of-edittext-in-android-after-setting-it-to-null-loses-inpu
	// Best solution sounds like to swap TextView for EditText. YUCK.
	// TODO: If we go with the hack, we need to make our own subclass of EditText so we can save old values properly.
	static private KeyListener savedKeyListener = null;
	public static void setReadOnlyMultiLine(final long ihandle_ptr, EditText text_view, boolean is_read_only)
	{
		if(is_read_only)
		{
			text_view.setInputType(InputType.TYPE_NULL);
			text_view.setTextIsSelectable(true);
			savedKeyListener = text_view.getKeyListener();
			text_view.setKeyListener(null);
		}
		else
		{
			text_view.setInputType(InputType.TYPE_CLASS_TEXT | InputType.TYPE_TEXT_VARIATION_NORMAL | InputType.TYPE_TEXT_FLAG_MULTI_LINE);
			text_view.setTextIsSelectable(true);
			text_view.setKeyListener(savedKeyListener);
		}
	}

	public static boolean getReadOnlyMultiLine(final long ihandle_ptr, EditText text_view)
	{
		if(null == text_view.getKeyListener())
		{
			return true;
		}
		else
		{
			return false;
		}
	}

	public static void setReadOnlySingleLine(final long ihandle_ptr, EditText text_view, boolean is_read_only)
	{
		if(is_read_only)
		{
			text_view.setInputType(InputType.TYPE_NULL);
			text_view.setTextIsSelectable(true);
			savedKeyListener = text_view.getKeyListener();
			text_view.setKeyListener(null);
		}
		else
		{
			text_view.setInputType(InputType.TYPE_CLASS_TEXT | InputType.TYPE_TEXT_VARIATION_NORMAL);
			text_view.setTextIsSelectable(true);
			text_view.setKeyListener(savedKeyListener);
		}
	}

	public static boolean getReadOnlySingleLine(final long ihandle_ptr, EditText text_view)
	{
		if(null == text_view.getKeyListener())
		{
			return true;
		}
		else
		{
			return false;
		}
	}

}

