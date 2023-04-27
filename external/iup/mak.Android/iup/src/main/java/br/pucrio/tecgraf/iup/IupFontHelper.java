package br.pucrio.tecgraf.iup;
import java.lang.Object;
import android.content.Context;
import android.graphics.Paint;
import android.graphics.Rect;
import android.graphics.Typeface;
import android.support.v7.view.ContextThemeWrapper;
import android.util.TypedValue;
import android.view.View;
import android.app.Activity;
import android.util.Log;
import android.widget.TextView;

import br.pucrio.tecgraf.iup.IupApplication;
import br.pucrio.tecgraf.iup.IupCommon;

public final class IupFontHelper
{
	// Metrics:
	// https://stackoverflow.com/questions/5611411/what-is-the-default-text-size-on-android
	public static int getStringWidth(final long ihandle_ptr, Object native_object, int object_type, String str)
	{

		// If there is already a text view, use
		// Paint the_paint = textView.getPaint();
		// 
	//	IupApplication application_context = IupApplication.getIupApplication();
		ContextThemeWrapper theme_context = IupCommon.getContextThemeWrapper();
		TextView text_view = new TextView(theme_context);
		Log.i("Text dimensions", "str: "+str);


		Paint the_paint = new Paint();
		//the_paint.setTypeface(Typeface.DEFAULT);
		Typeface default_typeface = text_view.getTypeface();
		the_paint.setTypeface(default_typeface);
		// Yuck: getTextSize returns in different units than what setTextSize uses. We must manually convert.
		float default_text_size = text_view.getTextSize();
		Log.i("Text dimensions", "default_text_size: "+default_text_size);

		// https://stackoverflow.com/questions/3687065/textview-settextsize-behaves-abnormally-how-to-set-text-size-of-textview-dynam
		// https://stackoverflow.com/questions/6263250/convert-pixels-to-sp
		float scaledDensity = theme_context.getResources().getDisplayMetrics().scaledDensity;
		Log.i("Text dimensions", "scaledDensity: "+scaledDensity);

		float adjusted_text_size = default_text_size/scaledDensity;
		Log.i("Text dimensions", "adjusted_text_size: "+adjusted_text_size);

		//the_paint.setTextSize(adjusted_text_size);
		//helloWorldTextView2.setTextSize(pixelsToSp(getActivity(), helloWorldTextView.getTextSize()));

		final float densityMultiplier = theme_context.getResources().getDisplayMetrics().density;
		Log.i("Text dimensions", "density: "+densityMultiplier);

		// I'm not sure which to use
		/*
		DisplayMetrics#scaledDensity

A scaling factor for fonts displayed on the display. This is the same as density, except that it may be adjusted in smaller increments at runtime based on a user preference for the font size.
DisplayMetrics#Density

The logical density of the display. This is a scaling factor for the Density Independent Pixel unit, where one DIP is one pixel on an approximately 160 dpi screen.

		 */
//		final float scaledPx = default_text_size * densityMultiplier;
		final float scaledPx = default_text_size * scaledDensity;
		Log.i("Text dimensions", "scaledPx: "+scaledPx);

		the_paint.setTextSize(scaledPx);

//
//		Typeface typeface = Typeface.createFromAsset(getAssets(), "Helvetica.ttf");
		//the_paint.setStyle(the_paint.Style.FILL);
		Rect rect_result = new Rect();
		the_paint.getTextBounds(str, 0, str.length(), rect_result);

Log.i("Text dimensions", "Width: "+rect_result.width());


		return rect_result.width();

	}

	// FIXME: This was originally getMultiLineStringSize, but quickly refactored to handle the GetTextSize api change. It is not clear to me if they should actually share the same routine.
	private static Rect helperFontGetMultilineStringSize(Typeface type_face, String str)
	{
		// If there is already a text view, use
		// Paint the_paint = textView.getPaint();
		//
		//IupApplication application_context = IupApplication.getIupApplication();
		ContextThemeWrapper theme_context = IupCommon.getContextThemeWrapper();
		TextView text_view = new TextView(theme_context);
		//text_view.setInputType(InputType.TYPE_TEXT_FLAG_MULTI_LINE);
		//text_view.setSingleLine(false);

		Paint the_paint = new Paint();

		// Hack: Used for getMultiLineStringSize. getTextSize will supply a Typeface
		if(null == type_face)
		{
			type_face = text_view.getTypeface();
		}

		the_paint.setTypeface(type_face);
		// Yuck: getTextSize returns in different units than what setTextSize uses. We must manually convert.
		float default_text_size = text_view.getTextSize();

		// https://stackoverflow.com/questions/3687065/textview-settextsize-behaves-abnormally-how-to-set-text-size-of-textview-dynam
		// https://stackoverflow.com/questions/6263250/convert-pixels-to-sp
		float scaled_density = theme_context.getResources().getDisplayMetrics().scaledDensity;




		// I'm not sure which to use
		/*
		DisplayMetrics#scaledDensity

A scaling factor for fonts displayed on the display. This is the same as density, except that it may be adjusted in smaller increments at runtime based on a user preference for the font size.
DisplayMetrics#Density

The logical density of the display. This is a scaling factor for the Density Independent Pixel unit, where one DIP is one pixel on an approximately 160 dpi screen.

		 */
		final float scaled_px = default_text_size * scaled_density;

		the_paint.setTextSize(scaled_px);

		Rect rect_result = new Rect();



		Paint.FontMetrics font_metric = the_paint.getFontMetrics();
//		float char_height = font_metric.descent - font_metric.ascent;
		int char_height = (int)(font_metric.bottom - font_metric.top + font_metric.leading + 0.5);

		int max_width = 0;
		int running_height = 0;
		String[] split_lines = str.split("\n");

		for(int i=0; i < split_lines.length; i++)
		{
			// TODO: Investigate this:
			// https://stackoverflow.com/questions/7549182/android-paint-measuretext-vs-gettextbounds
			the_paint.getTextBounds(split_lines[i], 0, split_lines[i].length(), rect_result);
			if(rect_result.width() > max_width)
			{
				max_width = rect_result.width();
			}




			// running_height = running_height + rect_result.height();
			running_height = running_height + char_height;
		}


		// y increases going down
		rect_result.set(0, 0, max_width, running_height);


		return rect_result;
	}

	public static Rect getMultiLineStringSize(final long ihandle_ptr, Object native_object, int object_type, String str)
	{
		return helperFontGetMultilineStringSize(null, str);
	}

	public static Rect getTextSize(String font_name, String str)
	{
		// FIXME: I don't know what the font string name syntax is and if comes embedded with point size and emphasis instructions.
		Typeface type_face = Typeface.create(font_name, Typeface.NORMAL);
		return helperFontGetMultilineStringSize(type_face, str);
	}

	// https://stackoverflow.com/questions/3654321/measuring-text-height-to-be-drawn-on-canvas-android
	public static Rect getCharSize(final long ihandle_ptr, Object native_object, int object_type)
	{

		// If there is already a text view, use
		// Paint the_paint = textView.getPaint();
		//
		//IupApplication application_context = IupApplication.getIupApplication();
		ContextThemeWrapper theme_context = IupCommon.getContextThemeWrapper();
		TextView text_view = new TextView(theme_context);


		Paint the_paint = new Paint();
		Typeface default_typeface = text_view.getTypeface();
		the_paint.setTypeface(default_typeface);
		// Yuck: getTextSize returns in different units than what setTextSize uses. We must manually convert.
		float default_text_size = text_view.getTextSize();

		// https://stackoverflow.com/questions/3687065/textview-settextsize-behaves-abnormally-how-to-set-text-size-of-textview-dynam
		// https://stackoverflow.com/questions/6263250/convert-pixels-to-sp
		float scaled_density = theme_context.getResources().getDisplayMetrics().scaledDensity;




		// I'm not sure which to use
		/*
		DisplayMetrics#scaledDensity

A scaling factor for fonts displayed on the display. This is the same as density, except that it may be adjusted in smaller increments at runtime based on a user preference for the font size.
DisplayMetrics#Density

The logical density of the display. This is a scaling factor for the Density Independent Pixel unit, where one DIP is one pixel on an approximately 160 dpi screen.

		 */
		final float scaled_px = default_text_size * scaled_density;

		the_paint.setTextSize(scaled_px);

		Rect rect_result = new Rect();
		String str = "WWWWWWWW"; // 8 characters, divide by 8 later
		the_paint.getTextBounds(str, 0, str.length(), rect_result);

//		Log.i("Text dimensions", "Width: "+rect_result.width());

		int char_width = (int)(rect_result.width()/8.0);

		Paint.FontMetrics font_metric = the_paint.getFontMetrics();
//		float char_height = font_metric.descent - font_metric.ascent;
		int char_height = (int)(font_metric.bottom - font_metric.top + font_metric.leading + 0.5);

		// y increases going down
		rect_result.set(0, 0, char_width, char_height);

		return rect_result;
	}
}


