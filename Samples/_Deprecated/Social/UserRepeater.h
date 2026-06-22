//--------------------------------------------------------------------------------------
// UserRepeater.h
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------
#pragma once

#include "ItemRepeater.h"

class UserListItem
{
public:
    UserListItem() { }
    UserListItem(const XblSocialManagerUser* user) : _socialUser(user) { }

    std::wstring GetName();
    std::wstring GetStatus(uint32_t titleId);
    uint64_t GetXuid();

private:
    const XblSocialManagerUser* _socialUser;
};

class UserRepeater : public ItemRepeater<std::shared_ptr<UserListItem>>
{
public:
    UserRepeater(
        std::shared_ptr<ATG::UIManager> mgr,
        POINT origin,
        SIZE itemBounds,
        unsigned idBase,
        uint32_t titleId
    ) :
        ItemRepeater(mgr, origin, itemBounds, idBase),
        _readonly(false), _titleId(titleId)
    {
    }

    void SetReadOnly(bool readonly) { _readonly = readonly; }

protected:
    void CreateItem(unsigned index, std::shared_ptr<UserListItem> item, RECT& bounds) override;
    void UpdateItem(unsigned index, std::shared_ptr<UserListItem> item) override;

private:
    bool _readonly;
    uint32_t _titleId;
};
