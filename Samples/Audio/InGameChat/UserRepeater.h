//--------------------------------------------------------------------------------------
// UserRepeater.h
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

#include "ItemRepeater.h"
#include "WICTextureLoader.h"
#include "Effects.h"

class UserListItem
{
public:
    UserListItem(XUserHandle user) : _systemUser(user), _chatUser(nullptr), _hasController(true), _hideGroup(false) { }
    UserListItem(xbox::services::game_chat_2::chat_user* user) : _systemUser(nullptr), _chatUser(user), _hasController(true), _hideGroup(false) { }
    UserListItem(const std::wstring& name) : _systemUser(nullptr), _chatUser(nullptr), _name(name), _hasController(true), _hideGroup(false) { }
    UserListItem(bool hasController = false) : _systemUser(nullptr), _chatUser(nullptr), _hasController(hasController), _hideGroup(false) { }

    enum MicType
    {
        None,
        Mic,
        Kinect
    };

    std::wstring GetName();
    int GetChannel();
    bool IsLocal();
    bool IsMuted();
    bool IsTalking();
    MicType HasMic();

    bool HasController() const { return _hasController; }
    bool HasUser() const { return !(_systemUser == nullptr && _chatUser == nullptr); }
    bool ShowGroup() const { return !_hideGroup; }
    void HideGroup() { _hideGroup = true; }

private:
    XUserHandle _systemUser;
    xbox::services::game_chat_2::chat_user* _chatUser;
    std::wstring _name;
    bool _hasController;
    bool _hideGroup;
};

class UserRepeater : public ItemRepeater<std::shared_ptr<UserListItem>>
{
public:
    UserRepeater(
        std::shared_ptr<ATG::UIManager> mgr,
        POINT origin,
        SIZE itemBounds,
        unsigned idBase
    ) :
        ItemRepeater(mgr, origin, itemBounds, idBase),
        _readonly(false)
    {
    }

    void LoadImages(ID3D12Device* device, DirectX::ResourceUploadBatch& upload, DirectX::DescriptorPile& pile);
    void SetReadOnly(bool readonly) { _readonly = readonly; }

protected:
    void CreateItem(unsigned index, std::shared_ptr<UserListItem> item, RECT& bounds) override;
    void UpdateItem(unsigned index, std::shared_ptr<UserListItem> item) override;

    inline void LoadImage(DirectX::DescriptorPile& pile, const wchar_t* name, unsigned int id)
    {

        auto index = pile.Allocate();
        auto slot = _factory->CreateTexture(name, static_cast<int>(index));

        Microsoft::WRL::ComPtr<ID3D12Resource> tex;
        _factory->GetResource(slot, tex.GetAddressOf());

        _mgr->RegisterImage(
            id,
            pile.GetGpuHandle(index),
            DirectX::GetTextureSize(tex.Get())
            );
    }

private:
    bool _readonly;
    std::unique_ptr<DirectX::EffectTextureFactory> _factory;
};
