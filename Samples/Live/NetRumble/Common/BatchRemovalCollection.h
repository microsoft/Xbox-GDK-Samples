//--------------------------------------------------------------------------------------
// BatchRemovalCollection.h
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

template<class T>
class BatchRemovalCollection
{
public:
    using container = std::vector<T>;
    using value_type = typename container::value_type;
    using reference = typename container::reference;
    using const_reference = typename container::const_reference;
    using iterator = typename container::iterator;
    using const_iterator = typename container::const_iterator;
    using size_type = typename container::size_type;

    inline void QueuePendingRemoval(const value_type &item)
    {
        pendingRemovals.push_back(item);
    }

    void ApplyPendingRemovals()
    {
        activeObjects.insert(activeObjects.end(), pendingAdds.begin(), pendingAdds.end());
        pendingAdds.clear();

        for (size_t i = 0; i < pendingRemovals.size(); i++)
        {
            auto itr = std::find(std::begin(activeObjects), std::end(activeObjects), pendingRemovals[i]);
            activeObjects.erase(itr);
        }
        pendingRemovals.clear();
    }

    inline void clear()
    {
        pendingAdds.clear();
        pendingRemovals.clear();
        activeObjects.clear();
    }

    inline void push_back(const value_type &item)
    {
        pendingAdds.push_back(item);
    }

    inline iterator begin()
    {
        return activeObjects.begin();
    }

    inline const_iterator cbegin() const
    {
        return activeObjects.cbegin();
    }

    inline iterator end()
    {
        return activeObjects.end();
    }

    inline const_iterator cend() const
    {
        return activeObjects.cend();
    }

    inline size_type size() const
    {
        return activeObjects.size();
    }

    inline size_type total_size() const
    {
        return activeObjects.size() + pendingAdds.size();
    }

    inline reference operator[](size_type index)
    {
        return activeObjects[index];
    }

    inline const_reference operator[](size_type index) const
    {
        return activeObjects[index];
    }

    inline const_iterator erase(const_iterator itr)
    {
        return activeObjects.erase(itr);
    }

private:
    container pendingAdds;
    container pendingRemovals;
    container activeObjects;
};