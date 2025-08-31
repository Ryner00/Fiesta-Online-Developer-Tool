#pragma once
#include <Scene/ScreenElements/ScreenElement.h>
#include <Scene/ScreenElements/PasteSettings/PasteSettings.h>
#include <ImGui/imgui.h>
#include <Data/IngameWorld/IngameWorld.h>
#include <vector>
#include <functional>

NiSmartPointer(ScatterMode);
class ScatterMode : public ScreenElement
{
    NiDeclareRTTI;

public:
    ScatterMode();
    virtual ~ScatterMode();
    virtual bool Draw() override;
    void Update(float deltaTime);

    void SetWorld(IngameWorldPtr world) { _world = world; }
    void SetSelectedObject(NiPickablePtr obj) { 
        _selectedObjects.clear();
        if (obj) {
            _selectedObjects.push_back(obj);
        }
        _selectedObject = obj;
    }
    void SetSelectedObjects(const std::vector<NiPickablePtr>& objects) { 
        _selectedObjects = objects;
        _selectedObject = objects.empty() ? nullptr : objects[0];
    }
    void SetPasteSettings(PasteSettingsPtr pasteSettings) { _pasteSettings = pasteSettings; }
    void SetClearSelectionCallback(std::function<void()> callback) { _clearSelectionCallback = callback; }
    void Show() { _isVisible = true; }
    void Hide() { _isVisible = false; }
    void Toggle() { 
        _isVisible = !_isVisible; 
        if (_isVisible && _pasteSettings)
            _pasteSettings->Show();
    }
    bool IsVisible() const { return _isVisible; }
    bool IsScatterModeActive() const { return _scatterActive; }
    
    void StartScatterMode();
    void StopScatterMode();
    void ProcessMouseInput(const ImVec2& mousePos);
    void ConfirmScatter();
    void CancelScatter();

    struct Settings {
        float brushSize = 25.0f;
        float density = 0.3f;
        int maxObjects = 50;
        bool enabled = false;
        bool useAreaScatter = true;
        bool drawOnObjects = false;
        bool keepSelection = false;
        bool noOverlap = false;
        float objectRadius = 5.0f;
    } scatterSettings;

private:
    void DrawScatterUI();
    void DrawBrushOverlay();
    void AddPaintPoint(const NiPoint3& worldPos);
    void ScatterObjects();
    bool IsPointInPaintedArea(const NiPoint3& point);
    bool IsPointInUnifiedPaintedArea(const NiPoint3& point);
    bool CheckObjectOverlap(const NiPoint3& newPos, const std::vector<NiPoint3>& placedPositions);
    NiPoint3 ScreenToWorld(const ImVec2& screenPos);
    
    struct PredictionResult {
        int totalAttempts;
        int validPositions;
        int afterDensity;
        int afterOverlap;
        int finalCount;
    };
    PredictionResult CalculatePredictedObjectCount();
    void UpdatePrediction();
    
    bool _isVisible;
    bool _scatterActive;
    bool _hasData;
    IngameWorldPtr _world;
    NiPickablePtr _selectedObject;
    std::vector<NiPickablePtr> _selectedObjects;
    PasteSettingsPtr _pasteSettings;
    
    struct PaintedArea {
        NiPoint3 worldPos;
        float brushSize;
    };
    std::vector<PaintedArea> _paintedPoints;
    ImVec2 _lastMousePos;
    bool _isDrawing;
    std::function<void()> _clearSelectionCallback;
    
    std::vector<NiPickablePtr> _tempHighlightedObjects;
    float _highlightTimer;
    
    PredictionResult _lastPrediction;
    bool _predictionDirty;
    float _predictionUpdateTimer;
};