


var LibraryIupDialog = {
//	$IupDialog__deps: ['$CommonGlobals'],
	$IupDialog: {
		hasCreatedFirstDialog: false,

		// Oh, what a mess...different browsers return different things.
		// http://matanich.com/2013/01/07/viewport-size
		// Inner size Code adapted from https://github.com/tysonmatanich/viewportSize/blob/master/viewportSize.js
		// Tyson Matanich, 2013 | License: MIT
		// We don't want the scrollbar included with our "inner".
		// @param width_or_height_string_capitalized "Width" or "Height"
		// This is an internal helper function and not currently directly called by C.
		GetInnerWidthOrHeight: function(width_or_height_string_capitalized, the_window)
		{
			var size;
			var width_or_height_string_lowercase = width_or_height_string_capitalized.toLowerCase();
			var document = the_window.document;
			var documentElement = document.documentElement;
			if (the_window["inner" + width_or_height_string_capitalized] === undefined) {
				// IE6 & IE7 don't have the_window.innerWidth or innerHeight
				size = documentElement["client" + width_or_height_string_capitalized];
			}
			else if (the_window["inner" + width_or_height_string_capitalized] != documentElement["client" + width_or_height_string_capitalized]) {
				// WebKit doesn't include scrollbars while calculating viewport size so we have to get fancy

				// Insert markup to test if a media query will match document.doumentElement["client" + width_or_height_string_capitalized]
				var bodyElement = document.createElement("body");
				bodyElement.id = "vpw-test-b";
				bodyElement.style.cssText = "overflow:scroll";
				var divElement = document.createElement("div");
				divElement.id = "vpw-test-d";
				divElement.style.cssText = "position:absolute;top:-1000px";
				// Getting specific on the CSS selector so it won't get overridden easily
				divElement.innerHTML = "<style>@media(" + width_or_height_string_lowercase + ":" + documentElement["client" + width_or_height_string_capitalized] + "px){body#vpw-test-b div#vpw-test-d{" + name + ":7px!important}}</style>";
				bodyElement.appendChild(divElement);
				documentElement.insertBefore(bodyElement, document.head);

				if (divElement["offset" + width_or_height_string_capitalized] == 7) {
					// Media query matches document.documentElement["client" + width_or_height_string_capitalized]
					size = documentElement["client" + width_or_height_string_capitalized];
				}
				else {
					// Media query didn't match, use the_window["inner" + width_or_height_string_capitalized]
					size = the_window["inner" + width_or_height_string_capitalized];
				}
				// Cleanup
				documentElement.removeChild(bodyElement);
			}
			else {
				// Default to use window["inner" + width_or_height_string_capitalized]
				size = the_window["inner" + width_or_height_string_capitalized];
			}
			return size;
		},


	},


	emjsDialog_CreateDialog: function(window_name, width, height) {

		var dialog;

		if(IupDialog.hasCreatedFirstDialog)
		{
			var new_window_name = null;
			if(window_name)
			{
				new_window_name = UTF8ToString(window_name);
			}
			if(0 === width)
			{
				width = window.self.width;
			}
			if(0 === height)
			{
				height = window.self.height;
			}

			var attrib_str = "width=" + width + ",height=" + height;
			dialog = window.open("", new_window_name, attrib_str);
		}
		else
		{
			//dialog = document.getElementsByTagName("body")[0];
			dialog = window.self;
			IupDialog.hasCreatedFirstDialog = true;

			if(window_name)
			{
				dialog.document.title = UTF8ToString(window_name);
			}


		}

		current_id = IupCommon.RegisterNewObject(dialog);


		// We need to listen for onResize callback events to handle re-layout and RESIZE_CB.
		dialog.addEventListener("resize", function(resize_event)
		{ 
			console.log('resize happening w: ' + resize_event.target.outerWidth + ' , h: ' + resize_event.target.outerHeight);
			var c_callback = Module.cwrap('emscriptenDialogResizeCallbackTrampoline', null, ['number', 'number', 'number', 'number', 'number']);

			// Oh, what a mess...different browsers return different things.
			// http://matanich.com/2013/01/07/viewport-size
			// Code adapted from https://github.com/tysonmatanich/viewportSize/blob/master/viewportSize.js
			// Tyson Matanich, 2013 | License: MIT
			// We don't want the scrollbar included with our "inner".

			// c_callback(current_id, resize_event.target.outerWidth, resize_event.target.outerHeight, resize_event.target.innerWidth, resize_event.target.innerHeight);

			var inner_width = IupDialog.GetInnerWidthOrHeight("Width", dialog);
			var inner_height = IupDialog.GetInnerWidthOrHeight("Height", dialog);
			var outer_width = resize_event.target.outerWidth;
			var outer_height = resize_event.target.outerHeight;

			c_callback(current_id, outer_width, outer_height, inner_width, inner_height);
		});


		return current_id;

	},


	emjsDialog_DestroyDialog: function(handle_id) {

		var dialog = IupCommon.GetObjectForID(handle_id);
		if(dialog)
		{

			dialog.close();
			IupCommon.DeleteObjectWithID(handle_id);
		}
	},


	emjsDialog_GetSize: function(handle_id, out_ptr_outer_width, out_ptr_outer_height, out_ptr_inner_width, out_ptr_inner_height)
	{
		var widget_object = IupCommon.GetObjectForID(handle_id);
		// Oh, what a mess...different browsers return different things.
		// http://matanich.com/2013/01/07/viewport-size
		// Code adapted from https://github.com/tysonmatanich/viewportSize/blob/master/viewportSize.js
		// Tyson Matanich, 2013 | License: MIT
		// We don't want the scrollbar included with our "inner".

		var inner_width = IupDialog.GetInnerWidthOrHeight("Width", widget_object);
		var inner_height = IupDialog.GetInnerWidthOrHeight("Height", widget_object);
		var outer_width = widget_object.outerWidth;
		var outer_height = widget_object.outerHeight;


	    setValue(out_ptr_outer_width, outer_width, 'i32');
		setValue(out_ptr_outer_height, outer_height, 'i32');	
	    setValue(out_ptr_inner_width, inner_width, 'i32');
		setValue(out_ptr_inner_height, inner_height, 'i32');	
	}

	


}

autoAddDeps(LibraryIupDialog, '$IupDialog');
mergeInto(LibraryManager.library, LibraryIupDialog);

