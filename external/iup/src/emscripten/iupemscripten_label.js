/*global document alert IupCommon UTF8ToString IupCommon autoAddDeps mergeInto LibraryManager*/

var LibraryIupLabel = {
  //	$Button__deps: ['$CommonGlobals'],
  $Label: {
  },

  emjsLabel_CreateLabel: function() {
    // TODO: need to refactor to support image label
    // should create canvas element from image data in iup
    // also need one for separators (horizontal and vertical)
    var widget_object;
    widget_object = document.createElement("div");

    var handle_id = IupCommon.RegisterNewObject(widget_object);
    IupCommon.InitializeObject(widget_object);

    return handle_id;
  },

  emjsLabel_CreateImage: function() {
    var widget_object;
    //widget_object = document.createElement("CANVAS");
    widget_object = document.createElement("IMG");
    var handle_id = IupCommon.RegisterNewObject(widget_object);
    IupCommon.InitializeObject(widget_object);

    return handle_id;
  },
  emjsLabel_CreateSeparatorHorizontal: function(handle_id) {
    var widget_object = IupCommon.GetObjectForID(handle_id);
    var h = document.createElement("hr");
    widget_object.appendChild(h); 
  },

  emjsLabel_CreateSeparatorVertical: function(handle_id) {
    var widget_object = IupCommon.GetObjectForID(handle_id);
    alert("vertical separator");
  },


  emjsLabel_DestroyLabel: function(handle_id) {
    // Do I need to removeEventListener?
    IupCommon.DeleteObject(handle_id);
  },

  emjsLabel_SetTitle: function(handle_id, label_title) {
    // console.log("label title " + label_title);
    var widget_object = IupCommon.GetObjectForID(handle_id);
    if(widget_object)
    {
      widget_object.innerHTML = UTF8ToString(label_title);
    }
  },

  emjsLabel_SetPadding: function(handle_id, horiz, vert) {
    var widget_object = IupCommon.GetObjectForID(handle_id);

    // how does padding work in Iup?  Is it from corners or center?
    widget_object.style.paddingTop = vert + "px";
    widget_object.style.paddingBottom = vert + "px";
    widget_object.style.paddingLeft = horiz + "px";
    widget_object.style.paddingRight = horiz + "px";
  },

  emjsLabel_SetFGColor: function(handle_id, color) {
    var widget_object = IupCommon.GetObjectForID(handle_id);

    // what format is color? assuming its an rgb separated by spaces
    // console.log('handle id: ' + handle_id);
    // console.log('fg color: ' + color);
    // if (color) {
    //   var rgbVals = color.split(" ");
    //   var jsColor = 'rgb(' + rgbVals[0] + ', ' + rgbVals[1] + ', ' + rgbVals[2] + ')'; 
    //   widget_object.style.color = jsColor;
    // }
  },

  emjsLabel_SetBGColor: function(obj, val) {
    // console.log("bgcolor first arg: " + obj);
    // console.log("bgcolor sec arg: " + val);
  },

  emjsLabel_ToggleActive: function(handle_id, enable) {
    var widget_object = IupCommon.GetObjectForID(handle_id);
    enable === 1 ? widget_object.disabled = false : widget_object.disabled = true;
  },

  emjsLabel_SetAlignmentAttrib: function(handle_id, value) {
    // FUT - check to make sure ih->data->type != IUP_LABEL_SETP_HORIZ

    // get object to apply alignment to
    var widget_object = IupCommon.GetObjectForID(handle_id);
    var alignment = UTF8ToString(value);

    // works like ARIGHT:ABOTTOM
    if (alignment === "ARIGHT") {
      widget_object.style.height = "30px";
      widget_object.style.display = "table-cell";
      widget_object.style.verticalAlign = "middle";
      widget_object.style.textAlign = "right";  
    }
    else if (alignment === "ACENTER") {

      widget_object.style.height = "30px";
      widget_object.style.display = "table-cell";
      widget_object.style.verticalAlign = "middle";
      widget_object.style.textAlign = "center";
    }
    else {  // must be ALEFT

      widget_object.style.height = "30px";
      widget_object.style.display = "table-cell";
      widget_object.style.verticalAlign = "middle";
      widget_object.style.textAlign = "left"; 
    }

    // cant really align vertically in CSS like it does in gtk...
    // will need to further understand how the containers work and
    // maybe use padding or margin percentage to emulate

    return 1;
  },


  emjsLabel_SetImageAttrib: function(handle_id, pixel_ptr, array_length, width, height, pitch) {
//    var canvas_widget = IupCommon.GetObjectForID(handle_id);
	  // NOTE: Originally, I created a Canvas as the root object and just drew to the canvas.
	  // But growing/shrinking of the image as the canvas was resized does not appear to be built-in.
	  // But for the IMG object, we get resizing for free.
	  // The problem is how to go from a pixel array to an image.
	  // Turns out we can convert a Canvas to an Image using toDataURL.
	  // So I changed the root object to be IMG instead of CANVAS.
	  // This means I need to create a temporary CANVAS to render to an IMG.
	  // All the various data conversions we are doing concern me wrt to performance, but I'm not sure what can be done to improve this.
	  // I also don't know the impact of creating and collecting a temporary canvas object is.
	  // Nor do I know how much RAM keeping a temporary canvas in the background always resident would be.
	  // We could try to reuse/share a single canvas to cut down memory.
	  // However, if any of this stuff needs to be re-entrant (perhaps we start looking at async/threads?)
	  // then sharing a canvas would be a bad idea.
	  // (Also, for some of the IUP alignment attributes, it may turn out we need more control over the drawing and may need to revert back to Canvas.)
	  // So for now, we do the simple thing of creating a temporary canvas and collecting it.
	  var canvas_widget = document.createElement("CANVAS");
	  var canvas_context = canvas_widget.getContext("2d");


	  // We must set the Canvas size to match the image size or we get a final png that either has tons of extra padding space around the image or too small.
	  // TODO: I am not sure if I need to set both the canvas and context sizes.
	  canvas_widget.width  = width;
	  canvas_widget.height = height;
	  canvas_context.canvas.width  = width;
	  canvas_context.canvas.height = height;


	// pitch is effectively width (with alignment padding) * 4 bytes per pixel
    var pixal_data = Module.HEAPU8.slice(pixel_ptr, pixel_ptr + pitch * height);
//  var pixal_data = Module.HEAPU8.slice(pixel_ptr, pixel_ptr + width * height * 4);
    var image_data = canvas_context.getImageData(0, 0, width, height);
    image_data.data.set(pixal_data);
    canvas_context.putImageData(image_data, 0, 0);
	// At this point we have a usable Canvas with an image in it.

	// So now convert the Canvas into something we can use with IMG.
	var image_dataurl = canvas_widget.toDataURL("image/png");      
    var image_widget = IupCommon.GetObjectForID(handle_id);
	image_widget.src = image_dataurl;
  },

  emjsLabel_DropFilesTarget: function(handle_id) {
    var widget_object = IupCommon.GetObjectForID(handle_id);
    var elem = document.createElement('div');
    elem.style.border = '1px solid black';
    elem.style.height = '100px';
    elem.style.width = '200px';
    elem.ondrop = this.dropHandler(event);
    elem.ondragover = this.dragOverHandler(event);
  },

  dropHandler: function(ev) {
    console.log('File(s) dropped');

    // Prevent default behavior (Prevent file from being opened)
    ev.preventDefault();

    if (ev.dataTransfer.items) {
      // Use DataTransferItemList interface to access the file(s)
      for (var i = 0; i < ev.dataTransfer.items.length; i++) {
        // If dropped items aren't files, reject them
        if (ev.dataTransfer.items[i].kind === 'file') {
          var file = ev.dataTransfer.items[i].getAsFile();
          console.log('... file[' + i + '].name = ' + file.name);
        }
      }
    } else {
      // Use DataTransfer interface to access the file(s)
      for (var i = 0; i < ev.dataTransfer.files.length; i++) {
        console.log('... file[' + i + '].name = ' + ev.dataTransfer.files[i].name);
      }
    } 
    
    // Pass event to removeDragData for cleanup
    this.removeDragData(ev);
  },

  removeDragData: function(ev) {
    console.log('Removing drag data');

    if (ev.dataTransfer.items) {
      // Use DataTransferItemList interface to remove the drag data
      ev.dataTransfer.items.clear();
    } else {
      // Use DataTransfer interface to remove the drag data
      ev.dataTransfer.clearData();
    }
  },

  dragOverHandler: function(ev) {
    console.log('File(s) in drop zone'); 

    // Prevent default behavior (Prevent file from being opened)
    ev.preventDefault();
  },

  emjsLabel_EnableEllipsis: function(handle_id) {
    var widget_object = IupCommon.GetObjectForID(handle_id);
    widget_object.style.whiteSpace = "nowrap";
    widget_object.style.overflow = "hidden";
    widget_object.style.textOverflow = "ellipsis";
    // missing: -o-text-overflow: ellipsis;
  },

  emjsLabel_DisableWordWrap: function(handle_id) {
    var widget_object = IupCommon.GetObjectForID(handle_id);
    widget_object.style.whiteSpace = "-moz-pre-wrap"; /* Firefox */
    widget_object.style.whiteSpace = "-o-pre-wrap"; /* Opera */
    widget_object.style.whiteSpace = "pre-wrap"; /* Chrome */
    widget_object.style.wordWrap = "break-word"; /* IE */
  }

};

autoAddDeps(LibraryIupLabel, '$Label');
mergeInto(LibraryManager.library, LibraryIupLabel);

