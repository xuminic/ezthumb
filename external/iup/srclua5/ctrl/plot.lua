------------------------------------------------------------------------------
-- Plot class 
------------------------------------------------------------------------------
local ctrl = {
  nick = "plot",
  parent = iup.WIDGET,
  creation = "",
  subdir = "ctrl",
  callback = {
    drawsample_cb = "nnddn",
    clicksample_cb = "nnddn",
    clicksegment_cb = "nnddnddn",
    select_cb = "nnddn",
    selectbegin_cb = "",
    selectend_cb = "",
    predraw_cb = "n",   -- fake definitions to be replaced by plotfuncs module
    postdraw_cb = "n",  -- fake definitions to be replaced by plotfuncs module
    editsample_cb = "nndd",
    delete_cb = "nndd",
    deletebegin_cb = "",
    deleteend_cb = "",
    propertieschanged_cb = "",
    propertiesvalidate_cb = "ss",
    dspropertieschanged_cb = "n",
    dspropertiesvalidate_cb = "in",
    plotmotion_cb = "dds",
    plotbutton_cb = "nndds",
    menucontext_cb = "inn",
    menucontextclose_cb = "inn",
    xtickformatnumber_cb = "ssds",   -- fake definitions to be replaced by plotfuncs module
    ytickformatnumber_cb = "ssds",   -- fake definitions to be replaced by plotfuncs module
  },
  include = "iup_plot.h",
  extrafuncs = 1,
}

ctrl.Begin = iup.PlotBegin 
ctrl.Add = iup.PlotAdd 
ctrl.AddStr = iup.PlotAddStr 
ctrl.AddSegment = iup.PlotAddSegment 
ctrl.End = iup.PlotEnd 
ctrl.LoadData = iup.PlotLoadData 
ctrl.SetFormula = iup.PlotSetFormula 
ctrl.FindSample = iup.PlotFindSample 
ctrl.Insert = iup.PlotInsert 
ctrl.InsertStr = iup.PlotInsertStr 
ctrl.InsertSegment = iup.PlotInsertSegment
ctrl.InsertSamples = iup.PlotInsertSamples 
ctrl.InsertStrSamples = iup.PlotInsertStrSamples 
ctrl.AddSamples = iup.PlotAddSamples 
ctrl.AddStrSamples = iup.PlotAddStrSamples 
ctrl.GetSample = iup.PlotGetSample
ctrl.GetSampleStr = iup.PlotGetSampleStr
ctrl.GetSampleSelection = iup.PlotGetSampleSelection
ctrl.SetSample = iup.PlotSetSample
ctrl.SetSampleStr = iup.PlotSetSampleStr
ctrl.SetSampleSelection = iup.PlotSetSampleSelection
ctrl.Transform = iup.PlotTransform
ctrl.TransformTo = iup.PlotTransformTo
ctrl.PaintTo = iup.PlotPaintTo

function ctrl.createElement(class, param)
   return iup.Plot(param.action)
end

iup.RegisterWidget(ctrl)
iup.SetClass(ctrl, "iupWidget")
