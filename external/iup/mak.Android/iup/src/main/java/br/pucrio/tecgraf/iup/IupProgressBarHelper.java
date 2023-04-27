package br.pucrio.tecgraf.iup;
import java.lang.Object;
import android.content.Context;
import android.support.v7.view.ContextThemeWrapper;
import android.view.View;
import android.util.Log;
import android.widget.ProgressBar;
 
import br.pucrio.tecgraf.iup.IupApplication;
import br.pucrio.tecgraf.iup.IupCommon;

public final class IupProgressBarHelper
{
	// value must be final in order to access in inner class
	public static ProgressBar createProgressBar(final long ihandle_ptr, int width, int height, boolean is_vertical)
	{
		//Context context = (Context)IupApplication.getIupApplication();
		ContextThemeWrapper theme_context = IupCommon.getContextThemeWrapper();
		ProgressBar progress_bar = null;
		// FIXME: Android doesn't support vertical progress bars.
		// https://stackoverflow.com/a/9311020
		if(is_vertical)
		{
			progress_bar = new IupProgressBarVertical(theme_context, null, android.R.attr.progressBarStyleHorizontal);
		}
		else
		{
			// Default style is circular indeterminate. Need to change into bar.
			progress_bar = new ProgressBar(theme_context, null, android.R.attr.progressBarStyleHorizontal);
		}

		return progress_bar;
	}

	public static void setProgressBarValues(final long ihandle_ptr, ProgressBar progress_bar, double min, double max, double value)
	{
		// WARNING: Android only supports int.
		// WARNING: Android before API 26 only supports ranges from 0 to 100.
		// We need to normalize.
		//
		// new_value = (NewMax - NewMin) / (OldMax - OldMin) * (value - OldMin) + NewMin
		double new_value = (100.0 - 0.0) / (max - min) * (value - min) + 0.0;
		int int_value = (int)(new_value + 0.5);
		// progress_bar.setMin(min);
		// progress_bar.setMax(max);
		progress_bar.setProgress(int_value);
	}

	public static void setIndeterminate(final long ihandle_ptr, ProgressBar progress_bar, boolean enable_marquee)
	{
		progress_bar.setIndeterminate(enable_marquee);
	}
}


