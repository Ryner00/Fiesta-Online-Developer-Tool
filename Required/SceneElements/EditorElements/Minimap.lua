WindowName = "Minimap"
function prepare(ElementPtr)
    SetWindowSize(ElementPtr, 600, 650)
    SetWindowPos(ElementPtr, 200, 50)
    SetWindowVisible(ElementPtr, false)
end

function render(ElementPtr)
    local ScenePtr = GetCurrentScenePtr(ElementPtr)
    local EditModePtr, EditModeName = GetCurrentEditMode(ScenePtr)
    
    if not EditModePtr then
        return true
    end
    
    Text("Minimap - Press M to toggle")
    Text("Mode: " .. EditModeName)
    
    if Button("Close") then
        SetWindowVisible(ElementPtr, false)
    end
    
    SameLine()
    if Button("Reset Zoom") then
        ResetMinimapZoom(EditModePtr)
    end
    
    NewLine()
    Text("Left click: Teleport camera")
    if EditModeName == "SBI" then
        Text("Shift+Click: Select/drag door")
        Text("Ctrl+Click: Create door")
    end
    
    NewLine()
    RenderMinimap(EditModePtr)
    
    return true
end