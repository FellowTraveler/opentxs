// Copyright (c) 2010-2020 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef OPENTXS_CORE_UNIQUEQUEUE_HPP
#define OPENTXS_CORE_UNIQUEQUEUE_HPP

#include "opentxs/Version.hpp"

#include <deque>
#include <mutex>
#include <set>

namespace opentxs
{
template <class T>
class UniqueQueue
{
public:
    using Key = int;

    void CancelByValue(const T& in) const
    {
        Lock lock(lock_);

        if (0 == set_.count(in)) { return; }

        for (auto i = queue_.cbegin(); i < queue_.cend(); ++i) {
            /* TODO: these lines will cause a segfault in the clang-5 ast
             *  parser.
            const auto & [ key, value ] = *i;
            [[maybe_unused]] const auto& notUsed = key;
            */
            const auto& value = std::get<1>(*i);

            if (value == in) {
                set_.erase(value);
                queue_.erase(i);
                break;
            }
        }

        OT_ASSERT(set_.size() == queue_.size())
    }

    void CancelByKey(const Key& in) const
    {
        Lock lock(lock_);

        for (auto i = queue_.cbegin(); i < queue_.cend(); ++i) {
            /* TODO: this line will cause a segfault in the clang-5 ast parser.
            const auto & [ key, value ] = *i;
            */
            const auto& key = std::get<0>(*i);
            const auto& value = std::get<1>(*i);

            if (key == in) {
                set_.erase(value);
                queue_.erase(i);
                break;
            }
        }

        OT_ASSERT(set_.size() == queue_.size())
    }

    std::map<T, Key> Copy() const
    {
        std::map<T, Key> output{};
        Lock lock(lock_);

        /* TODO: this line will cause a segfault in the clang-5 ast parser.
        for (const auto & [ key, value ] : queue_) {
        */
        for (const auto& it : queue_) {
            const auto& key = it.first;
            const auto& value = it.second;
            output.emplace(value, key);
        }

        return output;
    }

    bool empty() const
    {
        Lock lock(lock_);

        return queue_.empty();
    }

    bool Push(const Key key, const T& in) const
    {
        OT_ASSERT(0 < key)

        Lock lock(lock_);

        if (0 == set_.count(in)) {
            queue_.push_front({key, in});
            set_.emplace(in);

            OT_ASSERT(set_.size() == queue_.size())

            return true;
        }

        return false;
    }

    bool Push(const Lock& lock, const Key key, const T& in) const
    {
        OT_ASSERT(0 < key)

        if (0 == set_.count(in)) {
            queue_.push_front({key, in});
            set_.emplace(in);

            OT_ASSERT(set_.size() == queue_.size())

            return true;
        }

        return false;
    }

    bool Pop(Key& key, T& out) const
    {
        Lock lock(lock_);

        if (0 == queue_.size()) { return false; }

        /* TODO: this line will cause a segfault in the clang-5 ast parser.
        const auto & [ outKey, outValue ] = queue_.back();
        */
        const auto& outKey = queue_.back().first;
        const auto& outValue = queue_.back().second;
        set_.erase(outValue);
        out = outValue;
        key = outKey;
        queue_.pop_back();

        OT_ASSERT(set_.size() == queue_.size())

        return true;
    }

    std::size_t size() const
    {
        Lock lock(lock_);

        return queue_.size();
    }

    UniqueQueue() noexcept
        : lock_()
        , queue_()
        , set_()
    {
    }

public:
    mutable std::mutex lock_;

private:
    mutable std::deque<std::pair<Key, T>> queue_;
    mutable std::set<T> set_;

    UniqueQueue(const UniqueQueue&) = delete;
    UniqueQueue(UniqueQueue&&) = delete;
    UniqueQueue& operator=(const UniqueQueue&) = delete;
    UniqueQueue& operator=(UniqueQueue&&) = delete;
};
}  // namespace opentxs
#endif
