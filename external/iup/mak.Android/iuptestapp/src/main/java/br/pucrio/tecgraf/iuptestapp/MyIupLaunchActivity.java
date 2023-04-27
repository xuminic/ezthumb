package br.pucrio.tecgraf.iuptestapp;



import android.app.Application;
import android.view.View;
import android.os.Bundle;
//import android.content.res.AssetManager;
import android.util.Log;

import br.pucrio.tecgraf.iup.IupLaunchActivity;


public class MyIupLaunchActivity extends IupLaunchActivity
{
	@Override
	protected String[] getLibraries()
	{
		return new String[]
				{
//						"MyIupProgram",
						"iuptestapp",
						"iupimglib",
						"iup",
				};
	}

	@Override
	public String getEntryPointLibraryName()
	{
//		return "libMyIupProgram.so";
		return "libiuptestapp.so";
	}
}
