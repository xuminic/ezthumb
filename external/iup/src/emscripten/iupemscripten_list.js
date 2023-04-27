/*global document alert IupCommon UTF8ToString IupCommon autoAddDeps mergeInto LibraryManager*/

var LibraryIupList = {
  //	$Button__deps: ['$CommonGlobals'],
  $List: {
  },


  emjsList_CreateList: function(sub_type, data, arr_size) {
    var num_widgets = 0;

    switch (sub_type) {
      case 0:
        // dropdown
        var widget_object = document.createElement("select");
        break;

      case 1:
        // editbox+dropdown
        // need to create <input> element to bind to <datalist>
        var input_box = document.createElement("datalist");
        var input_id = IupCommon.RegisterNewObject(input_box);
        IupCommon.InitializeObject(input_box);
        num_widgets++;

        // sets the data attribute, which binds the <input> to the <datalist>
        {{{ makeSetValue('data', '4', 'input_id', 'i32') }}};
        var widget_object = document.createElement("input");
        break;

      case 2:
      case 3:
        // multiple or single list
        var widget_object = document.createElement("select");
        widget_object.setAttribute('multiple', 'multiple');
        break;

      case 4:
        break;

      default:
        break;
    }

    // var widget_object = document.createElement("datalist");
    var handle_id = IupCommon.RegisterNewObject(widget_object);
    IupCommon.InitializeObject(widget_object);
    num_widgets++;
    {{{ makeSetValue('data', '0', 'handle_id', 'i32') }}};

    // set id(s) of created elems; for input, attach to list id
    if (sub_type == 1) {
      widget_object.setAttribute("list", "list_" + input_id);
      input_box.id = 'list_' + input_id;
    }
    else widget_object.id = 'list_' + handle_id;
    return num_widgets;
  },

  // emjsList_CreateListDropdown: function() {
  //   console.log("Create list fired");
  //   var widget_object;
  //   widget_object = document.createElement("select");
  //   var handle_id = IupCommon.RegisterNewObject(widget_object);
  //   widget_object.id = 'list_' + handle_id;
  //   return handle_id;
  // },

  emjsList_DestroyList: function(handle_id) {
    // Do I need to removeEventListener?
    IupCommon.DeleteObject(handle_id);
  },
  emjsList_GetCount: function(handle_id, sub_type) {
    var widget_object = IupCommon.GetObjectForID(handle_id);
    switch (sub_type) {
      case 0:
        // dropdown
        return widget_object.length;
      case 1:
        // editbox+dropdown
        return widget_object.options.length;
      case 2:
      case 3:
        // multiple or single list
        return widget_object.length;
      case 4:
        return "";
      default:
        return 0;
    }
  },

  emjsList_AppendItem: function(handle_id, sub_type, value) {
    var widget_object = IupCommon.GetObjectForID(handle_id);

    switch (sub_type) {
      case 0:
        // dropdown
        var item = document.createElement('option');
        item.innerHTML = UTF8ToString(value);
        widget_object.appendChild(item);
        return "";
      case 1:
        // editbox+dropdown
        var item = document.createElement('option');
        item.innerHTML = UTF8ToString(value);
        widget_object.appendChild(item);
        return "";
      case 2:
      case 3:
        // multiple or single list
        var item = document.createElement('option');
        item.innerHTML = UTF8ToString(value);
        widget_object.appendChild(item);
        return "";
      case 4:
        return "";
      default:
        return 0;
    }
  },

  emjsListCreateIdValueAttrib: function(handle_id, pos) {
    var widget_object = IupCommon.GetObjectForID(handle_id);
    var ret_str = widget_object.options[pos].text;
    var c_str = allocate(intArrayFromString(ret_str), 'i8', ALLOC_NORMAL);
    return c_str;
  }
};

autoAddDeps(LibraryIupList, '$List');
mergeInto(LibraryManager.library, LibraryIupList);
