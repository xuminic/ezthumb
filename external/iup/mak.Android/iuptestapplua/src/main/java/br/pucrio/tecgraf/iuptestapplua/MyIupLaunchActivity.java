package br.pucrio.tecgraf.iuptestapplua;



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
						"iuptestapplua",
						"iupluaweb",
						"iuplua",
						"iupweb",
						"iup",
						"lua"
				};
	}
	
	@Override
	public String getEntryPointLibraryName()
	{
		return "libiuptestapplua.so";
	}
}
