// Copyright (c) 2010-2020 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#pragma once

#include <string>

#include "opentxs/api/Core.hpp"
#include "opentxs/api/Factory.hpp"
#include "opentxs/core/Identifier.hpp"
#include "opentxs/core/Secret.hpp"
#include "opentxs/core/contract/ServerContract.hpp"
#include "opentxs/core/contract/Signable.hpp"
#include "opentxs/core/contract/UnitDefinition.hpp"
#include "opentxs/core/contract/peer/PeerReply.hpp"
#include "opentxs/core/contract/peer/PeerRequest.hpp"
#include "opentxs/core/identifier/Nym.hpp"
#include "opentxs/protobuf/PeerReply.pb.h"
#include "opentxs/protobuf/PeerRequest.pb.h"
#include "opentxs/protobuf/ServerContract.pb.h"
#include "opentxs/protobuf/UnitDefinition.pb.h"

namespace opentxs::contract::blank
{
struct Signable : virtual public opentxs::contract::Signable {
    auto Alias() const -> std::string final { return {}; }
    auto ID() const -> OTIdentifier final { return id_; }
    auto Name() const -> std::string final { return {}; }
    auto Nym() const -> Nym_p final { return {}; }
    auto Terms() const -> const std::string& final { return terms_; }
    auto Serialize() const -> OTData final { return OTData{id_}; }
    auto Validate() const -> bool final { return {}; }
    auto Version() const -> VersionNumber final { return 0; }

    void SetAlias(const std::string&) final {}

    Signable(const api::Core& api)
        : api_(api)
        , id_(api.Factory().Identifier())
        , terms_()
    {
    }

    ~Signable() override = default;

protected:
    const api::Core& api_;
    const OTIdentifier id_;
    const std::string terms_;

    Signable(const Signable& rhs)
        : api_(rhs.api_)
        , id_(rhs.id_)
        , terms_(rhs.terms_)
    {
    }
};

struct Unit final : virtual public opentxs::contract::Unit, public Signable {
    auto AddAccountRecord(const std::string&, const Account&) const
        -> bool final
    {
        return {};
    }
    auto Contract() const -> SerializedType final { return {}; }
    auto DecimalPower() const -> std::int32_t final { return {}; }
    auto DisplayStatistics(String&) const -> bool final { return {}; }
    auto EraseAccountRecord(const std::string&, const Identifier&) const
        -> bool final
    {
        return {};
    }
    auto FormatAmountLocale(
        const Amount&,
        std::string&,
        const std::string&,
        const std::string&) const -> bool final
    {
        return {};
    }
    auto FormatAmountWithoutSymbolLocale(
        const Amount&,
        std::string&,
        const std::string&,
        const std::string&) const -> bool final
    {
        return {};
    }
    auto FractionalUnitName() const -> std::string final { return {}; }
    auto GetCurrencyName() const -> const std::string& final { return terms_; }
    auto GetCurrencySymbol() const -> const std::string& final
    {
        return terms_;
    }
    auto PublicContract() const -> SerializedType final { return {}; }
    auto StringToAmountLocale(
        OTAmount&,
        const std::string&,
        const std::string&,
        const std::string&) const -> bool final
    {
        return {};
    }
    auto TLA() const -> std::string final { return {}; }
    auto Type() const -> proto::UnitType final { return {}; }
    auto UnitOfAccount() const -> proto::ContactItemType final { return {}; }
    auto VisitAccountRecords(
        const std::string&,
        AccountVisitor&,
        const PasswordPrompt&) const -> bool final
    {
        return {};
    }

    void InitAlias(const std::string&) final {}

    Unit(const api::Core& api)
        : Signable(api)
    {
    }

    ~Unit() override = default;

private:
    auto clone() const noexcept -> Unit* override { return new Unit(*this); }

    Unit(const Unit& rhs)
        : Signable(rhs)
    {
    }
};

struct Server final : virtual public opentxs::contract::Server,
                      public blank::Signable {
    auto ConnectInfo(
        std::string&,
        std::uint32_t&,
        proto::AddressType&,
        const proto::AddressType&) const -> bool final
    {
        return {};
    }
    auto Contract() const -> proto::ServerContract final { return {}; }
    auto EffectiveName() const -> std::string final { return {}; }
    auto PublicContract() const -> proto::ServerContract final { return {}; }
    auto Statistics(String&) const -> bool final { return {}; }
    auto TransportKey() const -> const Data& final { return id_; }
    auto TransportKey(Data&, const PasswordPrompt&) const -> OTSecret final
    {
        return api_.Factory().Secret(0);
    }

    void InitAlias(const std::string&) final {}

    Server(const api::Core& api)
        : Signable(api)
    {
    }

    ~Server() final = default;

private:
    auto clone() const noexcept -> Server* final { return new Server(*this); }

    Server(const Server& rhs)
        : Signable(rhs)
    {
    }
};
}  // namespace opentxs::contract::blank

namespace opentxs::contract::peer::blank
{
struct Reply final : virtual public opentxs::contract::peer::Reply,
                     public contract::blank::Signable {
    auto Contract() const -> SerializedType final { return {}; }
    auto Type() const -> proto::PeerRequestType final
    {
        return proto::PEERREQUEST_ERROR;
    }

    Reply(const api::Core& api)
        : Signable(api)
    {
    }

    ~Reply() final = default;

private:
    auto clone() const noexcept -> Reply* final { return new Reply(*this); }

    Reply(const Reply& rhs)
        : Signable(rhs)
    {
    }
};

struct Request final : virtual public opentxs::contract::peer::Request,
                       public contract::blank::Signable {
    auto Contract() const -> SerializedType final { return {}; }
    auto Initiator() const -> const identifier::Nym& final { return nym_; }
    auto Recipient() const -> const identifier::Nym& final { return nym_; }
    auto Type() const -> proto::PeerRequestType final
    {
        return proto::PEERREQUEST_ERROR;
    }

    Request(const api::Core& api)
        : Signable(api)
        , nym_(api.Factory().NymID())
    {
    }

    ~Request() final = default;

private:
    const identifier::Nym& nym_;

    auto clone() const noexcept -> Request* final { return new Request(*this); }

    Request(const Request& rhs)
        : Signable(rhs)
        , nym_(rhs.nym_)
    {
    }
};
}  // namespace opentxs::contract::peer::blank
