// Copyright (c) 2010-2020 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

// IWYU pragma: private
// IWYU pragma: friend ".*src/core/contract/peer/BailmentNotice.cpp"

#pragma once

#include <string>

#include "core/contract/peer/PeerRequest.hpp"
#include "opentxs/Types.hpp"
#include "opentxs/core/Identifier.hpp"
#include "opentxs/core/contract/peer/BailmentNotice.hpp"
#include "opentxs/core/contract/peer/PeerRequest.hpp"
#include "opentxs/core/identifier/Server.hpp"
#include "opentxs/core/identifier/UnitDefinition.hpp"

namespace opentxs
{
namespace api
{
namespace internal
{
struct Core;
}  // namespace internal
}  // namespace api

namespace identifier
{
class Nym;
}  // namespace identifier

class Factory;
class PasswordPrompt;
}  // namespace opentxs

namespace opentxs::contract::peer::request::implementation
{
class BailmentNotice final : public request::BailmentNotice,
                             public peer::implementation::Request
{
public:
    BailmentNotice(
        const api::internal::Core& api,
        const Nym_p& nym,
        const SerializedType& serialized);
    BailmentNotice(
        const api::internal::Core& api,
        const Nym_p& nym,
        const identifier::Nym& recipientID,
        const identifier::UnitDefinition& unitID,
        const identifier::Server& serverID,
        const Identifier& requestID,
        const std::string& txid,
        const Amount& amount);

    ~BailmentNotice() final = default;

private:
    friend opentxs::Factory;

    const OTUnitID unit_;
    const OTServerID server_;
    const OTIdentifier requestID_;
    const std::string txid_;
    const OTAmount amount_;

    auto clone() const noexcept -> BailmentNotice* final
    {
        return new BailmentNotice(*this);
    }
    auto IDVersion(const Lock& lock) const -> SerializedType final;

    BailmentNotice() = delete;
    BailmentNotice(const BailmentNotice&);
    BailmentNotice(BailmentNotice&&) = delete;
    auto operator=(const BailmentNotice&) -> BailmentNotice& = delete;
    auto operator=(BailmentNotice&&) -> BailmentNotice& = delete;
};
}  // namespace opentxs::contract::peer::request::implementation
