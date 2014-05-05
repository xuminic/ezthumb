
------------------------------------------------------------------------------
-- Callback handler  
------------------------------------------------------------------------------

iup.callbacks = {} -- storage for the C callbacks

function iup.CallMethod(name, ...)
  local handle = ... -- the first argument is always the handle
  local lua_func = handle[name] -- use "gettable" to retrieve the Lua callback
  if (not lua_func) then
    return
  end
  
  if type(lua_func) == "function" then
    return lua_func(...)
  elseif type(lua_func) == "string" then  
    local temp = self
    self = handle
    local result = iup.dostring(lua_func)
    self = temp
    return result
  else
    return iup.ERROR
  end
end

function iup.RegisterCallback(name, c_func, type)
  -- Store a C callback for future use
  if not iup.callbacks[name] then iup.callbacks[name] = {} end
  local cb = iup.callbacks[name]
  if type then
    cb[type] = c_func
  else
    cb[1] = c_func
  end
end

function iup.CallGlobalMethod(name, ...)
  local INDEX = string.upper(name)
  local cb = iup.callbacks[INDEX]
  if (not cb) then 
    return
  end

  local lua_func = cb[2]
  if type(lua_func) == "function" then
    return lua_func(...)
  elseif type(lua_func) == "string" then  
    return iup.dostring(lua_func)
  else
    return iup.ERROR
  end
end

function iup.SetGlobalCallback(name, lua_func)
  local INDEX = string.upper(name)
  local cb = iup.callbacks[INDEX]
  if (cb) then -- if a callback name
    if (lua_func) then
      local c_func = cb[1]
      iup.SetFunction(INDEX, c_func) -- set the pre-defined C callback
    else
      iup.SetFunction(INDEX, nil)
    end
    cb[2] = lua_func -- store also the Lua callback
  end
end


------------------------------------------------------------------------------
-- Meta Methods 
------------------------------------------------------------------------------


local widget_gettable = function(object, index)
  local p = object
  local v
  while 1 do
    v = rawget(p, index)
    if v then return v end
    p = rawget(p, "parent")
      if not p then return nil end
  end
end

iup.NewClass("iup widget")
iup.SetMethod("iup widget", "__index", widget_gettable)


local ihandle_gettable = function(handle, index)
  local INDEX = string.upper(index)
  if (iup.callbacks[INDEX]) then 
   local object = iup.GetWidget(handle)
   if (not object or type(object)~="table") then error("invalid iup handle") end
   return object[index]
  else
    local value = iup.GetAttribute(handle, INDEX)
    if (not value) then
      local object = iup.GetWidget(handle)
      if (not object or type(object)~="table") then error("invalid iup handle") end
      return object[index]
    elseif type(value)== "number" or type(value) == "string" then
      local ih = iup.GetHandle(value)
      if ih then return ih
      else return value end
    else
      return value 
    end
  end
end

local ihandle_settable = function(handle, index, value)
  local ti = type(index)
  local tv = type(value)
  local object = iup.GetWidget(handle)
  if (not object or type(object)~="table") then error("invalid iup handle") end
  if ti == "number" or ti == "string" then -- check if a valid C name
    local INDEX = string.upper(index)
    local cb = iup.callbacks[INDEX]
    if (cb) then -- if a callback name
      local c_func = cb[1]
      if (not c_func) then
        c_func = cb[iup.GetClassName(handle)]
      end
      iup.SetCallback(handle, INDEX, c_func, value) -- set the pre-defined C callback
      object[index] = value -- store also in Lua
    elseif iup.GetClass(value) == "iup handle" then -- if a iup handle
      local name = iup.SetHandleName(value)
      iup.SetAttribute(handle, INDEX, name)
      object[index] = nil -- if there was something in Lua remove it
    elseif tv == "string" or tv == "number" or tv == "nil" then -- if a common value
      iup.SetAttribute(handle, INDEX, value)
      object[index] = nil -- if there was something in Lua remove it
    else
      object[index] = value -- store also in Lua
    end
  else
    object[index] = value -- store also in Lua
  end
end

iup.NewClass("iup handle")
iup.SetMethod("iup handle", "__index", ihandle_gettable)
iup.SetMethod("iup handle", "__newindex", ihandle_settable)
iup.SetMethod("iup handle", "__tostring", iup.ihandle_tostring) -- implemented in C
iup.SetMethod("iup handle", "__eq", iup.ihandle_compare) -- implemented in C


------------------------------------------------------------------------------
-- Utilities 
------------------------------------------------------------------------------

function iup.SetHandleName(v)  -- used also by radio and zbox
  local name = iup.GetName(v)
  if not name then
    local autoname = string.format("_IUPLUA_NAME(%s)", tostring(v))
    iup.SetHandle(autoname, v)
    return autoname
  end
  return name
end

function iup.RegisterWidget(ctrl) -- called by all the controls initialization functions
  iup[ctrl.nick] = function(param)
    if (not ctrl.constructor) then print(ctrl.nick) end
    return ctrl:constructor(param)
  end
end

function iup.RegisterHandle(handle, typename)

  iup.SetClass(handle, "iup handle")
  
  local object = iup.GetWidget(handle)
  if not object then

    local class = iup[string.upper(typename)]
    if not class then
      class = WIDGET
    end

    local object = { parent=class, handle=handle }
    iup.SetClass(object, "iup widget")
    iup.SetWidget(handle, object)
  end
  
  return handle
end

------------------------------------------------------------------------------
-- Widget class (top class) 
------------------------------------------------------------------------------

iup.WIDGET = {
  callback = {}
}

function iup.WIDGET.show(object)
  iup.Show(object.handle)
end

function iup.WIDGET.hide(object)
  iup.Hide(object.handle)
end

function iup.WIDGET.map(object)
  iup.Map(object.handle)
end

function iup.WIDGET.unmap(object)
  iup.Unmap(object.handle)
end

function iup.WIDGET.destroy(object)
  iup.Destroy(object.handle)
end

function iup.WIDGET.constructor(class, param)
  local handle = class:createElement(param)
  local object = { 
    parent = class,
    handle = handle
  }
  iup.SetClass(handle, "iup handle")
  iup.SetClass(object, "iup widget")
  iup.SetWidget(handle, object)
  object:setAttributes(param)
  return handle
end

function iup.WIDGET.setAttributes(object, param)
  local handle = object.handle
  for i,v in pairs(param) do 
    if type(i) == "number" and iup.GetClass(v) == "iup handle" then
      -- We should not set this or other elements (such as iuptext)
      -- will erroneosly inherit it
      rawset(object, i, v)
    else
      -- this will call settable metamethod
      handle[i] = v
    end
  end
end

-- all the objects in the hierarchy must be "iup widget"
-- Must repeat this call for every new widget
iup.SetClass(iup.WIDGET, "iup widget")


------------------------------------------------------------------------------
-- Box class (inherits from WIDGET) 
------------------------------------------------------------------------------

iup.BOX = {
  parent = iup.WIDGET
}

function iup.BOX.setAttributes(object, param)
  local handle = rawget(object, "handle")
  local n = #param
  for i = 1, n do
    if iup.GetClass(param[i]) == "iup handle" then 
      iup.Append(handle, param[i]) 
    end
  end
  iup.WIDGET.setAttributes(object, param)
end

iup.SetClass(iup.BOX, "iup widget")


------------------------------------------------------------------------------
-- Compatibility functions.
------------------------------------------------------------------------------

iup.error_message_popup = nil

function iup._ERRORMESSAGE(msg,traceback)
  msg = msg..(traceback or "")
  if (iup.error_message_popup) then
    iup.error_message_popup.value = msg
  else  
    local bt = iup.button{title="Ok", size="60", action="iup.error_message_popup = nil; return iup.CLOSE"}
    local ml = iup.multiline{expand="YES", readonly="YES", value=msg, size="300x150"}
    local vb = iup.vbox{ml, bt; alignment="ACENTER", margin="10x10", gap="10"}
    local dg = iup.dialog{vb; title="Error Message",defaultesc=bt,defaultenter=bt,startfocus=bt}
    iup.error_message_popup = ml
    dg:popup(CENTER, CENTER)
    dg:destroy()
    iup.error_message_popup = nil
  end
end

iup.pack = function (...) return {...} end

function iup.protectedcall(f, msg)
  if not f then 
    iup._ERRORMESSAGE(msg)
    return 
  end
  local ret = iup.pack(pcall(f))
  if not ret[1] then 
    iup._ERRORMESSAGE(ret[2])
    return
  else  
    table.remove(ret, 1)
    return unpack(ret)   --must replace this by table.unpack when 5.1 is not supported
  end
end

function iup.dostring(s) return iup.protectedcall(loadstring(s)) end
function iup.dofile(f) return iup.protectedcall(loadfile(f)) end

function iup.RGB(r, g, b)
  return string.format("%d %d %d", 255*r, 255*g, 255*b)
end

-- This will allow both names to be used in the same application
-- also will allow static linking to work with require for the main library (only)
if _G.package then
   _G.package.loaded["iuplua"] = iup
   iup._M = iup
   iup._PACKAGE = "iuplua"
end

function iup.layoutdialog(obj)
  return iup.LayoutDialog(obj[1])
end
