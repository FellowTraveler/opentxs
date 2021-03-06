// Copyright (c) 2010-2021 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef OPENTXS_UI_CONTACTSUBSECTION_HPP
#define OPENTXS_UI_CONTACTSUBSECTION_HPP

// IWYU pragma: no_include "opentxs/Proto.hpp"

#include "opentxs/Version.hpp"  // IWYU pragma: associated

#include <string>

#include "opentxs/Proto.hpp"
#include "opentxs/SharedPimpl.hpp"
#include "opentxs/ui/List.hpp"
#include "opentxs/ui/ListRow.hpp"

#ifdef SWIG
// clang-format off
%extend opentxs::ui::ContactSubsection {
    int Type() const
    {
        return static_cast<int>($self->Type());
    }
}
%ignore opentxs::ui::ContactSubsection::Type;
%ignore opentxs::ui::ContactSubsection::Update;
%template(OTUIContactSubsection) opentxs::SharedPimpl<opentxs::ui::ContactSubsection>;
%rename(UIContactSubsection) opentxs::ui::ContactSubsection;
// clang-format on
#endif  // SWIG

namespace opentxs
{
namespace ui
{
class ContactSubsection;
}  // namespace ui

using OTUIContactSubsection = SharedPimpl<ui::ContactSubsection>;
}  // namespace opentxs

namespace opentxs
{
namespace ui
{
class ContactSubsection : virtual public List, virtual public ListRow
{
public:
    OPENTXS_EXPORT virtual std::string Name(
        const std::string& lang) const noexcept = 0;
    OPENTXS_EXPORT virtual opentxs::SharedPimpl<opentxs::ui::ContactItem>
    First() const noexcept = 0;
    OPENTXS_EXPORT virtual opentxs::SharedPimpl<opentxs::ui::ContactItem> Next()
        const noexcept = 0;
    OPENTXS_EXPORT virtual contact::ContactItemType Type() const noexcept = 0;

    OPENTXS_EXPORT ~ContactSubsection() override = default;

protected:
    ContactSubsection() noexcept = default;

private:
    ContactSubsection(const ContactSubsection&) = delete;
    ContactSubsection(ContactSubsection&&) = delete;
    ContactSubsection& operator=(const ContactSubsection&) = delete;
    ContactSubsection& operator=(ContactSubsection&&) = delete;
};
}  // namespace ui
}  // namespace opentxs
#endif
