// Copyright (c) 2010-2020 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#pragma once

#include <cstdint>

#include "opentxs/Proto.hpp"
#include "opentxs/Types.hpp"
#include "opentxs/blind/Token.hpp"
#include "opentxs/core/identifier/Server.hpp"
#include "opentxs/core/identifier/UnitDefinition.hpp"
#include "opentxs/protobuf/CashEnums.pb.h"
#include "opentxs/protobuf/Enums.pb.h"
#include "opentxs/protobuf/Token.pb.h"

namespace opentxs
{
namespace api
{
namespace internal
{
struct Core;
}  // namespace internal
}  // namespace api

namespace blind
{
class Mint;
class Purse;

namespace implementation
{
class Purse;
}  // namespace implementation
}  // namespace blind

namespace crypto
{
namespace key
{
class Symmetric;
}  // namespace key
}  // namespace crypto

namespace identity
{
class Nym;
}  // namespace identity

namespace proto
{
class Ciphertext;
}  // namespace proto

class PasswordPrompt;
}  // namespace opentxs

#define OT_TOKEN_VERSION 1

namespace opentxs::blind::token::implementation
{
class Token : virtual public blind::Token
{
public:
    auto Notary() const -> const identifier::Server& override
    {
        return notary_;
    }
    auto Owner() const noexcept -> Purse& final { return purse_; }
    auto Series() const -> MintSeries override { return series_; }
    auto State() const -> proto::TokenState override { return state_; }
    auto Type() const -> proto::CashType override { return type_; }
    auto Unit() const -> const identifier::UnitDefinition& override
    {
        return unit_;
    }
    auto ValidFrom() const -> Time override { return valid_from_; }
    auto ValidTo() const -> Time override { return valid_to_; }
    auto Value() const -> Denomination override { return denomination_; }

    virtual auto GenerateTokenRequest(

        const identity::Nym& owner,
        const Mint& mint,
        const PasswordPrompt& reason) -> bool = 0;

    ~Token() override = default;

protected:
    static const proto::SymmetricMode mode_;

    const api::internal::Core& api_;
    Purse& purse_;
    proto::TokenState state_;
    const OTServerID notary_;
    const OTUnitID unit_;
    const std::uint64_t series_;
    const OTAmount denomination_;
    const Time valid_from_;
    const Time valid_to_;

    auto reencrypt(
        const crypto::key::Symmetric& oldKey,
        const PasswordPrompt& oldPassword,
        const crypto::key::Symmetric& newKey,
        const PasswordPrompt& newPassword,
        proto::Ciphertext& ciphertext) -> bool;

    auto Serialize() const -> proto::Token override;

    Token(
        const api::internal::Core& api,
        Purse& purse,
        const proto::Token& serialized);
    Token(
        const api::internal::Core& api,
        Purse& purse,
        const VersionNumber version,
        const proto::TokenState state,
        const std::uint64_t series,
        const Amount& denomination,
        const Time validFrom,
        const Time validTo);
    Token(const Token&);

private:
    friend blind::implementation::Purse;

    const proto::CashType type_;
    const VersionNumber version_;

    virtual auto clone() const noexcept -> Token* override = 0;

    Token(
        const api::internal::Core& api,
        Purse& purse,
        const proto::TokenState state,
        const proto::CashType type,
        const identifier::Server& notary,
        const identifier::UnitDefinition& unit,
        const std::uint64_t series,
        const Amount& denomination,
        const Time validFrom,
        const Time validTo,
        const VersionNumber version);
    Token() = delete;
    Token(Token&&) = delete;
    auto operator=(const Token&) -> Token& = delete;
    auto operator=(Token&&) -> Token& = delete;
};
}  // namespace opentxs::blind::token::implementation
