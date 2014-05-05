------------------------------------------------------------------------------
-- ProgressDlg class 
------------------------------------------------------------------------------
local ctrl = {
  nick = "progressdlg",
  parent = iup.WIDGET,
  creation = "",
  callback = {
    cancel_cb = "",
  },
  funcname = "ProgressDlg"
} 

function ctrl.showxy(handle, x, y)
  return iup.ShowXY(handle, x, y)
end

function ctrl.popup(handle, x, y)
  iup.Popup(handle,x,y)
end

function ctrl.createElement(class, param)
   return iup.ProgressDlg()
end
   
iup.RegisterWidget(ctrl)
iup.SetClass(ctrl, "iup widget")
