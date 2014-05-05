------------------------------------------------------------------------------
-- ScrollBox class 
------------------------------------------------------------------------------
local ctrl = {
  nick = "scrollbox",
  parent = iup.WIDGET,
  creation = "I",
  funcname = "ScrollBox",
  callback = {}
}

function ctrl.createElement(class, param)
   return iup.ScrollBox(param[1])
end

iup.RegisterWidget(ctrl)
iup.SetClass(ctrl, "iup widget")
