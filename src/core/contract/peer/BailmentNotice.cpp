// Copyright (c) 2010-2020 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include "0_stdafx.hpp"                           // IWYU pragma: associated
#include "1_Internal.hpp"                         // IWYU pragma: associated
#include "core/contract/peer/BailmentNotice.hpp"  // IWYU pragma: associated

#include <memory>
#include <stdexcept>
#include <utility>

#include "2_Factory.hpp"
#include "core/contract/peer/PeerRequest.hpp"
#include "internal/api/Api.hpp"
#include "opentxs/Pimpl.hpp"
#include "opentxs/api/Factory.hpp"
#include "opentxs/api/Wallet.hpp"
#include "opentxs/core/Log.hpp"
#include "opentxs/core/LogSource.hpp"
#include "opentxs/core/String.hpp"
#include "opentxs/core/identifier/Server.hpp"
#include "opentxs/core/identifier/UnitDefinition.hpp"
#include "opentxs/protobuf/Check.hpp"
#include "opentxs/protobuf/PeerEnums.pb.h"
#include "opentxs/protobuf/PeerRequest.pb.h"
#include "opentxs/protobuf/PendingBailment.pb.h"
#include "opentxs/protobuf/verify/PeerRequest.hpp"

#define CURRENT_VERSION 6

namespace opentxs
{
using ParentType = contract::peer::implementation::Request;
using ReturnType = contract::peer::request::implementation::BailmentNotice;

auto Factory::BailmentNotice(
    const api::internal::Core& api,
    const Nym_p& nym,
    const identifier::Nym& recipientID,
    const identifier::UnitDefinition& unitID,
    const identifier::Server& serverID,
    const opentxs::Identifier& requestID,
    const std::string& txid,
    const opentxs::Amount& amount,
    const opentxs::PasswordPrompt& reason) noexcept
    -> std::shared_ptr<contract::peer::request::BailmentNotice>
{
    try {
        api.Wallet().UnitDefinition(unitID);
        auto output = std::make_shared<ReturnType>(
            api, nym, recipientID, unitID, serverID, requestID, txid, amount);

        OT_ASSERT(output);

        auto& reply = *output;

        if (false == ParentType::Finish(reply, reason)) { return {}; }

        return std::move(output);
    } catch (const std::exception& e) {
        LogOutput("opentxs::Factory::")(__FUNCTION__)(": ")(e.what()).Flush();

        return {};
    }
}

auto Factory::BailmentNotice(
    const api::internal::Core& api,
    const Nym_p& nym,
    const proto::PeerRequest& serialized) noexcept
    -> std::shared_ptr<contract::peer::request::BailmentNotice>
{
    if (false == proto::Validate(serialized, VERBOSE)) {
        LogOutput("opentxs::Factory::")(__FUNCTION__)(
            ": Invalid serialized request.")
            .Flush();

        return {};
    }

    try {
        auto output = std::make_shared<ReturnType>(api, nym, serialized);
        auto& contract = *output;
        Lock lock(contract.lock_);

        if (false == contract.validate(lock)) {
            LogOutput("opentxs::Factory::")(__FUNCTION__)(": Invalid request.")
                .Flush();

            return {};
        }

        return std::move(output);
    } catch (const std::exception& e) {
        LogOutput("opentxs::Factory::")(__FUNCTION__)(": ")(e.what()).Flush();

        return {};
    }
}
}  // namespace opentxs

namespace opentxs::contract::peer::request::implementation
{
BailmentNotice::BailmentNotice(
    const api::internal::Core& api,
    const Nym_p& nym,
    const identifier::Nym& recipientID,
    const identifier::UnitDefinition& unitID,
    const identifier::Server& serverID,
    const Identifier& requestID,
    const std::string& txid,
    const Amount& amount)
    : Request(
          api,
          nym,
          CURRENT_VERSION,
          recipientID,
          serverID,
          proto::PEERREQUEST_PENDINGBAILMENT)
    , unit_(unitID)
    , server_(serverID)
    , requestID_(requestID)
    , txid_(txid)
    , amount_(opentxs::Factory::Amount(amount))
{
    Lock lock(lock_);
    first_time_init(lock);
}

BailmentNotice::BailmentNotice(
    const api::internal::Core& api,
    const Nym_p& nym,
    const SerializedType& serialized)
    : Request(api, nym, serialized)
    , unit_(api_.Factory().UnitID(serialized.pendingbailment().unitid()))
    , server_(api_.Factory().ServerID(serialized.pendingbailment().serverid()))
    , requestID_(Identifier::Factory(serialized.pendingbailment().requestid()))
    , txid_(serialized.pendingbailment().txid())
    , amount_(opentxs::Factory::Amount(serialized.pendingbailment().amount()))
{
    Lock lock(lock_);
    init_serialized(lock);
}

BailmentNotice::BailmentNotice(const BailmentNotice& rhs)
    : Request(rhs)
    , unit_(rhs.unit_)
    , server_(rhs.server_)
    , requestID_(rhs.requestID_)
    , txid_(rhs.txid_)
    , amount_(opentxs::Factory::Amount(rhs.amount_))
{
}

auto BailmentNotice::IDVersion(const Lock& lock) const -> SerializedType
{
    auto contract = Request::IDVersion(lock);
    auto& pendingbailment = *contract.mutable_pendingbailment();
    pendingbailment.set_version(version_);
    pendingbailment.set_unitid(String::Factory(unit_)->Get());
    pendingbailment.set_serverid(String::Factory(server_)->Get());
    pendingbailment.set_requestid(String::Factory(requestID_)->Get());
    pendingbailment.set_txid(txid_);
    pendingbailment.set_amount_mp(amount_->str());

    return contract;
}
}  // namespace opentxs::contract::peer::request::implementation
