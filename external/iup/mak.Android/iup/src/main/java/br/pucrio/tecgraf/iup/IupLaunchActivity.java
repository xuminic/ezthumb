package br.pucrio.tecgraf.iup;

import android.content.Context;
import android.app.Activity;

import android.app.AlertDialog;
import android.content.DialogInterface;

import android.support.v7.app.AppCompatActivity;
import android.view.View;
import android.os.Bundle;
//import android.content.res.AssetManager;
import android.util.Log;

import android.content.pm.PackageManager;
import android.content.pm.ApplicationInfo;

import android.view.ViewGroup;

import android.content.Intent;



public class IupLaunchActivity extends AppCompatActivity
{

    /**
     * This method is called before loading the native shared libraries.
	 * This is inspired from SDL and allows you to list library names
	 * without the explict lib prefix and .so suffix.
	 * It is intended to be overridden by a user subclass for their own 
	 * application so they may add other libraries without having to modify
	 * this file directly.
     * The default implementation returns the defaults. It never returns null.
     * An array returned by a new implementation must at least contain "iup".
     * Also keep in mind that the order the libraries are loaded may matter.
     * @return names of shared libraries to be loaded (e.g. "iup", "iupImage").
     */
    protected String[] getLibraries()
	{
		return new String[]
		{
			"iup",
		};
	}

	// When subclassing, do not call super.
	public String getEntryPointLibraryName()
	{
		return "libMyIupProgram.so";
	}

	// Load the .so
	public void loadLibraries()
	{
		for(String a_lib : getLibraries())
		{
			System.loadLibrary(a_lib);
		}
	}

	/*
	   static
	   {
	   System.loadLibrary("c++_shared");
	   System.loadLibrary("icudataswift");
	   System.loadLibrary("icuucswift");
		System.loadLibrary("icui18nswift");
		System.loadLibrary("swiftCore");
		System.loadLibrary("swiftSwiftOnoneSupport");
		System.loadLibrary("MySDLMainActivity");
	}
	*/

	/* A native method that is implemented by the
     * 'hello-jni' native library, which is packaged
     * with this application.
     */
//    public static native boolean doStaticActivityInit();
	public native void IupEntry(IupLaunchActivity this_activity, String entry_function_name, String entry_library_name);
	public native void doPause();
	public native void doResume();
	public native void doDestroy();

	private boolean loadLibraryFailed = false;

	/** Called when the activity is first created. */
    @Override
	public void onCreate(Bundle savedInstanceState)
	{
		super.onCreate(savedInstanceState);

		// Load shared libraries
		String error_msg = "";
		try
		{
			loadLibraries();
		}
		catch(UnsatisfiedLinkError e)
		{
			System.err.println(e.getMessage());
            loadLibraryFailed = true;
			error_msg = e.getMessage();
		}
		catch(Exception e)
		{
			System.err.println(e.getMessage());
            loadLibraryFailed = true;
			error_msg = e.getMessage();
		}

		if(loadLibraryFailed)
		{
			AlertDialog.Builder alert_dialog  = new AlertDialog.Builder(this);
			alert_dialog.setMessage("An error occurred while trying to start the application after calling System.LoadLibrary()."
				+ System.getProperty("line.separator")
				+ System.getProperty("line.separator")
				+ "Error: " + error_msg
			);
			alert_dialog.setTitle("IUP Error");
			alert_dialog.setPositiveButton("Exit",
				new DialogInterface.OnClickListener()
				{
					@Override
					public void onClick(DialogInterface dialog,int id)
					{
						// if this button is clicked, close current activity
						IupLaunchActivity.this.finish();
					}
				}
			);
			alert_dialog.setCancelable(false);
			alert_dialog.create().show();

			return;
		}

	}

	@Override
	protected void onStart()
	{
		Log.i("HelloAndroidIupLaunchActivity", "calling onStart");
		
		super.onStart();
		Log.i("HelloAndroidIupLaunchActivity", "calling doInit");
		//AssetManager java_asset_manager = this.getAssets();
		//doInit(java_asset_manager, this);

		String entry_function_name = getEntryPointFunctionNameFromManifest();
		Log.i("HelloAndroidIupLaunchActivity", "AndroidManifest entry_function_name: " + entry_function_name);

		String entry_library_name = getEntryPointLibraryNameFromManifest();
		Log.i("HelloAndroidIupLaunchActivity", "AndroidManifest entry_library_name: " + entry_library_name);
		if(null == entry_library_name)
		{
			entry_library_name = getEntryPointLibraryName();
		}

		IupEntry(this, entry_function_name, entry_library_name);
		Log.i("HelloAndroidIupLaunchActivity", "finished calling doInit");
	}

	// String is allowed to be null
	private String getEntryPointFunctionNameFromManifest()
	{
		String entry_function_name = null;
		try
		{
			ApplicationInfo app_info = getPackageManager().getApplicationInfo(this.getPackageName(), PackageManager.GET_META_DATA);
			Bundle bundle = app_info.metaData;
			entry_function_name = bundle.getString("ENTRY_POINT");
		}
		catch(PackageManager.NameNotFoundException e)
		{
			// Log.e(TAG, "Failed to load meta-data, NameNotFound: " + e.getMessage());
		}
		catch (NullPointerException e)
		{
			// Log.e(TAG, "Failed to load meta-data, NullPointer: " + e.getMessage());			
		}
		return entry_function_name;
	}

	// String is allowed to be null
	private String getEntryPointLibraryNameFromManifest()
	{
		String entry_library_name = null;
		try
		{
			ApplicationInfo app_info = getPackageManager().getApplicationInfo(this.getPackageName(), PackageManager.GET_META_DATA);
			Bundle bundle = app_info.metaData;
			entry_library_name = bundle.getString("ENTRY_LIBRARY");
		}
		catch(PackageManager.NameNotFoundException e)
		{
			// Log.e(TAG, "Failed to load meta-data, NameNotFound: " + e.getMessage());
		}
		catch (NullPointerException e)
		{
			// Log.e(TAG, "Failed to load meta-data, NullPointer: " + e.getMessage());
		}
		return entry_library_name;
	}
	/** Called when the activity is about to be paused. */
	@Override
	protected void onPause()
	{
		Log.i("HelloAndroidIupLaunchActivity", "calling onPause");

		//		doPause();
		super.onPause();
	}

	@Override
	protected void onResume()
	{
		Log.i("HelloAndroidIupLaunchActivity", "calling onResume");

		super.onResume();
		//		doResume();
	}

	/** Called when the activity is about to be destroyed. */
	@Override
	protected void onDestroy()
	{
		Log.i("HelloAndroidIupLaunchActivity", "calling onDestroy");
		//		doDestroy();

		super.onDestroy();
		Log.i("HelloAndroidIupLaunchActivity", "finished calling onDestroy");		
	}


}

