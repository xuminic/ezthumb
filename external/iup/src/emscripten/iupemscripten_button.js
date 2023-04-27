

var LibraryIupButton = {
//  $Button__deps: ['$CommonGlobals'],
  $Button: {
  },

  emjsButton_CreateButton: function() {
    var widget_object = document.createElement("button");
    var handle_id = IupCommon.RegisterNewObject(widget_object);

    IupCommon.InitializeObject(widget_object);

    return handle_id;
  },

  emjsButton_DestroyButton: function(handle_id) {
    // Do I need to removeEventListener?
    IupCommon.DeleteObject(handle_id);
  },

  emjsButton_SetTitle: function(handle_id, button_title) {
    var widget_object = IupCommon.GetObjectForID(handle_id);
    if(widget_object)
    {
      widget_object.innerHTML = UTF8ToString(button_title);
    }
  },

  emjsButton_SetCallback: function(handle_id, ih_pointer) {

    var button = IupCommon.GetObjectForID(handle_id);
    //button.ihPointer = ih_pointer;
    if(button)
    {
      // ACTION corresponds to onlick in javascript
      button.addEventListener("click", function() { 
        var c_callback = Module.cwrap('emscriptenButtonCallbackTrampoline', null, ['number']);
        c_callback(handle_id);
        console.log('click happening');
      });

      // for BUTTON_CB, need on mousedown and onmouseup
      button.addEventListener("mousedown", function(e) {
        var clicktype = e.button;
        var xCoord = e.clientX;
        var yCoord = e.clientY;
        var c_callback = Module.cwrap('emscriptenButtonCallbackTrampoline_Cb', null, ['number', 'number', 'number', 'number', 'number', 'string']);
        var pressed = 1;
        console.log(handle_id);
        c_callback(handle_id, clicktype, pressed, xCoord, yCoord, "");
        console.log('bdown happening');
      });

      button.addEventListener("mouseup", function(e) {
        var clicktype = e.button;
        var xCoord = e.clientX;
        var yCoord = e.clientY;
        var c_callback = Module.cwrap('emscriptenButtonCallbackTrampoline_Cb', null, ['number', 'number', 'number', 'number', 'number', 'string']);
        var pressed = 0;
        c_callback(handle_id, clicktype, pressed, xCoord, yCoord, "");
        console.log('bup happening');
      });
    }

  },


};

autoAddDeps(LibraryIupButton, '$Button');
mergeInto(LibraryManager.library, LibraryIupButton);

