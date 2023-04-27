------------------------------------------------------------------------------
-- FlatList class 
------------------------------------------------------------------------------
local ctrl = {
  nick = "flatlist",
  parent = iup.WIDGET,
  subdir = "elem",
  creation = "",
  funcname = "FlatList",
  callback = {
     flat_action = "snn", 
--     multiselect_cb = "s",
--     dblclick_cb = "ns",
--     flat_button_cb = "nnnns",
     flat_motion_cb = "nns",
--     flat_focus_cb = "n",
   }
} 

function ctrl.createElement(class, param)
  return iup.FlatList()
end
   
iup.RegisterWidget(ctrl)
iup.SetClass(ctrl, "iupWidget")
