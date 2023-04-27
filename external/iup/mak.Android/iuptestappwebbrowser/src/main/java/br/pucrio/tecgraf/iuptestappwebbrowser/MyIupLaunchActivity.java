package br.pucrio.tecgraf.iuptestappwebbrowser;



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
						"iuptestappwebbrowser",
						"iupweb",
						"iup",
				};
	}

	@Override
	public String getEntryPointLibraryName()
	{
		return "libiuptestappwebbrowser.so";
	}
}
