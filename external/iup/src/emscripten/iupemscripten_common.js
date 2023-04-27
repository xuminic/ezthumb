

var LibraryIupCommon = {
//	$Common__deps: ['$CommonGlobals'],
	$IupCommonGlobals: {
		idCounter: 1,
		objectIDMap: {},
	},

	$IupCommon: {


		GetObjectForID: function(handle_id) {
			return IupCommonGlobals.objectIDMap[handle_id];
		},
		SetObjectForID: function(the_object, handle_id) {
			IupCommonGlobals.objectIDMap[handle_id] = the_object;
		},
		RegisterNewObject: function(the_object) {
			var current_id = IupCommonGlobals.idCounter;
			IupCommonGlobals.objectIDMap[current_id] = the_object;
			the_object.handleID = current_id;
			IupCommonGlobals.idCounter += 1;
			return current_id;
		},
    InitializeObject: function(the_object) {
      // any other useful properties for all objects should go here
      the_object.style.position = 'absolute';
    },
		DeleteObject: function(handle_id) {
			var the_object = IupCommonGlobals.objectIDMap[handle_id] = null;
			the_object.handleID = null;
			IupCommonGlobals.objectIDMap[handle_id] = null;
			the_object = null;
		},
	},

  emjsCommon_SetPosition: function(handle_id, x, y, width, height)
  {

    // var real_elem = document.getElementById(handle_id);

		var elem = IupCommon.GetObjectForID(handle_id);

    // elem.style.color = 'white';
    elem.style.left = x + 'px';
    console.log(elem.nodeName);
    console.log("left: " + elem.style.left);
    elem.style.top = y + 'px';
    console.log("top: " + elem.style.top);

    // elem.style.width =
    console.log("width: " + width);
    console.log("height: " + height);
    console.log("actual width: " + elem.style.width);
    console.log("actual height: " + elem.style.height);
    // if (width != 0 && height != 0) {
      elem.style.width = width + 'px';
      elem.style.height = height + 'px';
    // }
    console.log("actual width after: " + elem.style.width);
    console.log("actual height after: " + elem.style.height);

  },

	emjsCommon_AddWidgetToDialog: function(parent_id, child_id) {
    console.log("AddWidgetToDialog");
		var parent_dialog = IupCommon.GetObjectForID(parent_id);
		var parent_body = parent_dialog.document.getElementsByTagName("body")[0];
		var child_widget = IupCommon.GetObjectForID(child_id);
		parent_body.appendChild(child_widget);
	},

  // <div> -- inject code -- </div>
  emjsCommon_AddCompoundToDialog: function(parent_id, elem_array, num_elems) {
    var parent_dialog = IupCommon.GetObjectForID(parent_id);
    var parent_body = parent_dialog.document.getElementsByTagName("body")[0];
    var child_widget;

    for (var i = 0; i < num_elems; i++) {
      var child_id = {{{ makeGetValue('elem_array', 'i*4', 'i32') }}};
      child_widget = IupCommon.GetObjectForID(child_id);
      parent_body.appendChild(child_widget);
    }
  },

  emjsCommon_AddCompoundToWidget: function(parent_id, elem_array, num_elems) {
		var parent_widget = IupCommon.GetObjectForID(parent_id);
    var child_widget;

    for (var i = 0; i < num_elems; i++) {
      var child_id = {{{ makeGetValue('elem_array', 'i*4', 'i32') }}};
      child_widget = IupCommon.GetObjectForID(child_id);
      parent_body.appendChild(child_widget);
    }
  },

	emjsCommon_AddWidgetToWidget: function(parent_id, child_id) {
    console.log("AddWidgetToWidget");
		var parent_widget = IupCommon.GetObjectForID(parent_id);
		var child_widget = IupCommon.GetObjectForID(child_id);

		parent_widget.appendChild(child_widget);
	},

  emjsCommon_Alert: function(message) {
    console.log("alert message: ");
    console.log(message);
    alert(UTF8ToString(message));
  },

  emjsCommon_Log: function(message) {
//    console.log("our log is working");
      console.log(UTF8ToString(message));
  },

	emjsCommon_IupLog: function(priority, message)
	{
		var stringified_message = UTF8ToString(message);
		/*	 
		enum IUPLOG_LEVEL
		{
			IUPLOG_LEVEL_LOG = 0,
			IUPLOG_LEVEL_DEBUG,
			IUPLOG_LEVEL_INFO,
			IUPLOG_LEVEL_WARN,
			IUPLOG_LEVEL_ERROR
		};
		*/

		switch(priority)
		{
			case 1:
				console.debug(stringified_message);
				break; 
			case 2:
				console.info(stringified_message);
				break;
			case 3:
				console.warn(stringified_message);
				break;
			case 4:
				console.error(stringified_message);
				break;
			default: 
				console.log(stringified_message);
		}
	},




  emjsCommon_SetFgColor: function(handle_id, r, g, b)
  {
    var current_widget = IupCommon.GetObjectForID(handle_id);
    console.log(current_widget);
    current_widget.style.color = "rgb(" + r + "," + g + "," + b + ")";
  },

	// Used only the the no-pthreads-path. I don't know how to conditionally compile JavaScript fragments here, so this always gets included, whether used or not.
	emjsCommon_IupPostMessageNoThreads: function(ih, s, i, d, message_data)
	{
    	var postMessageCallback = function(number_ih, str, number_i, number_d, number_message_data)
		{
			var c_callback = Module.cwrap('emscriptenIupPostMessageNoThreadsCallbackTrampoline', null, ['number', 'string', 'number', 'number', 'number']);

			c_callback(number_ih, str, number_i, number_d, number_message_data);
		}
		// This was a lot of trial & error to figure out how to get the string parameter all the way through.
		// Ultimately, I needed to use UTF8ToString on the char* to pass through the Trampoline as a 'string'.
		// This allowed it to re-emerge properly as a string with the correct contents in the EMSCRIPTEN_KEEPALIVE trampoline callback.
		// But additionally, I'm concerned about the life-cycle of the original char* s passed into the function since callback may happen after the user frees the original string.
		// So I need to make a copy. But it gets hairy trying to manage the memory on the C side 
		// because if I strdup() initially, the pointer that surfaces in the trampoline may be different 
		// because we used UTF8ToString. And I'm also a little nervous about allocating memory on the C-side and _free()'ing it on the JS side.
		// So instead, we can use UTF8ToString here, which should make a JavaScript copy of the string and we can let the system manage the life-cycle of that.
		var stringified_s = UTF8ToString(s);

		setTimeout(
			postMessageCallback,
			0,
			ih,
			stringified_s,
			i,
			d,
			message_data
		);
  },

  iupEmscriptenSetFgColor: function(handle_id, r, g, b) {
	  var sel_object = IupCommon.GetObjectForID(handle_id);
    sel_object.style.color = "rgb(" + r + "," + g + "," + b + ")";
  }

};

autoAddDeps(LibraryIupCommon, '$IupCommonGlobals');
autoAddDeps(LibraryIupCommon, '$IupCommon');
mergeInto(LibraryManager.library, LibraryIupCommon);
