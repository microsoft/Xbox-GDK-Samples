//--------------------------------------------------------------------------------------
// UserRepeater.cpp
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "pch.h"
#include "UserRepeater.h"
#include "StringUtil.h"

using namespace DirectX;

void UserRepeater::InitializeProfilePics(size_t count, DX::DeviceResources* deviceResources, DirectX::DescriptorPile* pile)
{
    auto device = deviceResources->GetD3DDevice();

    RenderTargetState rtState(
        deviceResources->GetBackBufferFormat(),
        deviceResources->GetDepthBufferFormat());

    ResourceUploadBatch resourceUpload(device);
    resourceUpload.Begin();

    for (size_t i = 0; i < count; i++)
    {
        auto newDisplay = std::make_shared<FriendGamerPicDisplay>();
        newDisplay->RestoreDevice(*pile);

        _friendGamerPicDisplays.push_back(newDisplay);
    }
    auto uploadResourcesFinished = resourceUpload.End(deviceResources->GetCommandQueue());
    uploadResourcesFinished.wait();
}

void UserRepeater::UpdateProfilePics(ID3D12Device* device, ID3D12CommandQueue* commandQueue)
{
    for (auto pic : _friendGamerPicDisplays)
    {
        pic->Update(device, commandQueue);
    }
}

void UserRepeater::ReleaseDevice()
{
    for (auto pic : _friendGamerPicDisplays)
    {
        pic->ReleaseDevice();
    }
}

void UserRepeater::RestoreDevice(DirectX::DescriptorPile& pile)
{
    for (auto pic : _friendGamerPicDisplays)
    {
        pic->RestoreDevice(pile);
    }
}

void UserRepeater::CreateItem(unsigned index, std::shared_ptr<UserListItem> /*item*/, RECT& bounds)
{
    auto base = GetItemId(index);
    auto panel = _mgr->FindPanel<ATG::Overlay>(_panelId);
    auto dim = bounds.bottom - bounds.top;

    // Name
    auto r = bounds;

    r.left += dim + 24;
    r.right = r.left + 400;
    r.top += 5;
    r.bottom = r.top + 40;

    auto label = new ATG::TextLabel(base + 1, L"", r, ATG::TextLabel::c_StyleFontSmall);
    label->SetBackgroundColor(DirectX::Colors::Transparent);
    label->SetForegroundColor(DirectX::Colors::White);
    panel->Add(label);

    // Status
    r = bounds;

    r.left += 500;
    r.right = r.left + 90;
    r.top += 5;
    r.bottom = r.top + 40;

    label = new ATG::TextLabel(base + 2, L"", r, ATG::TextLabel::c_StyleFontSmall);
    label->SetBackgroundColor(DirectX::Colors::Transparent);
    label->SetForegroundColor(DirectX::Colors::White);
    panel->Add(label);

    // Image
    RECT imgRect = { bounds.left, bounds.top, bounds.left + dim, bounds.top + dim };
    auto image = new ATG::Image(base + 3, base, imgRect);
    image->SetVisible(false);
    panel->Add(image);
}

void UserRepeater::UpdateItem(unsigned index, std::shared_ptr<UserListItem> item)
{
    auto base = GetItemId(index);
    auto picDisplay = _friendGamerPicDisplays.size() > 0 ? _friendGamerPicDisplays[index] : nullptr;

    if (item)
    {
        _mgr->FindControl<ATG::TextLabel>(_panelId, base + 1)->SetText(item->GetName().c_str());
        _mgr->FindControl<ATG::TextLabel>(_panelId, base + 2)->SetText(item->GetStatus(_titleId).c_str());
        _mgr->FindControl<ATG::Image>(_panelId, base + 3)->SetImageId(static_cast<unsigned int>(item->GetXuid()));

        if (!picDisplay->IsInitialized())
        {
            if (picDisplay->CheckGamerPicCache(_mgr.get(), _profilePicCache))
            {
                _mgr->FindControl<ATG::Image>(_panelId, base + 3)->SetVisible(true);
                picDisplay->SetInitlized();
            }
        }
    }
    else
    {
        _mgr->FindControl<ATG::TextLabel>(_panelId, base + 1)->SetText(L"");
        _mgr->FindControl<ATG::TextLabel>(_panelId, base + 2)->SetText(L"");
        _mgr->FindControl<ATG::Image>(_panelId, base + 3)->SetVisible(false);
        if(picDisplay)
            picDisplay->SetInitlized(false);
    }
}

std::wstring UserListItem::GetName()
{
    if (!_socialUser)
        return L"";

    return DX::Utf8ToWide(_socialUser->displayName);
}

std::wstring UserListItem::GetStatus(uint32_t titleId)
{
    if (!_socialUser)
        return L"";

    if(XblSocialManagerPresenceRecordIsUserPlayingTitle(&_socialUser->presenceRecord, titleId))
        return L"In title";

    switch (_socialUser->presenceRecord.userState)
    {
        case XblPresenceUserState::Away:    return L"Away";
        case XblPresenceUserState::Offline: return L"Offline";
        case XblPresenceUserState::Online:  return L"Online";
        case XblPresenceUserState::Unknown: return L"Unknown";
        default: return L"";
    }
}

uint64_t UserListItem::GetXuid()
{
    if (!_socialUser)
        return 0;

    return _socialUser->xboxUserId;
}
