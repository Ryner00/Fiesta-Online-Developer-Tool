WindowName = "HTDG-Editor"
function prepare(ElementPtr)
    MakeNoCollapse(ElementPtr)
end



function render(ElementPtr)
    local ret = true
    local ScenePtr = GetCurrentScenePtr(ElementPtr)
    local EditModePtr, EditModeName = GetCurrentEditMode(ScenePtr)

    if EditModeName == "HTDG" then
        local BrushSize = GetBrushSize(EditModePtr)
        local changed, BrushSize = DragInt(BrushSize,"Brush Size",1.0,0,100)
        if changed then
            SetBrushSize(EditModePtr, BrushSize)
        end
        local SnapMove = ShowSHMDElements(EditModePtr)
        if CheckBox("Show SHMD Elements",SnapMove) then
            if SnapMove then
                SnapMove = false
            else
                SnapMove = true
            end 
            SetShowSHMDElements(EditModePtr,SnapMove)
        end
        local brushMode = GetBrushMode(EditModePtr)
        if CheckBox("Brush Mode", brushMode) then
            if brushMode then
                brushMode = false
            else
                brushMode = true
            end
            SetBrushMode(EditModePtr, brushMode)
        end
        
        if brushMode then
            Separator()
            Text("Paint areas to generate, then confirm:")
            if Button("Confirm Generation") then
                ConfirmBrushGeneration(EditModePtr)
            end
            SameLine()
            if Button("Clear Areas") then
                ClearPaintedAreas(EditModePtr)
            end
        end
        
        RenderBrushes(EditModePtr)
    end
end

