------------------------------------------------------------------------------
-- Space class 
------------------------------------------------------------------------------
local ctrl = {
  nick = "space",
  parent = iup.WIDGET,
  subdir = "elem",
  creation = "",
  callback = {}
}

function ctrl.createElement(class, param)
   return iup.Space()
end

iup.RegisterWidget(ctrl)
iup.SetClass(ctrl, "iupWidget")
