// Copyright (c) 2010-2020 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

// IWYU pragma: private
// IWYU pragma: friend ".*src/core/contract/basket/BasketContract.cpp"

#pragma once

#include <cstdint>
#include <string>

#include "core/contract/UnitDefinition.hpp"
#include "opentxs/Proto.hpp"
#include "opentxs/Types.hpp"
#include "opentxs/core/Identifier.hpp"
#include "opentxs/core/contract/basket/BasketContract.hpp"
#include "opentxs/protobuf/ContactEnums.pb.h"
#include "opentxs/protobuf/ContractEnums.pb.h"
#include "opentxs/protobuf/UnitDefinition.pb.h"

namespace opentxs
{
namespace api
{
namespace internal
{
struct Core;
}  // namespace internal
}  // namespace api

class PasswordPrompt;
}  // namespace opentxs

namespace opentxs::contract::unit::implementation
{
class Basket final : public unit::Basket, public contract::implementation::Unit
{
public:
    auto BasketID() const -> OTIdentifier final;
    auto Currencies() const -> const Subcontracts& final
    {
        return subcontracts_;
    }
    auto Type() const -> proto::UnitType final
    {
        return proto::UNITTYPE_BASKET;
    }
    auto Weight() const -> const Amount& final { return weight_; }

    Basket(
        const api::internal::Core& api,
        const Nym_p& nym,
        const std::string& shortname,
        const std::string& name,
        const std::string& symbol,
        const std::string& terms,
        const Amount& weight,
        const proto::ContactItemType unitOfAccount,
        const VersionNumber version);
    Basket(
        const api::internal::Core& api,
        const Nym_p& nym,
        const proto::UnitDefinition serialized);

    ~Basket() final = default;

private:
    friend unit::Basket;

    Subcontracts subcontracts_;
    OTAmount weight_;

    auto BasketIDVersion(const Lock& lock) const -> proto::UnitDefinition;
    auto clone() const noexcept -> Basket* final { return new Basket(*this); }
    auto IDVersion(const Lock& lock) const -> proto::UnitDefinition final;

    Basket(const Basket&);
    Basket(Basket&&) = delete;
    auto operator=(const Basket&) -> Basket& = delete;
    auto operator=(Basket&&) -> Basket& = delete;
};
}  // namespace opentxs::contract::unit::implementation
