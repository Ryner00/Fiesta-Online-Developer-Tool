WindowName = "SHBD-Editor"
function prepare(ElementPtr)
    MakeNoCollapse(ElementPtr)
end

function render(ElementPtr)
    local ret = true
    local ScenePtr = GetCurrentScenePtr(ElementPtr)
    local EditModePtr, EditModeName = GetCurrentEditMode(ScenePtr)

    if EditModeName == "SHBD" then
        local BrushSize = GetBrushSize(EditModePtr)
        local changed, BrushSize = DragInt(BrushSize,"Brush Size",1.0,0,100)
        if changed then
            SetBrushSize(EditModePtr, BrushSize)
        end
        local Walkable = GetWalkable(EditModePtr)
        if CheckBox("Switch to Walkable",Walkable) then
            if Walkable then
                Walkable = false
            else
                Walkable = true
            end 
            SetWalkable(EditModePtr,Walkable)
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

        local CaptureHeight = GetCaptureHeight(EditModePtr)
        local changed1, CaptureHeight = DragFloat(CaptureHeight, "Capture Height", 1.0, -200.0, 500.0)
        if changed1 then
            SetCaptureHeight(EditModePtr, CaptureHeight)
        end
        
        local CaptureTolerance = GetCaptureTolerance(EditModePtr)
        local changed2, CaptureTolerance = DragFloat(CaptureTolerance, "Capture Tolerance", 0.1, 1.0, 50.0)
        if changed2 then
            SetCaptureTolerance(EditModePtr, CaptureTolerance)
        end
        
        local CurrentHeight = GetCurrentSHBDHeight(EditModePtr)
        Text("Current SHBD Height: " .. string.format("%.1f", CurrentHeight))
        
        local KeepExisting = GetKeepExistingSHBD(EditModePtr)
        if CheckBox("Keep Existing SHBD", KeepExisting) then
            if KeepExisting then
                KeepExisting = false
            else
                KeepExisting = true
            end 
            SetKeepExistingSHBD(EditModePtr, KeepExisting)
        end
        
        local Accuracy = GetGenerationAccuracy(EditModePtr)
        local changed3, Accuracy = DragInt(Accuracy, "Generation Speed", 1, 1, 7)
        if changed3 then
            SetGenerationAccuracy(EditModePtr, Accuracy)
        end
        Text("1=Fastest (less accurate), 7=Slowest (most accurate)")
        
        if Button("Auto generate SHBD (Objects)") then
            AutoGenerateSHBD(EditModePtr)
        end
        
        if Button("Auto generate SHBD (Terrain)") then
            AutoGenerateTerrainSHBD(EditModePtr)
        end
        
        if Button("Reset SHBD (Temporary)") then
            ResetSHBD(EditModePtr)
        end
    end
end

