//--------------------------------------------------------------------------------------
// FrontPanelManager.h
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------
#pragma once

#include "FrontPanel/FrontPanelDisplay.h"
#include "FrontPanel/FrontPanelInput.h"

#include <functional>
#include <array>

#include "StepTimer.h"
#include "PanelScreen.h"

class FrontPanelManager
{
public:
    FrontPanelManager();

    void CreateScreens();

    template<typename child_t, typename... args>
    child_t &CreateChild(args... params)
    {
        auto child = new child_t( this, params... );
        AddChild(child);
        return *child;
    }

    void Update(DX::StepTimer const& timer);
    void Navigate(const PanelScreen &navToChild);

    struct ActionRecord
    {
        unsigned id;
        bool operator==(const ActionRecord &other) const
        {
            return id == other.id;
        }

        std::wstring name;
        std::wstring description;
    };

    static size_t GetIndexForButtonId(XFrontPanelButton buttonId);

    size_t ButtonActionCount() const;
    const ActionRecord &operator[] (size_t idx) const;
    const ActionRecord &CreateButtonAction(const wchar_t *name, const wchar_t *description, std::function<void()> op);
    
    bool IsActionAssigned(XFrontPanelButton buttonId) const;
    const ActionRecord &GetActionAssignment(XFrontPanelButton buttonId) const;
    void AssignActionToButton(const ActionRecord &action, XFrontPanelButton buttonId);
    void ClearActionAssignment(XFrontPanelButton buttonId);
    XFrontPanelLight GetAssignedLights() const;
    void SetAssignedLigts() const;

    void CreateDeviceDependentResources(DX::DeviceResources *deviceResources);
    void CreateWindowSizeDependentResources(DX::DeviceResources *deviceResources);
    void GPURender(DX::DeviceResources *deviceResources);

    bool IsAvailable() const
    {
        return m_available;
    }
    
private:
    void AddChild(PanelScreen *child);
    bool OnButtonPressed(XFrontPanelButton whichButton);
    bool InvokeButtonAction(int buttonId);
    
    std::vector<std::unique_ptr<PanelScreen>>       m_children;
    PanelScreen                                    *m_currentPanelScreen;
    
    struct ButtonAction : public ActionRecord
    {
        std::function<void()> Invoke;
    };
    
    std::vector<std::unique_ptr<ButtonAction>>      m_buttonActions;
    std::array<const ButtonAction *, 5>             m_buttonActionAssignments;

    // FrontPanel objects
    std::unique_ptr<ATG::FrontPanelDisplay>         m_frontPanelDisplay;
    std::unique_ptr<ATG::FrontPanelInput>           m_frontPanelInput;
    ATG::FrontPanelInput::ButtonStateTracker        m_frontPanelInputButtons;
    bool                                            m_available;
};