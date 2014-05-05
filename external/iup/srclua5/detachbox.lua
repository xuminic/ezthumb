------------------------------------------------------------------------------
-- DetachBox class 
------------------------------------------------------------------------------
local ctrl = {
  nick = "detachbox",
  parent = iup.WIDGET,
  creation = "I",
  funcname = "DetachBox",
  callback = {
    detached_cb = "inn",
  }
}

function ctrl.createElement(class, param)
  return iup.DetachBox(param[1])
end

iup.RegisterWidget(ctrl)
iup.SetClass(ctrl, "iup widget")
