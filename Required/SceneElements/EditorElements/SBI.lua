WindowName = "SBI-Editor"
function prepare(ElementPtr)
    MakeNoCollapse(ElementPtr)
    SetWindowSize(ElementPtr, 400, 600)
end

function render(ElementPtr)
    local ret = true
    local ScenePtr = GetCurrentScenePtr(ElementPtr)
    local EditModePtr, EditModeName = GetCurrentEditMode(ScenePtr)

    if EditModeName == "SBI" then
        local BrushSize = GetBrushSize(EditModePtr)
        local changed, BrushSize = DragInt(BrushSize,"Brush Size",1.0,1,20)
        if changed then
            SetBrushSize(EditModePtr, BrushSize)
        end
        
        local PaintMode = GetPaintMode(EditModePtr)
        if CheckBox("Paint Mode",PaintMode) then
            if PaintMode then
                PaintMode = false
            else
                PaintMode = true
            end 
            SetPaintMode(EditModePtr,PaintMode)
        end
        
        local DragMode = GetDragMode(EditModePtr)
        if CheckBox("Drag Mode",DragMode) then
            if DragMode then
                DragMode = false
            else
                DragMode = true
            end 
            SetDragMode(EditModePtr,DragMode)
        end
        
        if DragMode then
            Text("Drag Mode: Click door to select")
            Text("Drag door center to move")
            Text("Drag corners to resize")
        end

        NewLine()
        Text("--- Door Management ---")
        local doorCount = GetDoorCount(EditModePtr)
        Text("Door Count: " .. doorCount)
        
        if Button("Create Door Near Camera") then
            CreateDoorNearCamera(EditModePtr, "Door" .. doorCount)
            SetSelectedDoor(EditModePtr, doorCount)
        end
        
        NewLine()
        Text("--- Selected Door ---")
        local selectedDoor = GetSelectedDoor(EditModePtr)
        if selectedDoor >= 0 then
            Text("Selected Door: " .. selectedDoor)
            
            local doorData = GetDoorData(EditModePtr, selectedDoor)
            if doorData then
                local startX, startY, endX, endY = doorData.startX, doorData.startY, doorData.endX, doorData.endY
                
                Text("Manual Position/Size Edit:")
                local changed1, newStartX = DragFloat(startX, "Start X", 1.0, 0, 2048)
                local changed2, newStartY = DragFloat(startY, "Start Y", 1.0, 0, 2048)
                local changed3, newEndX = DragFloat(endX, "End X", 1.0, 0, 2048)
                local changed4, newEndY = DragFloat(endY, "End Y", 1.0, 0, 2048)
                
                if changed1 or changed2 or changed3 or changed4 then
                    ResizeDoor(EditModePtr, selectedDoor, newStartX, newStartY, newEndX, newEndY)
                end
                
                NewLine()
                if Button("Delete Selected Door") then
                    DeleteDoor(EditModePtr, selectedDoor)
                end
            end
        else
            Text("No Door Selected")
        end
        
        NewLine()
        Text("--- Quick Actions ---")
        if Button("Select Door 0") then
            if doorCount > 0 then
                SetSelectedDoor(EditModePtr, 0)
            end
        end
        
        if Button("Clear All Doors") then
            for i = doorCount - 1, 0, -1 do
                DeleteDoor(EditModePtr, i)
            end
        end
        
        NewLine()
        Text("--- Navigation ---")
        local teleportDoorIndex = GetTeleportDoorIndex(EditModePtr)
        local changed, teleportDoorIndex = DragInt(teleportDoorIndex, "Teleport to Door", 1.0, 0, doorCount - 1)
        if changed then
            SetTeleportDoorIndex(EditModePtr, teleportDoorIndex)
        end
        
        if Button("Teleport") then
            if doorCount > 0 and teleportDoorIndex >= 0 and teleportDoorIndex < doorCount then
                TeleportToDoor(EditModePtr, teleportDoorIndex)
            end
        end
    end
end