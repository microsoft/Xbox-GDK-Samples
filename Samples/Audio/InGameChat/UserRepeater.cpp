//--------------------------------------------------------------------------------------
// UserRepeater.cpp
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "pch.h"
#include "InGameChat.h"
#include "UserRepeater.h"
#include "Effects.h"

using namespace xbox::services::game_chat_2;

void UserRepeater::LoadImages(ID3D12Device* device, DirectX::ResourceUploadBatch& upload, DirectX::DescriptorPile& pile)
{
    _factory = std::make_unique<DirectX::EffectTextureFactory>(device, upload, pile.Heap());

    LoadImage(pile, L".//Assets//particpantbg.png", 9000);
    LoadImage(pile, L".//Assets//controller1.png", 9001);
    LoadImage(pile, L".//Assets//controller2.png", 9002);
    LoadImage(pile, L".//Assets//controller3.png", 9003);
    LoadImage(pile, L".//Assets//controller4.png", 9004);
    LoadImage(pile, L".//Assets//controller5.png", 9005);
    LoadImage(pile, L".//Assets//controller6.png", 9006);
    LoadImage(pile, L".//Assets//controller7.png", 9007);
    LoadImage(pile, L".//Assets//controller8.png", 9008);
    LoadImage(pile, L".//Assets//mic.png", 9009);
    LoadImage(pile, L".//Assets//debugdisabled.png", 9010);
    LoadImage(pile, L".//Assets//particpantmutedbg.png", 9011);
    LoadImage(pile, L".//Assets/spacer.png", 9012);
    LoadImage(pile, L".//Assets/talking.png", 9013);
    LoadImage(pile, L".//Assets//mic_Mute.png", 9014);
}

void UserRepeater::CreateItem(unsigned index, std::shared_ptr<UserListItem>, RECT & bounds)
{
    auto base = GetItemId(index);
    auto panel = _mgr->FindPanel<ATG::Overlay>(_panelId);

    // Background image
    panel->Add(new ATG::Image(base + 1, 9011, bounds));

    // Controller image
    auto r = bounds;

    r.left = bounds.right - 51 - 5;
    r.top = bounds.top + 5;
    r.right = bounds.right - 5;
    r.bottom = r.top + 64;

    auto img = new ATG::Image(base + 2, 9001, r);

    img->SetVisible(false);

    panel->Add(img);

    // Text area
    r = bounds;

    r.left += 84;
    r.right -= 64;
    r.top += 20;
    r.bottom -= 10;

    auto label = new ATG::TextLabel(base + 3, L"", r, ATG::TextLabel::c_StyleFontSmall | ATG::TextLabel::c_StyleTransparent);

    label->SetBackgroundColor(DirectX::Colors::Transparent);
    panel->Add(label);

    // Mic image
    r = bounds;

    r.left += 24;
    r.right = r.left + 27;
    r.top += 20;
    r.bottom = r.top + 40;

    img = new ATG::Image(base + 4, 9009, r);

    img->SetVisible(false);

    panel->Add(img);

    // Muted Mic image
    r = bounds;

    r.left += 24;
    r.right = r.left + 35;
    r.top += 20;
    r.bottom = r.top + 40;

    img = new ATG::Image(base + 5, 9014, r);

    img->SetVisible(false);

    panel->Add(img);

    // Squaker
    r = bounds;

    r.left -= 31;
    r.right = r.left + 26;
    r.top += 19;
    r.bottom = r.top + 45;

    img = new ATG::Image(base + 6, 9013, r);

    img->SetVisible(false);

    panel->Add(img);
}

void UserRepeater::UpdateItem(unsigned index, std::shared_ptr<UserListItem> item)
{
    auto base = GetItemId(index);

    if (!item->HasController())
    {
        // Clear the text, hide the images and set the background image
        _mgr->FindControl<ATG::Image>(_panelId, base + 1)->SetImageId(9011);

        _mgr->FindControl<ATG::TextLabel>(_panelId, base + 3)->SetText(L"");

        _mgr->FindControl<ATG::Image>(_panelId, base + 2)->SetVisible(false);
        _mgr->FindControl<ATG::Image>(_panelId, base + 4)->SetVisible(false);
        _mgr->FindControl<ATG::Image>(_panelId, base + 5)->SetVisible(false);
        _mgr->FindControl<ATG::Image>(_panelId, base + 6)->SetVisible(false);
        _mgr->FindControl<ATG::Button>(_panelId, base)->SetEnabled(false);
    }
    else
    {
        _mgr->FindControl<ATG::Button>(_panelId, base)->SetEnabled(!_readonly);
        _mgr->FindControl<ATG::Image>(_panelId, base + 1)->SetImageId(9000);
        _mgr->FindControl<ATG::Image>(_panelId, base + 2)->SetImageId(9001 + unsigned(item->GetChannel() - 1));
        _mgr->FindControl<ATG::Image>(_panelId, base + 2)->SetVisible(true);

        if (item->HasUser() || item->GetName().empty() != true)
        {
            _mgr->FindControl<ATG::TextLabel>(_panelId, base + 3)->SetText(item->GetName().c_str());
            _mgr->FindControl<ATG::Image>(_panelId, base + 2)->SetVisible(true);
        }
        else
        {
            _mgr->FindControl<ATG::TextLabel>(_panelId, base + 3)->SetText(L"Press Y on any gamepad to join");
            _mgr->FindControl<ATG::Image>(_panelId, base + 2)->SetVisible(false);
        }

        _mgr->FindControl<ATG::Image>(_panelId, base + 4)->SetVisible(false);
        _mgr->FindControl<ATG::Image>(_panelId, base + 5)->SetVisible(false);
        _mgr->FindControl<ATG::Image>(_panelId, base + 6)->SetVisible(false);

        if (item->HasMic() == UserListItem::MicType::Mic)
        {
            if (item->IsMuted())
            {
                _mgr->FindControl<ATG::Image>(_panelId, base + 5)->SetVisible(true);
            }
            else
            {
                _mgr->FindControl<ATG::Image>(_panelId, base + 4)->SetVisible(true);
            }
        }

        if (item->IsTalking())
        {
            _mgr->FindControl<ATG::Image>(_panelId, base + 6)->SetVisible(true);
        }
        else
        {
            _mgr->FindControl<ATG::Image>(_panelId, base + 6)->SetVisible(false);
        }

        if (!item->ShowGroup())
        {
            _mgr->FindControl<ATG::Image>(_panelId, base + 2)->SetVisible(false);
        }
    }
}

std::wstring UserListItem::GetName()
{
    if (_systemUser != nullptr)
    {
        char gamertag[XUserGamertagComponentClassicMaxBytes] = {};

        auto hr = XUserGetGamertag(
            _systemUser,
            XUserGamertagComponent::Classic, XUserGamertagComponentClassicMaxBytes,
            gamertag,
            nullptr
            );

        if (SUCCEEDED(hr))
        {
            return DX::Utf8ToWide(gamertag);
        }

        return L"GAMERTAG_ERROR";
    }
    else if (_chatUser != nullptr)
    {
        return Sample::Instance()->GetLiveManager()->GetGamertagForXuid(std::stoull(_chatUser->xbox_user_id()));
    }
    else if (_name.empty() != true)
    {
        return _name;
    }

    return std::wstring();
}

int UserListItem::GetChannel()
{
    if (_chatUser != nullptr)
    {
        return Sample::Instance()->GetChatManager()->GetChannelForUser(std::stoull(_chatUser->xbox_user_id()));
    }

    return 0;
}

bool UserListItem::IsLocal()
{
    return false;
}

bool UserListItem::IsMuted()
{
    if (_chatUser != nullptr)
    {
        return _chatUser->chat_indicator() == game_chat_user_chat_indicator::incoming_communications_muted
            || _chatUser->chat_indicator() == game_chat_user_chat_indicator::local_microphone_muted;
    }

    return false;
}

bool UserListItem::IsTalking()
{
    if (_chatUser != nullptr)
    {
        return _chatUser->chat_indicator() == game_chat_user_chat_indicator::talking;
    }

    return false;
}

UserListItem::MicType UserListItem::HasMic()
{
    if (_systemUser)
    {
        auto user = Sample::Instance()->GetUserManager()->GetUserByHandle(_systemUser);

        if (user != nullptr)
        {
            return user->HasMic ? MicType::Mic : MicType::None;
        }

        return MicType::Mic;
    }
    else if (_chatUser)
    {
        return MicType::Mic;
    }

    return MicType::None;
}
