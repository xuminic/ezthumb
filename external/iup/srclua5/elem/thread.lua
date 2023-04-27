
------------------------------------------------------------------------------
-- Thread class 
------------------------------------------------------------------------------
local ctrl = {
  nick = "thread",
  parent = iup.WIDGET,
  subdir = "elem",
  creation = "",
  callback = {
    thread_cb = "", 
  }
}

function ctrl.createElement(class, param)
  return iup.Thread()
end

iup.RegisterWidget(ctrl)
iup.SetClass(ctrl, "iupWidget")
