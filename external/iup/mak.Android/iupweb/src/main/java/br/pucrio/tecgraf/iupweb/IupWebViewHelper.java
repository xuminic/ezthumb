package br.pucrio.tecgraf.iupweb;
import java.lang.Object;
import java.lang.reflect.Method;

import android.content.ClipData;
import android.content.ClipboardManager;
import android.content.Context;
import android.print.PrintAttributes;
import android.print.PrintDocumentAdapter;
import android.print.PrintJob;
import android.view.View;
import android.app.Activity;
import android.util.Log;
import android.webkit.ValueCallback;
import android.webkit.WebBackForwardList;
import android.webkit.WebHistoryItem;
import android.webkit.WebView;
import android.webkit.WebViewClient;
//import android.widget.LinearLayout;

import android.print.PrintManager; // requires API 19


import br.pucrio.tecgraf.iup.IupApplication;
import br.pucrio.tecgraf.iup.IupCommon;


public final class IupWebViewHelper
{

	public static IupWebView createWebView(final long ihandle_ptr)
	{
		Context the_context = (Context)IupApplication.getIupApplication();
		IupWebView web_view = new IupWebView(the_context, ihandle_ptr);

	//	web_view.setLayoutParams(new LinearLayout.LayoutParams(LinearLayout.LayoutParams.FILL_PARENT, 600));

		return web_view;
 	};

	public static void loadUrl(IupWebView web_view, String url_string)
	{
		android.util.Log.v("IupWebViewHelper", "url_string " + url_string);

		web_view.loadUrl(url_string);
	};

	public static String getUrl(IupWebView web_view)
	{
		return web_view.getUrl();
	};

	public static String getItemHistoryAtIndex(IupWebView web_view, int index)
	{
		/* IUP: Negative values represent steps backward while positive values represent steps forward. */
		// But Android uses a list that goes from 0 to n-1.
		WebBackForwardList back_forward_list = web_view.copyBackForwardList();
		android.util.Log.v("IupWebViewHelper", "getItemHistoryAtIndex " + index);

		int list_size = back_forward_list.getSize();
		if(list_size <= 0)
		{
			return null;
		}

		int current_index = back_forward_list.getCurrentIndex();
		if(current_index < 0)
		{
			return null;
		}
		android.util.Log.v("IupWebViewHelper", "list_size " + list_size);
		android.util.Log.v("IupWebViewHelper", "current_index " + current_index);

		// Forward items: current index up to getSize-1
		// Back items: current index down to 0
		int mapped_index;
		if(index == 0)
		{
			mapped_index = current_index;
		}
		else if(index > 0)
		{
			mapped_index = current_index + index;
			if(mapped_index >= list_size)
			{
				return null;
			}
		}
		else
		{
			// index is negative in this case.
			mapped_index = current_index + index;
			if(mapped_index < 0)
			{
				return null;
			}
		}
		android.util.Log.v("IupWebViewHelper", "mapped_index " + mapped_index);

		WebHistoryItem history_item = back_forward_list.getItemAtIndex(mapped_index);
		if(null != history_item)
		{
			return history_item.getUrl();
		}
		return null;
	}

	public static int getForwardCount(IupWebView web_view)
	{
		/* IUP: Negative values represent steps backward while positive values represent steps forward. */
		// But Android uses a list that goes from 0 to n-1.
		WebBackForwardList back_forward_list = web_view.copyBackForwardList();

		int list_size = back_forward_list.getSize();
		if(list_size <= 0)
		{
			return 0;
		}

		int current_index = back_forward_list.getCurrentIndex();
		if(current_index < 0)
		{
			return 0;
		}

		// Forward items: current index up to getSize-1
		// Back items: current index down to 0

		int number_of_items = (list_size -1) - current_index;
		return number_of_items;
	}

	public static int getBackCount(IupWebView web_view)
	{
		/* IUP: Negative values represent steps backward while positive values represent steps forward. */
		// But Android uses a list that goes from 0 to n-1.
		WebBackForwardList back_forward_list = web_view.copyBackForwardList();

		int list_size = back_forward_list.getSize();
		if(list_size <= 0)
		{
			return 0;
		}

		int current_index = back_forward_list.getCurrentIndex();
		if(current_index < 0)
		{
			return 0;
		}

		// Forward items: current index up to getSize-1
		// Back items: current index down to 0

		int number_of_items = current_index - 0;
		return number_of_items;
	}

	public static void loadHtmlString(IupWebView web_view, String html_string)
	{
		web_view.loadData(html_string, "text/html; charset=utf-8", "UTF-8");
	}

	public static void copySelectionToClipboard(IupWebView web_view)
	{
		// Looks like Android only does text (or gives you URIs)
		// How do I get the selected text?
		// Seems like only way is through JavaScript. Not sure if this will break a webpage.
		// https://stackoverflow.com/questions/34804100/how-to-get-the-selected-text-of-webview-in-actionmode-override
		// Requires API 19
		if(android.os.Build.VERSION.SDK_INT >= android.os.Build.VERSION_CODES.KITKAT)
		{
			web_view.evaluateJavascript("(function(){return window.getSelection().toString()})()",
			new ValueCallback<String>()
			{
				@Override
				public void onReceiveValue(String selected_text)
				{
					Context application_context = IupApplication.getIupApplication();
					ClipboardManager clipboard = (ClipboardManager) application_context.getSystemService(Context.CLIPBOARD_SERVICE);
					ClipData clip = ClipData.newPlainText("webview", selected_text);
					clipboard.setPrimaryClip(clip);
				}
			});
		}
	}

	public static void print(IupWebView web_view)
	{
		// requires API 19 KitKat
		if(android.os.Build.VERSION.SDK_INT >= android.os.Build.VERSION_CODES.KITKAT)
		{
			Context application_context = IupApplication.getIupApplication();

			// Get a PrintManager instance
			PrintManager print_manager = (PrintManager) application_context.getSystemService(Context.PRINT_SERVICE);

			// Get a print adapter instance
			PrintDocumentAdapter print_adapter = web_view.createPrintDocumentAdapter();

			// Create a print job with name and adapter instance
			//String job_name = getString(R.string.app_name) + " Document";
			String job_name = web_view.getTitle();
			PrintJob print_job = print_manager.print(job_name, print_adapter, new PrintAttributes.Builder().build());
		}
	}

	public static int getLoadStatus(final long ihandle_ptr, IupWebView web_view)
	{
		return web_view.getCurrentLoadStatus();
 	};

	public static void reload(IupWebView web_view)
	{
		web_view.reload();
	};

	public static void stopLoading(IupWebView web_view)
	{
		web_view.stopLoading();
	};

	public static void goBackOrForward(IupWebView web_view, int num_steps)
	{
		web_view.goBackOrForward(num_steps);
	};

	public static boolean canGoBack(IupWebView web_view)
	{
		return web_view.canGoBack();
	};

	public static boolean canGoForward(IupWebView web_view)
	{
		return web_view.canGoBack();
	};

	public static void goBack(IupWebView web_view)
	{
		web_view.goBack();
	};

	public static void goForward(IupWebView web_view)
	{
		web_view.goForward();
	};

}


