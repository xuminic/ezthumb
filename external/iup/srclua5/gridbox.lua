------------------------------------------------------------------------------
-- GridBox class 
------------------------------------------------------------------------------
local ctrl = {
  nick = "gridbox",
  parent = iup.BOX,
  creation = "-",
  funcname = "GridBox",
  callback = {}
}

function ctrl.append (handle, elem)
  iup.Append(handle, elem)
end

function ctrl.createElement(class, param)
   return iup.GridBox()
end

iup.RegisterWidget(ctrl)
iup.SetClass(ctrl, "iup widget")
