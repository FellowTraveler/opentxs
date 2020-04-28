// Copyright (c) 2010-2020 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef OPENTXS_UI_ACCOUNTLISTITEM_HPP
#define OPENTXS_UI_ACCOUNTLISTITEM_HPP

#include "opentxs/Forward.hpp"  // IWYU pragma: associated

#include <string>

#include "ListRow.hpp"
#include "opentxs/SharedPimpl.hpp"

#ifdef SWIG
// clang-format off
%extend opentxs::ui::AccountListItem {
    int Unit() const
    {
        return static_cast<int>($self->Unit());
    }
}
%ignore opentxs::ui::AccountListItem::Unit;
%template(OTUIAccountListItem) opentxs::SharedPimpl<opentxs::ui::AccountListItem>;
%rename(UIAccountListItem) opentxs::ui::AccountListItem;
// clang-format on
#endif  // SWIG

namespace opentxs
{
namespace ui
{
class AccountListItem;
}  // namespace ui

using OTUIAccountListItem = SharedPimpl<ui::AccountListItem>;
}  // namespace opentxs

namespace opentxs
{
namespace ui
{
class AccountListItem : virtual public ListRow
{
public:
    OPENTXS_EXPORT virtual std::string AccountID() const noexcept = 0;
    OPENTXS_EXPORT virtual std::string Balance() const noexcept = 0;
    OPENTXS_EXPORT virtual std::string ContractID() const noexcept = 0;
    OPENTXS_EXPORT virtual std::string DisplayBalance() const noexcept = 0;
    OPENTXS_EXPORT virtual std::string DisplayUnit() const noexcept = 0;
    OPENTXS_EXPORT virtual std::string Name() const noexcept = 0;
    OPENTXS_EXPORT virtual std::string NotaryID() const noexcept = 0;
    OPENTXS_EXPORT virtual std::string NotaryName() const noexcept = 0;
    OPENTXS_EXPORT virtual AccountType Type() const noexcept = 0;
    OPENTXS_EXPORT virtual proto::ContactItemType Unit() const noexcept = 0;

    OPENTXS_EXPORT ~AccountListItem() override = default;

protected:
    AccountListItem() noexcept = default;

private:
    AccountListItem(const AccountListItem&) = delete;
    AccountListItem(AccountListItem&&) = delete;
    AccountListItem& operator=(const AccountListItem&) = delete;
    AccountListItem& operator=(AccountListItem&&) = delete;
};
}  // namespace ui
}  // namespace opentxs
#endif
