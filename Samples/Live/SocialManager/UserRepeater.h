//--------------------------------------------------------------------------------------
// UserRepeater.h
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include <map>

#include "DeviceResources.h"
#include "ItemRepeater.h"
#include "FriendGamerPicDisplay.h"

class UserListItem
{
public:
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
        uint32_t titleId,
        std::map<uint64_t, std::pair<uint8_t*, size_t>>* profilePicCache
    ) :
        ItemRepeater(mgr, origin, itemBounds, idBase),
        _readonly(false), _titleId(titleId),
        _profilePicCache(profilePicCache)
    {
        _friendGamerPicDisplays = std::vector<std::shared_ptr<FriendGamerPicDisplay>>(0);
    }

    void ReleaseDevice();
    void RestoreDevice(DirectX::DescriptorPile& m_resourceDescriptors);
    void SetReadOnly(bool readonly) { _readonly = readonly; }
    void SetLiveContext(XblContextHandle handle) { _contextHandle = handle; }
    void SetPicDisplayXuid(unsigned index, uint64_t xuid) { _friendGamerPicDisplays[index]->SetXuid(xuid); };

    void InitializeProfilePics(size_t count, DX::DeviceResources* deviceResources, DirectX::DescriptorPile* pile);
    void UpdateProfilePics(ID3D12Device* device, ID3D12CommandQueue* commandQueue);

protected:
    void CreateItem(unsigned index, std::shared_ptr<UserListItem> item, RECT& bounds) override;
    void UpdateItem(unsigned index, std::shared_ptr<UserListItem> item) override;

private:
    bool _readonly;
    uint32_t _titleId;
    std::vector<std::shared_ptr<FriendGamerPicDisplay>> _friendGamerPicDisplays;
    XblContextHandle _contextHandle = nullptr;
    std::map<uint64_t, std::pair<uint8_t*, size_t>>* _profilePicCache;
};
