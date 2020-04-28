// Copyright (c) 2010-2020 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef OPENTXS_UI_BALANCEITEM_HPP
#define OPENTXS_UI_BALANCEITEM_HPP

#include "opentxs/Forward.hpp"  // IWYU pragma: associated

#include <chrono>
#include <cstdint>
#include <list>
#include <string>

#include "ListRow.hpp"
#include "opentxs/SharedPimpl.hpp"
#include "opentxs/Types.hpp"

#ifdef SWIG
// clang-format off
%extend opentxs::ui::BalanceItem {
    int Timestamp() const noexcept
    {
        return Clock::to_time_t($self->Timestamp());
    }
}
%ignore opentxs::ui::BalanceItem::Timestamp;
%ignore opentxs::ui::BalanceItem::Update;
%template(ListOfContactIDs) std::vector<std::string>;
%template(OTUIBalanceItem) opentxs::SharedPimpl<opentxs::ui::BalanceItem>;
%rename(UIBalanceItem) opentxs::ui::BalanceItem;
// clang-format on
#endif  // SWIG

namespace opentxs
{
namespace ui
{
class BalanceItem;
}  // namespace ui

using OTUIBalanceItem = SharedPimpl<ui::BalanceItem>;
}  // namespace opentxs

namespace opentxs
{
namespace ui
{
class BalanceItem : virtual public ListRow
{
public:
    OPENTXS_EXPORT virtual opentxs::Amount Amount() const noexcept = 0;
//    OPENTXS_EXPORT virtual std::string Amount() const noexcept = 0;
    OPENTXS_EXPORT virtual std::vector<std::string> Contacts()
        const noexcept = 0;
    OPENTXS_EXPORT virtual std::string DisplayAmount() const noexcept = 0;
    OPENTXS_EXPORT virtual std::string Memo() const noexcept = 0;
    OPENTXS_EXPORT virtual std::string Workflow() const noexcept = 0;
    OPENTXS_EXPORT virtual std::string Text() const noexcept = 0;
    OPENTXS_EXPORT virtual Time Timestamp() const noexcept = 0;
    OPENTXS_EXPORT virtual StorageBox Type() const noexcept = 0;
    OPENTXS_EXPORT virtual std::string UUID() const noexcept = 0;

    OPENTXS_EXPORT ~BalanceItem() override = default;

protected:
    BalanceItem() noexcept = default;

private:
    BalanceItem(const BalanceItem&) = delete;
    BalanceItem(BalanceItem&&) = delete;
    BalanceItem& operator=(const BalanceItem&) = delete;
    BalanceItem& operator=(BalanceItem&&) = delete;
};
}  // namespace ui
}  // namespace opentxs
#endif
