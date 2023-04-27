/*global document alert IupCommon Pointer_stringify IupCommon autoAddDeps mergeInto LibraryManager*/

var LibraryIupWebBrowser = {
  //	$Button__deps: ['$CommonGlobals'],
  $WebBrowser: {
  },

  emjsWebBrowser_CreateBrowser: function() {
    var widget_object;
    widget_object = document.createElement('iframe');
    widget_object.src = 'https://www.mdn.org';
    widget_object.setAttribute('mozbrowser', 'remote');

    var handle_id = IupCommon.RegisterNewObject(widget_object);
    IupCommon.InitializeObject(widget_object);
    console.log(widget_object);
    return handle_id;
  },

  emjsWebBrowser_DestroyBrowser: function(handle_id) {
    IupCommon.DeleteObject(handle_id);
  },

  emjsWebBrowser_GetValueAttrib: function(handle_id) {
    var widget_object = IupCommon.GetObjectForID(handle_id);
    var source = widget_object.src;
    var c_str = allocate(intArrayFromString(source), 'i8', ALLOC_NORMAL);
    return c_str;
  },

  emjsWebBrowser_SetValueAttrib: function(handle_id, value) {
    var widget_object = IupCommon.GetObjectForID(handle_id);
    widget_object.src = Pointer_stringify(value);
  },

  emjsWebBrowser_GoBack: function(handle_id) {
    var widget_object = IupCommon.GetObjectForID(handle_id);
    widget_object.contentWindow.history.back();
  },

  emjsWebBrowser_GoForward: function(handle_id) {
    var widget_object = IupCommon.GetObjectForID(handle_id);
    widget_object.goForward();
  }

};

autoAddDeps(LibraryIupWebBrowser, '$WebBrowser');
mergeInto(LibraryManager.library, LibraryIupWebBrowser);
