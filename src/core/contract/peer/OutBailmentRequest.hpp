// Copyright (c) 2010-2020 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

// IWYU pragma: private
// IWYU pragma: friend ".*src/core/contract/peer/OutbailmentRequest.cpp"

#pragma once

#include <cstdint>
#include <string>

#include "core/contract/peer/PeerRequest.hpp"
#include "opentxs/Types.hpp"
#include "opentxs/core/contract/peer/OutBailmentRequest.hpp"
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

namespace proto
{
class PeerRequest;
}  // namespace proto

class Factory;
class PasswordPrompt;
}  // namespace opentxs

namespace opentxs::contract::peer::request::implementation
{
class Outbailment final : public request::Outbailment,
                          public peer::implementation::Request
{
public:
    Outbailment(
        const api::internal::Core& api,
        const Nym_p& nym,
        const identifier::Nym& recipientID,
        const identifier::UnitDefinition& unitID,
        const identifier::Server& serverID,
        const std::uint64_t& amount,
        const std::string& terms);
    Outbailment(
        const api::internal::Core& api,
        const Nym_p& nym,
        const proto::PeerRequest& serialized);

    ~Outbailment() final = default;

private:
    friend opentxs::Factory;

    const OTUnitID unit_;
    const OTServerID server_;
    const std::uint64_t amount_;

    auto clone() const noexcept -> Outbailment* final
    {
        return new Outbailment(*this);
    }
    auto IDVersion(const Lock& lock) const -> SerializedType final;

    Outbailment() = delete;
    Outbailment(const Outbailment&);
    Outbailment(Outbailment&&) = delete;
    auto operator=(const Outbailment&) -> Outbailment& = delete;
    auto operator=(Outbailment&&) -> Outbailment& = delete;
};
}  // namespace opentxs::contract::peer::request::implementation
