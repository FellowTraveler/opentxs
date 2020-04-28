// Copyright (c) 2010-2020 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

// IWYU pragma: private
// IWYU pragma: friend ".*src/blind/token/Lucre.cpp"

#pragma once

#include <cstdint>
#include <memory>
#include <stdexcept>
#include <string>

#include "blind/Token.hpp"
#include "opentxs/Proto.hpp"
#include "opentxs/Proto.hpp"
#include "opentxs/Types.hpp"
#include "opentxs/blind/Token.hpp"
#include "opentxs/core/Data.hpp"
#include "opentxs/core/String.hpp"
#include "opentxs/protobuf/CashEnums.pb.h"
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
}  // namespace blind

namespace identity
{
class Nym;
}  // namespace identity

namespace proto
{
class Ciphertext;
class LucreTokenData;
}  // namespace proto

class Factory;
class PasswordPrompt;
}  // namespace opentxs

namespace opentxs::blind::token::implementation
{
class Lucre final : public Token
{
public:
    auto GetSpendable(String& output, const PasswordPrompt& reason) const
        -> bool;
    auto ID(const PasswordPrompt& reason) const -> std::string final;
    auto IsSpent(const PasswordPrompt& reason) const -> bool final;
    auto Serialize() const -> proto::Token final;

    auto AddSignature(const String& signature) -> bool;
    auto ChangeOwner(
        Purse& oldOwner,
        Purse& newOwner,
        const PasswordPrompt& reason) -> bool final;
    auto GenerateTokenRequest(
        const identity::Nym& owner,
        const Mint& mint,
        const PasswordPrompt& reason) -> bool final;
    auto GetPublicPrototoken(String& output, const PasswordPrompt& reason)
        -> bool;
    auto MarkSpent(const PasswordPrompt& reason) -> bool final;
    auto Process(
        const identity::Nym& owner,
        const Mint& mint,
        const PasswordPrompt& reason) -> bool final;

    Lucre(
        const api::internal::Core& api,
        Purse& purse,
        const proto::Token& serialized);
    Lucre(
        const api::internal::Core& api,
        const identity::Nym& owner,
        const Mint& mint,
        const Amount& value,
        Purse& purse,
        const PasswordPrompt& reason);
    Lucre(const Lucre& rhs, blind::Purse& newOwner);

    ~Lucre() final = default;

private:
    friend opentxs::Factory;

    const VersionNumber lucre_version_;
    OTString signature_;
    std::shared_ptr<proto::Ciphertext> private_;
    std::shared_ptr<proto::Ciphertext> public_;
    std::shared_ptr<proto::Ciphertext> spend_;

    void serialize_private(proto::LucreTokenData& lucre) const;
    void serialize_public(proto::LucreTokenData& lucre) const;
    void serialize_signature(proto::LucreTokenData& lucre) const;
    void serialize_spendable(proto::LucreTokenData& lucre) const;

    auto clone() const noexcept -> Lucre* final { return new Lucre(*this); }

    Lucre(
        const api::internal::Core& api,
        Purse& purse,
        const VersionNumber version,
        const proto::TokenState state,
        const std::uint64_t series,
        const Amount& value,
        const Time validFrom,
        const Time validTo,
        const String& signature,
        const std::shared_ptr<proto::Ciphertext> publicKey,
        const std::shared_ptr<proto::Ciphertext> privateKey,
        const std::shared_ptr<proto::Ciphertext> spendable);
    Lucre() = delete;
    Lucre(const Lucre&);
    Lucre(Lucre&&) = delete;
    auto operator=(const Lucre&) -> Lucre& = delete;
    auto operator=(Lucre&&) -> Lucre& = delete;
};
}  // namespace opentxs::blind::token::implementation
