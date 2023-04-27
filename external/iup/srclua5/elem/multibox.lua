------------------------------------------------------------------------------
-- MultiBox class 
------------------------------------------------------------------------------
local ctrl = {
  nick = "multibox",
  parent = iup.BOX,
  subdir = "elem",
  creation = "-",
  funcname = "MultiBox",
  callback = {}
}

function ctrl.createElement(class, param)
   return iup.MultiBox()
end

iup.RegisterWidget(ctrl)
iup.SetClass(ctrl, "iupWidget")
