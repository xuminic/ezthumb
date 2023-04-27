------------------------------------------------------------------------------
-- MatrixList class 
------------------------------------------------------------------------------
local ctrl = {
  nick = "matrixlist",
  parent = iup.WIDGET,
  creation = "",
  subdir = "ctrl",
  callback = {
    action_cb = "nnnns", -- must repeat this callback because of its non-exclusive name
    mousemove_cb = "nn", -- must repeat this callback because of its non-exclusive name
    imagevaluechanged_cb = "nn",
    listclick_cb = "nns",
    listrelease_cb = "nns",
    listinsert_cb = "n",
    listremove_cb = "n",
--    listdraw_cb = "nnnnnnn",  implemented in matrix for matrixlist
    listedition_cb = "nnnn",
    listaction_cb = "nn",
  },
  funcname = "MatrixList",
  include = "iupcontrols.h",
--  extrafuncs = 1,
}

function ctrl.createElement(class, param)
   return iup.MatrixList()
end

iup.RegisterWidget(ctrl)
iup.SetClass(ctrl, "iupWidget")
