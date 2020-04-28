// Copyright (c) 2010-2020 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef OPENTXS_UI_ACCOUNTSUMMARYITEM_HPP
#define OPENTXS_UI_ACCOUNTSUMMARYITEM_HPP

#include "opentxs/Forward.hpp"  // IWYU pragma: associated

#include <string>

#include "ListRow.hpp"
#include "opentxs/SharedPimpl.hpp"

#ifdef SWIG
// clang-format off
%template(OTUIAccountSummaryItem) opentxs::SharedPimpl<opentxs::ui::AccountSummaryItem>;
%rename(UIAccountSummaryItem) opentxs::ui::AccountSummaryItem;
// clang-format on
#endif  // SWIG

namespace opentxs
{
namespace ui
{
class AccountSummaryItem;
}  // namespace ui

using OTUIAccountSummaryItem = SharedPimpl<ui::AccountSummaryItem>;
}  // namespace opentxs

namespace opentxs
{
namespace ui
{
class AccountSummaryItem : virtual public ListRow
{
public:
    OPENTXS_EXPORT virtual std::string AccountID() const noexcept = 0;
    OPENTXS_EXPORT virtual std::string Balance() const noexcept = 0;
    OPENTXS_EXPORT virtual std::string DisplayBalance() const noexcept = 0;
    OPENTXS_EXPORT virtual std::string Name() const noexcept = 0;

    OPENTXS_EXPORT ~AccountSummaryItem() override = default;

protected:
    AccountSummaryItem() noexcept = default;

private:
    AccountSummaryItem(const AccountSummaryItem&) = delete;
    AccountSummaryItem(AccountSummaryItem&&) = delete;
    AccountSummaryItem& operator=(const AccountSummaryItem&) = delete;
    AccountSummaryItem& operator=(AccountSummaryItem&&) = delete;
};
}  // namespace ui
}  // namespace opentxs
#endif
