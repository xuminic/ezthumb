/*global document alert IupCommon UTF8ToString IupCommon autoAddDeps mergeInto LibraryManager*/

var LibraryIupProgressBar = {
  $ProgressBar: {},

  emjsProgressBar_Create: function()
  {
    var widget_object;
    widget_object = document.createElement('progress');
    console.log(widget_object);
    var handle_id = IupCommon.RegisterNewObject(widget_object);
    IupCommon.InitializeObject(widget_object);
    return handle_id;
  },
  emjsProgressBar_SetValueAttrib: function(handle_id, value)
  {
   //todo
    var widget_object = IupCommon.GetObjectForID(handle_id);
    widget_object.value = value;
  }
}

autoAddDeps(LibraryIupProgressBar, '$ProgressBar');
mergeInto(LibraryManager.library, LibraryIupProgressBar);
