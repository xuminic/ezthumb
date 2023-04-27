/** \file
 * \brief Web Browser Control
 *
 * See Copyright Notice in "iup.h"
 */

/*
 * WKWebView is Apple's new webview implementation as of Mac OS X 10.10 & iOS 8.
 * It replaces the prior WebView and UIWebView classes.
 * It supposedly is more secure. It also enables JIT.
 * Our legacy WebView and UIWebView implementations will eventually be removed.
 */
#import <Foundation/Foundation.h>
#import <WebKit/WKWebView.h>
#import <WebKit/WKUIDelegate.h>
#import <WebKit/WKNavigationAction.h>
#import <WebKit/WKNavigationDelegate.h>
#import <WebKit/WKUserContentController.h>
#import <WebKit/WKUserScript.h>
#import <WebKit/WKWebViewConfiguration.h>


#import <objc/runtime.h>

#include "iup.h"
#include "iupcbs.h"

#include "iup_object.h"

#include "iup_str.h"
#include "iup_webbrowser.h"
#include "iup_drv.h"
#include "iup_drvfont.h"


// the point of this is we have a unique memory address for an identifier
static const void* IUP_APPLEWKWEBVIEW_WEBVIEW_DELEGATE_OBJ_KEY = "IUP_APPLEWKWEBVIEW_WEBVIEW_DELEGATE_OBJ_KEY";


#include <TargetConditionals.h>
// https://stackoverflow.com/questions/146986/what-defines-are-set-up-by-xcode-when-compiling-for-iphone
// iOS, watchOS, tvOS (device & simulator)
#if (TARGET_OS_IPHONE == 1)
	#import <UIKit/UIKit.h>
	#include "iupcocoatouch_drv.h"
	#define iupAppleWKAddToParent iupCocoaTouchAddToParent
	#define iupAppleWKRemoveFromParent iupCocoaTouchRemoveFromParent

#else // Mac
	#import <AppKit/AppKit.h>
	#include "iupcocoa_drv.h"
	#define iupAppleWKAddToParent iupCocoaAddToParent
	#define iupAppleWKRemoveFromParent iupCocoaRemoveFromParent
#endif



typedef NS_ENUM(NSInteger, IupAppleWKWebViewLoadStatus)
{
	IupAppleWKWebViewLoadStatusFinished,
	IupAppleWKWebViewLoadStatusFailed,
	IupAppleWKWebViewLoadStatusLoading
};

@interface IupAppleWKWebViewDelegate : NSObject <WKNavigationDelegate, WKUIDelegate>

@property(nonatomic, assign) Ihandle* ihandle;
@property(nonatomic, assign) IupAppleWKWebViewLoadStatus currentLoadStatus;
@end

@implementation IupAppleWKWebViewDelegate

// NOTE: I don't see a clear place to implement the NEWWINDOW_CB. Desktop Cocoa WebView has createWebViewWithRequest, but there doesn't seem to be a WKWebView counterpart.
// WKNavigationDelegate
- (void) webView:(WKWebView*)the_sender decidePolicyForNavigationAction:(WKNavigationAction*)navigation_action decisionHandler:(void (^)(WKNavigationActionPolicy))decision_handler
{
	
	Ihandle* ih = [self ihandle];
	
	IFns cb = (IFns)IupGetCallback(ih, "NAVIGATE_CB");
	if (cb)
	{
		/* NOTE: Below are the types of events that may trigger this call back.
			We may need to filter accordingly.
			WKNavigationTypeLinkActivated,
			WKNavigationTypeFormSubmitted,
			WKNavigationTypeBackForward,
			WKNavigationTypeReload,
			WKNavigationTypeFormResubmitted,
			WKNavigationTypeOther = -1,
		*/
		WKNavigationType navigation_type = [navigation_action navigationType];
		if(WKNavigationTypeLinkActivated == navigation_type)
		{
		
			NSURL* ns_url = [[navigation_action request] URL];
			const char* c_url = [[ns_url absoluteString] UTF8String];
			
			if(cb(ih, (char*)c_url) == IUP_IGNORE)
			{
				decision_handler(WKNavigationActionPolicyCancel);
			}
			else
			{
				decision_handler(WKNavigationActionPolicyAllow);
			}
		}
		else
		{
			decision_handler(WKNavigationActionPolicyAllow);
		}
	}
	else
	{
		decision_handler(WKNavigationActionPolicyAllow);
	}
	

}

// WKUIDelegate
- (nullable WKWebView *)webView:(WKWebView*)web_view createWebViewWithConfiguration:(WKWebViewConfiguration*)webview_configuration forNavigationAction:(WKNavigationAction*)navigation_action windowFeatures:(WKWindowFeatures*)window_features
{
	// FIXME: I think the GTK version is returning the original webview and not creating a new window.
	// I'm doing the same, but I don't know if that is right.
	// Maybe the intention is that the user must explicitly create a new window themselves?
	Ihandle* ih = [self ihandle];

	IFns cb = (IFns)IupGetCallback(ih, "NEWWINDOW_CB");
	if (cb)
	{
		/* NOTE: Below are the types of events that may trigger this call back.
			We may need to filter accordingly.
			WKNavigationTypeLinkActivated,
			WKNavigationTypeFormSubmitted,
			WKNavigationTypeBackForward,
			WKNavigationTypeReload,
			WKNavigationTypeFormResubmitted,
			WKNavigationTypeOther = -1,
		*/
		// WKNavigationType navigation_type = [navigation_action navigationType];
		
		NSURL* ns_url = [[navigation_action request] URL];
		const char* c_url = [[ns_url absoluteString] UTF8String];
			
		cb(ih, (char*)c_url);
	}
	
	return web_view;
	
}

// WKNavigationDelegate
- (void) webView:(WKWebView*)web_view didFailNavigation:(WKNavigation*)navigation withError:(NSError*)the_error
{
	Ihandle* ih = [self ihandle];

	[self setCurrentLoadStatus:IupAppleWKWebViewLoadStatusFailed];
	
	IFns cb = (IFns)IupGetCallback(ih, "ERROR_CB");
	if (cb)
	{
		// Might be useful
		// [[the_error localizedDescription] UTF8String]
		// [the_error code]
	
//		NSLog(@"webView:didFailLoadWithError:forFrame: %@", the_error);

		// In testing, I get error 999 a lot.
		// "It actually means that another request is made before the previous request is completed."
		// https://stackoverflow.com/questions/30024244/webview-didfailloadwitherror-error-code-999
		// Maybe we should suppresss.
		// TODO: Introduce API to pass error code and error string.
		
		const char* failed_url = [[[the_error userInfo] valueForKey:NSURLErrorFailingURLStringErrorKey] UTF8String];
		cb(ih, (char*)failed_url);
	}

}

- (void) webView:(WKWebView*)web_view didFinishNavigation:(WKNavigation*)navigation
{
	Ihandle* ih = [self ihandle];

	[self setCurrentLoadStatus:IupAppleWKWebViewLoadStatusFinished];

	IFns cb = (IFns)IupGetCallback(ih, "COMPLETED_CB");
	if (cb)
	{
//		const char* c_url = [[[[[web_frame provisionalDataSource] request] URL] absoluteString] UTF8String];
		const char* c_url = [[[web_view URL] absoluteString] UTF8String];
		cb(ih, (char*)c_url);
	}
	
}

- (void) webView:(WKWebView*)web_view didStartProvisionalNavigation:(WKNavigation*)navigation
{
	[self setCurrentLoadStatus:IupAppleWKWebViewLoadStatusLoading];
}

@end





/*********************************************************************************************/

/*
#if 0
static char* cocoaTouchWebBrowserGetItemHistoryAttrib(Ihandle* ih, int index)
{
	// Negative values represent steps backward while positive values represent steps forward.
	WKWebView* web_view = (WKWebView*)ih->handle;

    return NULL;
}

static char* cocoaTouchWebBrowserGetForwardCountAttrib(Ihandle* ih)
{
	WKWebView* web_view = (WKWebView*)ih->handle;

	return iupStrReturnInt(0);
}

static char* cocoaTouchWebBrowserGetBackCountAttrib(Ihandle* ih)
{
	WKWebView* web_view = (WKWebView*)ih->handle;

	return iupStrReturnInt(0);
}
#endif
*/
static int cocoaTouchWebBrowserSetHTMLAttrib(Ihandle* ih, const char* value)
{
	if(value)
	{
		WKWebView* web_view = (WKWebView*)ih->handle;
		NSString* html_string = [NSString stringWithUTF8String:value];
		[web_view loadHTMLString:html_string baseURL:nil];
	}
	return 0; /* do not store value in hash table */
}


static int cocoaTouchWebBrowserSetCopyAttrib(Ihandle* ih, const char* value)
{
	(void)value;

	WKWebView* web_view = (WKWebView*)ih->handle;
	// use the responder chain. WebView provides a custom implementation. Does this work with WKWebView?
	//https://forums.developer.apple.com/thread/6480
	[web_view copy:nil];

/*
	DOMRange* dom_range = [web_view selectedDOMRange];
	NSPasteboard* paste_board = [NSPasteboard generalPasteboard];

	[paste_board setData:[[dom_range webArchive] data] forType:WebArchivePboardType];
*/
	
	return 0;
}

static int cocoaTouchWebBrowserSetSelectAllAttrib(Ihandle* ih, const char* value)
{
	(void)value;
	WKWebView* web_view = (WKWebView*)ih->handle;
	// use the responder chain. WebView provides a custom implementation. Does this work with WKWebView?
	[web_view selectAll:nil];
	return 0;
}

static int cocoaTouchWebBrowserSetPrintAttrib(Ihandle* ih, const char* value)
{
	(void)value;
	
	// iOS, watchOS, tvOS (device & simulator)
#if (TARGET_OS_IPHONE == 1)
	WKWebView* web_view = (WKWebView*)ih->handle;
	if([UIPrintInteractionController isPrintingAvailable])
	{
		UIPrintInteractionController* print_controller = [UIPrintInteractionController sharedPrintController];
		UIPrintInfo* print_info = [UIPrintInfo printInfo];
		[print_info setOutputType:UIPrintInfoOutputGeneral];
		[print_info setJobName:[[web_view URL] absoluteString]];
	
		[print_controller setPrintInfo:print_info];
		// Internet suggests YES causes some problems with scaling to fit. Say no?
		[print_controller setShowsPageRange:NO];
		[print_controller setPrintFormatter:[web_view viewPrintFormatter]];
		[print_controller presentAnimated:YES completionHandler:
			^(UIPrintInteractionController* print_contoller_completion, BOOL is_completed, NSError* the_error)
			{
				if(nil != the_error)
				{
					NSLog(@"Print error: %@", the_error);
				}
			}
		];
	}

#else // Mac
	// Ugh. This looks completely broken still as of 10.14.
	// The only workarounds are to either take a screenshot and print the image, or fallback to a old WebView.
	// https://forums.developer.apple.com/thread/78354
	WKWebView* web_view = (WKWebView*)ih->handle;
	// Use documentView to print whole document instead of visible
#if 1
	NSPrintOperation* print_operation = [NSPrintOperation printOperationWithView:web_view];
	NSWindow* parent_window = [web_view window];
	[print_operation runOperationModalForWindow:parent_window delegate:nil didRunSelector:nil contextInfo:nil];
#else
	// This didn't work either
//	[web_view evaluateJavaScript:@"window.print();" completionHandler:nil];

#endif

#endif

	
	
	return 0;
}

// magnification APIs only on Mac
#if !TARGET_OS_IPHONE
static int cocoaTouchWebBrowserSetZoomAttrib(Ihandle* ih, const char* value)
{
  int zoom;
  if (iupStrToInt(value, &zoom))
  {
  	CGFloat magnification = (CGFloat)zoom/100.0;
	WKWebView* web_view = (WKWebView*)ih->handle;
	[web_view setMagnification:magnification];
  }
  return 0;
}

static char* cocoaTouchWebBrowserGetZoomAttrib(Ihandle* ih)
{
	WKWebView* web_view = (WKWebView*)ih->handle;
	CGFloat zoom_factor = ([web_view magnification] * 100.0);
	return iupStrReturnInt(zoom_factor+0.5);
}
#endif

/*
 Besides implementing the webView:didStartProvisionalLoadForFrame: method to display the page title, you can also use it to display the status, for example, “Loading.” Remember that at this point the content has only been requested, not loaded; therefore, the data source is provisional.
 
 Similarly, implement the webView:didFinishLoadForFrame:, webView:didFailProvisionalLoadWithError:forFrame: and webView:didFailLoadWithError:forFrame: delegate methods to receive notification when a page has been loaded successfully or unsuccessfully. You might want to display a message if an error occurred.
*/
static char* cocoaTouchWebBrowserGetStatusAttrib(Ihandle* ih)
{
	WKWebView* web_view = (WKWebView*)ih->handle;

	// The state must be tracked through delegate callbacks. We save the current state directly in the delegate for convenience.
	IupAppleWKWebViewDelegate* webview_delegate = (IupAppleWKWebViewDelegate*)objc_getAssociatedObject(web_view, IUP_APPLEWKWEBVIEW_WEBVIEW_DELEGATE_OBJ_KEY);
	
	IupAppleWKWebViewLoadStatus current_status = [webview_delegate currentLoadStatus];
	
	switch(current_status)
	{
		case IupAppleWKWebViewLoadStatusFailed:
		{
			return "FAILED";
			break;
		}
		case IupAppleWKWebViewLoadStatusLoading:
		{
			return "LOADING";
			break;
		}
		case IupAppleWKWebViewLoadStatusFinished:
		{
			return "COMPLETED";
			break;
		}
		default:
		{
			NSLog(@"Unexpected case in AppleWKWebBrowserGetStatusAttrib");
			return "FAILED";
		}
	}
	return "FAILED";
}

static int cocoaTouchWebBrowserSetReloadAttrib(Ihandle* ih, const char* value)
{
	WKWebView* web_view = (WKWebView*)ih->handle;
	[web_view reload];
	return 0; /* do not store value in hash table */
}

static int cocoaTouchWebBrowserSetStopAttrib(Ihandle* ih, const char* value)
{
	WKWebView* web_view = (WKWebView*)ih->handle;
	[web_view stopLoading];
	return 0; /* do not store value in hash table */
}

// iOS doesn't have the WebBackForwardList as on Mac.
// This is the brute force way of doing it, but I'm not convinced this is reliable.
// The API doesn't specifiy what happens if you try to keep going back while loading.
// From a user-perspective, I've seen sites with aggressive redirects fight the back button and events get eaten.
// This is implemented for completeness/convenience, but officially I don't want to say this is supported.
// Please use the new CANGOBACK/FORWARD, and GOBACK/FORWARD APIs instead.
static int cocoaTouchWebBrowserSetBackForwardAttrib(Ihandle* ih, const char* value)
{
	int val;
	if(iupStrToInt(value, &val))
	{
		WKWebView* web_view = (WKWebView*)ih->handle;
		
		/* Negative values represent steps backward while positive values represent steps forward. */
		if(val > 0)
		{
			for(int i = 0; i < val; i++)
			{
				[web_view goForward];
			}
		}
		else if(val < 0)
		{
			for(int i = 0; i < -(val); i++)
			{
				[web_view goBack];
			}
		}
	}
		
	return 0; // do not store value in hash table
}


static int cocoaTouchWebBrowserSetValueAttrib(Ihandle* ih, const char* value)
{
	if(value)
	{
		WKWebView* web_view = (WKWebView*)ih->handle;
		NSString* ns_string = [NSString stringWithUTF8String:value];
		NSURL* ns_url = [NSURL URLWithString:ns_string];
		NSURLRequest* url_request = [NSURLRequest requestWithURL:ns_url];
		[web_view loadRequest:url_request];
	}
	return 0; /* do not store value in hash table */
}

static char* cocoaTouchWebBrowserGetValueAttrib(Ihandle* ih)
{
	WKWebView* web_view = (WKWebView*)ih->handle;
	const char* c_url = [[[web_view URL] absoluteString] UTF8String];
	return iupStrReturnStr(c_url);
}


static char* cocoaTouchWebBrowserCanGoBackAttrib(Ihandle* ih)
{
	WKWebView* web_view = (WKWebView*)ih->handle;
	BOOL is_true = [web_view canGoBack];
	return iupStrReturnBoolean((int)is_true);
}

static char* cocoaTouchWebBrowserCanGoForwardAttrib(Ihandle* ih)
{
	WKWebView* web_view = (WKWebView*)ih->handle;
	BOOL is_true = [web_view canGoForward];
	return iupStrReturnBoolean((int)is_true);
}

static int cocoaTouchWebBrowserSetBackAttrib(Ihandle* ih, const char* value)
{
	(void)value;
	
	WKWebView* web_view = (WKWebView*)ih->handle;
	[web_view goBack];
	
	return 0; /* do not store value in hash table */
}

static int cocoaTouchWebBrowserSetForwardAttrib(Ihandle* ih, const char* value)
{
	(void)value;
	
	WKWebView* web_view = (WKWebView*)ih->handle;
	[web_view goForward];
	
	return 0; /* do not store value in hash table */
}



/*********************************************************************************************/


static int cocoaTouchWebBrowserMapMethod(Ihandle* ih)
{
//	char* value;

//	WKWebView* web_view = [[WKWebView alloc] initWithFrame:CGRectZero];
//	WKWebView* web_view = [[WKWebView alloc] initWithFrame:CGRectMake(0,0,480, 640)];
	// https://stackoverflow.com/questions/26295277/wkwebview-equivalent-for-uiwebviews-scalespagetofit
//	[web_view setScalesPageToFit:YES];
	
	// This will be for iOS (mobile) only. (Not sure about AppleTV.)
	// This doesn't seem to work for me. But since my sizing code is still broken, I'm not sure if it is because of that or this. So come back to this later.
	// Alternatively, does UIScrollView control the zoom now?
//#if (TARGET_OS_IPHONE == 1)
#if 0
	NSString* java_script = @"var meta = document.createElement('meta'); meta.setAttribute('name', 'viewport'); meta.setAttribute('content', 'width=device-width'); document.getElementsByTagName('head')[0].appendChild(meta);";

	WKUserScript* user_script = [[WKUserScript alloc] initWithSource:java_script injectionTime:WKUserScriptInjectionTimeAtDocumentEnd forMainFrameOnly:NO];
	[user_script autorelease];
	WKUserContentController* user_content_controller = [[WKUserContentController alloc] init];
	[user_content_controller autorelease];
	[user_content_controller addUserScript:user_script];

	WKWebViewConfiguration* web_config = [[WKWebViewConfiguration alloc] init];
	[web_config autorelease];
	[web_config setUserContentController:user_content_controller];
	
	WKWebView* web_view = [[WKWebView alloc] initWithFrame:CGRectZero configuration:web_config];
#else
	WKWebViewConfiguration* web_config = [[WKWebViewConfiguration alloc] init];
	[web_config autorelease];
	WKWebView* web_view = [[WKWebView alloc] initWithFrame:CGRectZero configuration:web_config];

#endif
	
	ih->handle = web_view;
	
	
	
	
	IupAppleWKWebViewDelegate* webview_delegate = [[IupAppleWKWebViewDelegate alloc] init];
	[webview_delegate setIhandle:ih];
	// I'm using objc_setAssociatedObject/objc_getAssociatedObject because it allows me to avoid making subclasses just to hold ivars.
	// We're going to use OBJC_ASSOCIATION_RETAIN because I do believe it will do the right thing for us.
	objc_setAssociatedObject(web_view, IUP_APPLEWKWEBVIEW_WEBVIEW_DELEGATE_OBJ_KEY, (id)webview_delegate, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
	[webview_delegate release];

	
	[web_view setNavigationDelegate:webview_delegate];
	[web_view setUIDelegate:webview_delegate];

	
	
	// All AppleWK views shoud call this to add the new view to the parent view.
	iupAppleWKAddToParent(ih);

	
	return IUP_NOERROR;
}

static void cocoaTouchWebBrowserUnMapMethod(Ihandle* ih)
{
	WKWebView* web_view = (WKWebView*)ih->handle;

/*
	id web_view = objc_getAssociatedObject(the_button, IUP_APPLEWKWEBVIEW_WEBVIEW_DELEGATE_OBJ_KEY);
	objc_setAssociatedObject(the_button, IUP_APPLEWKWEBVIEW_WEBVIEW_DELEGATE_OBJ_KEY, nil, OBJC_ASSOCIATION_ASSIGN);
	[web_view release];
*/
	iupAppleWKRemoveFromParent(ih);

	[web_view release];
	ih->handle = NULL;
	
}






static void cocoaTouchWebBrowserComputeNaturalSizeMethod(Ihandle* ih, int *w, int *h, int *children_expand)
{
  int natural_w = 0, natural_h = 0;
  (void)children_expand; /* unset if not a container */

  /* natural size is 1 character */
  iupdrvFontGetCharSize(ih, &natural_w, &natural_h);

  *w = natural_w;
  *h = natural_h;
	
	/*
	*w = 480;
	*h = 640;
*/
	// FIXME: Iup is not expanding and making the visible height 1 row. Screen size routines never seem to be hit.
	*w = 480;
	*h = 480;
}

static int cocoaTouchWebBrowserCreateMethod(Ihandle* ih, void **params)
{
	ih->expand = IUP_EXPAND_BOTH;

#if 0
  (void)params;

  ih->data = iupALLOCCTRLDATA();

  /* default EXPAND is YES */
  ih->expand = IUP_EXPAND_BOTH;
  ih->data->sb = IUP_SB_HORIZ | IUP_SB_VERT;  /* default is YES */

#endif

  return IUP_NOERROR;
}

Iclass* iupWebBrowserNewClass(void)
{
  Iclass* ic = iupClassNew(NULL);

  ic->name = "webbrowser";
  ic->format = NULL; /* no parameters */
  ic->nativetype  = IUP_TYPECONTROL;
  ic->childtype   = IUP_CHILDNONE;
  ic->is_interactive = 1;
  ic->has_attrib_id = 1;   /* has attributes with IDs that must be parsed */

  /* Class functions */
  ic->New = iupWebBrowserNewClass;
  ic->Create = cocoaTouchWebBrowserCreateMethod;
  ic->Map = cocoaTouchWebBrowserMapMethod;
  ic->UnMap = cocoaTouchWebBrowserUnMapMethod;
  ic->ComputeNaturalSize = cocoaTouchWebBrowserComputeNaturalSizeMethod;
  ic->LayoutUpdate = iupdrvBaseLayoutUpdateMethod;

  /* Callbacks */
  iupClassRegisterCallback(ic, "NEWWINDOW_CB", "s");
  iupClassRegisterCallback(ic, "NAVIGATE_CB", "s");
  iupClassRegisterCallback(ic, "ERROR_CB", "s");

  /* Common */
  iupBaseRegisterCommonAttrib(ic);

  /* Visual */
  iupBaseRegisterVisualAttrib(ic);

  /* Overwrite Visual */
  iupClassRegisterAttribute(ic, "BGCOLOR", NULL, iupdrvBaseSetBgColorAttrib, IUPAF_SAMEASSYSTEM, "DLGBGCOLOR", IUPAF_DEFAULT); 

  // TODO: WKWebView doesn't have arbitrary back/forward history list APIs, but does support 1 step goBack/goForward and canGoBack/Forward query APIs. Consider adding to official API "GOBACK", "GOFORWARD", "CANGOBACK", "CANGOFORWARD"

  /* IupWebBrowser only */
  iupClassRegisterAttribute(ic, "VALUE", cocoaTouchWebBrowserGetValueAttrib, cocoaTouchWebBrowserSetValueAttrib, NULL, NULL, IUPAF_NO_DEFAULTVALUE|IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "BACKFORWARD", NULL, cocoaTouchWebBrowserSetBackForwardAttrib, NULL, NULL, IUPAF_NO_DEFAULTVALUE|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "STOP", NULL, cocoaTouchWebBrowserSetStopAttrib, NULL, NULL, IUPAF_WRITEONLY|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "RELOAD", NULL, cocoaTouchWebBrowserSetReloadAttrib, NULL, NULL, IUPAF_WRITEONLY|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "HTML", NULL, cocoaTouchWebBrowserSetHTMLAttrib, NULL, NULL, IUPAF_WRITEONLY|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "STATUS", cocoaTouchWebBrowserGetStatusAttrib, NULL, NULL, NULL, IUPAF_NO_DEFAULTVALUE|IUPAF_READONLY|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "COPY", NULL, cocoaTouchWebBrowserSetCopyAttrib, NULL, NULL, IUPAF_WRITEONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "SELECTALL", NULL, cocoaTouchWebBrowserSetSelectAllAttrib, NULL, NULL, IUPAF_WRITEONLY | IUPAF_NO_INHERIT);
#if !TARGET_OS_IPHONE
  iupClassRegisterAttribute(ic, "ZOOM", cocoaTouchWebBrowserGetZoomAttrib, cocoaTouchWebBrowserSetZoomAttrib, NULL, NULL, IUPAF_NO_INHERIT);
#endif
  iupClassRegisterAttribute(ic, "PRINT", NULL, cocoaTouchWebBrowserSetPrintAttrib, NULL, NULL, IUPAF_WRITEONLY | IUPAF_NO_INHERIT);

#if 0
  iupClassRegisterAttribute(ic, "BACKCOUNT", cocoaTouchWebBrowserGetBackCountAttrib, NULL, NULL, NULL, IUPAF_NO_DEFAULTVALUE|IUPAF_READONLY|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "FORWARDCOUNT", cocoaTouchWebBrowserGetForwardCountAttrib, NULL, NULL, NULL, IUPAF_NO_DEFAULTVALUE|IUPAF_READONLY|IUPAF_NO_INHERIT);
  iupClassRegisterAttributeId(ic, "ITEMHISTORY",  cocoaTouchWebBrowserGetItemHistoryAttrib,  NULL, IUPAF_READONLY|IUPAF_NO_INHERIT);
#endif

  iupClassRegisterAttribute(ic, "CANGOBACK", cocoaTouchWebBrowserCanGoBackAttrib, NULL, NULL, NULL, IUPAF_NO_DEFAULTVALUE|IUPAF_READONLY|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "CANGOFORWARD", cocoaTouchWebBrowserCanGoForwardAttrib, NULL, NULL, NULL, IUPAF_NO_DEFAULTVALUE|IUPAF_READONLY|IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "GOBACK", NULL, cocoaTouchWebBrowserSetBackAttrib, NULL, NULL, IUPAF_WRITEONLY|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "GOFORWARD", NULL, cocoaTouchWebBrowserSetForwardAttrib, NULL, NULL, IUPAF_WRITEONLY|IUPAF_NO_INHERIT);

  return ic;
}
