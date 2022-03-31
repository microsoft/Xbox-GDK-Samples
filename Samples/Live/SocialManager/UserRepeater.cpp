//--------------------------------------------------------------------------------------
// UserRepeater.cpp
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "pch.h"
#include "UserRepeater.h"
#include "StringUtil.h"

void UserRepeater::CreateItem(unsigned index, std::shared_ptr<UserListItem> /*item*/, RECT & bounds)
{
    auto base = GetItemId(index);
    auto panel = _mgr->FindPanel<ATG::Overlay>(_panelId);

    // Name
    auto r = bounds;

    r.left += 24;
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
}

void UserRepeater::UpdateItem(unsigned index, std::shared_ptr<UserListItem> item)
{
    auto base = GetItemId(index);

    if (item)
    {
        _mgr->FindControl<ATG::TextLabel>(_panelId, base + 1)->SetText(item->GetName().c_str());
        _mgr->FindControl<ATG::TextLabel>(_panelId, base + 2)->SetText(item->GetStatus(_titleId).c_str());
    }
    else
    {
        _mgr->FindControl<ATG::TextLabel>(_panelId, base + 1)->SetText(L"");
        _mgr->FindControl<ATG::TextLabel>(_panelId, base + 2)->SetText(L"");
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
