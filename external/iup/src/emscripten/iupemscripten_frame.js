/*global document alert IupCommon UTF8ToString IupCommon autoAddDeps mergeInto LibraryManager*/

var LibraryIupFrame = {
  //	$Button__deps: ['$CommonGlobals'],
  $Frame: {
  },

  emjsFrame_CreateFrame: function() {
    console.log("Create frame fired");
    var parent_obj, child_obj;
    parent_obj = document.createElement("fieldset");
    child_obj = document.createElement("legend");
    parent_obj.appendChild(child_obj);

    var handle_id = IupCommon.RegisterNewObject(parent_obj);
    parent_obj.id = 'frame_' + handle_id;
    return handle_id;
  },

  emjsFrame_DestroyFame: function(handle_id) {
    // Do I need to removeEventListener?
    IupCommon.DeleteObject(handle_id);
  }

};

autoAddDeps(LibraryIupFrame, '$Frame');
mergeInto(LibraryManager.library, LibraryIupFrame);

