package br.pucrio.tecgraf.iup;
import java.lang.Object;
import android.content.Context;
import android.support.v7.view.ContextThemeWrapper;
import android.support.v7.widget.AppCompatButton;
import android.view.View;
import android.app.Activity;
import android.util.Log;
import android.widget.Button;
 
import br.pucrio.tecgraf.iup.IupApplication;
import br.pucrio.tecgraf.iup.IupCommon;

public final class IupButtonHelper
{
	// value must be final in order to access in inner class
	public static Button createButton(final long ihandle_ptr)
	{
		//Context context = (Context)IupApplication.getIupApplication();
		ContextThemeWrapper theme_context = IupCommon.getContextThemeWrapper();
		Button new_button = new AppCompatButton(theme_context);

		String attrib_string = IupCommon.iupAttribGet(ihandle_ptr, "TITLE");

		android.util.Log.w("IupButtonHelper::createButton", "attrib_string: " + attrib_string);

		if(null != attrib_string)
		{
			new_button.setText(attrib_string);
		}


		new_button.setOnClickListener(new View.OnClickListener()
		{
			public void onClick(View v)
			{
				IupCommon.handleIupCallback(ihandle_ptr, "ACTION");
			}
		});



		return new_button;
	}
	
}


