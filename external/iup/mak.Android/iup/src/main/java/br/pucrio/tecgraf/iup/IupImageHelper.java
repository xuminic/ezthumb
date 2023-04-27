package br.pucrio.tecgraf.iup;

import android.content.res.AssetManager;
import android.graphics.BitmapFactory;
import android.view.View;

import java.io.File;
import java.io.IOException;
import java.io.InputStream;
import java.lang.Object;
import android.content.Context;
import android.view.View;
//import android.app.Activity;
import android.util.Log;
import android.graphics.Bitmap;

import br.pucrio.tecgraf.iup.IupApplication;
import br.pucrio.tecgraf.iup.IupCommon;

public final class IupImageHelper
{
	public static Bitmap createBitmap(int width, int height, int bits_per_pixel)
	{
		//Context context = (Context)IupApplication.getIupApplication();

		Bitmap bitmap = Bitmap.createBitmap(width, height, Bitmap.Config.ARGB_8888);
		return bitmap;
	}

	public static Bitmap loadBitmap(String file_name)
	{
		if(null == file_name)
		{
			return null;
		}

		Bitmap bitmap = null;
		boolean file_exists = false;
		// We are going to try looking in the file system.
		// If not there, we will try looking in assets inside the .apk.
		try
		{
			File file_wrapper = new File(file_name);
			file_exists = file_wrapper.exists();

		}
		catch(NullPointerException ex)
		{
			return null;
		}


		if(file_exists)
		{
			bitmap = BitmapFactory.decodeFile(file_name);
			return bitmap;
		}
		else // try in assets in the apk
		{
			InputStream input_stream = null;
			try
			{
				Context context = (Context) IupApplication.getIupApplication();
				AssetManager asset_manager = context.getAssets();

				input_stream = asset_manager.open(file_name);
				bitmap = BitmapFactory.decodeStream(input_stream);
			}
			catch(IOException ex)
			{
			}
			finally
			{
				if(input_stream != null)
				{
					try
					{
						input_stream.close();
					}
					catch(IOException ex)
					{
					}
				}
			}

			return bitmap;
		}
	}
}

