// Copyright (c) 2010-2020 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

// IWYU pragma: private
// IWYU pragma: friend ".*src/blind/Purse.cpp"

#pragma once

#include <iosfwd>
#include <memory>
#include <optional>
#include <vector>

#include "opentxs/Proto.hpp"
#include "opentxs/Types.hpp"
#include "opentxs/blind/Purse.hpp"
#include "opentxs/blind/Token.hpp"
#include "opentxs/core/Amount.hpp"
#include "opentxs/core/Secret.hpp"
#include "opentxs/core/identifier/Server.hpp"
#include "opentxs/core/identifier/UnitDefinition.hpp"
#include "opentxs/crypto/Envelope.hpp"
#include "opentxs/crypto/key/Symmetric.hpp"
#include "opentxs/protobuf/CashEnums.pb.h"
#include "opentxs/protobuf/Enums.pb.h"
#include "opentxs/protobuf/Envelope.pb.h"
#include "opentxs/protobuf/Purse.pb.h"

namespace opentxs
{
namespace api
{
namespace internal
{
struct Core;
}  // namespace internal

namespace server
{
namespace internal
{
struct Manager;
}  // namespace internal
}  // namespace server
}  // namespace api

namespace blind
{
class Mint;
}  // namespace blind

namespace identifier
{
class Nym;
}  // namespace identifier

namespace identity
{
class Nym;
}  // namespace identity

class Factory;
class PasswordPrompt;
}  // namespace opentxs

namespace opentxs::blind::implementation
{
class Purse final : virtual public blind::Purse
{
public:
    auto at(const std::size_t position) const -> const Token& final
    {
        return tokens_.at(position);
    }
    auto begin() const noexcept -> const_iterator final { return cbegin(); }
    auto cbegin() const noexcept -> const_iterator final
    {
        return const_iterator(this, 0);
    }
    auto cend() const noexcept -> const_iterator final
    {
        return const_iterator(this, tokens_.size());
    }
    auto EarliestValidTo() const -> Time final { return earliest_valid_to_; }
    auto end() const noexcept -> const_iterator final { return cend(); }
    auto IsUnlocked() const -> bool final { return unlocked_; }
    auto LatestValidFrom() const -> Time final { return latest_valid_from_; }
    auto Notary() const -> const identifier::Server& final { return notary_; }
    auto Process(
        const identity::Nym& owner,
        const Mint& mint,
        const PasswordPrompt& reason) -> bool final;
    auto Serialize() const -> proto::Purse final;
    auto size() const noexcept -> std::size_t final { return tokens_.size(); }
    auto State() const -> proto::PurseType final { return state_; }
    auto Type() const -> proto::CashType final { return type_; }
    auto Unit() const -> const identifier::UnitDefinition& final
    {
        return unit_;
    }
    auto Unlock(const identity::Nym& nym, const PasswordPrompt& reason) const
        -> bool final;
    auto Verify(const api::server::internal::Manager& server) const
        -> bool final;
    auto Value() const -> const Amount& final { return total_value_; }

    auto AddNym(const identity::Nym& nym, const PasswordPrompt& reason)
        -> bool final;
    auto at(const std::size_t position) -> Token& final
    {
        return tokens_.at(position);
    }
    auto begin() noexcept -> iterator final { return iterator(this, 0); }
    auto end() noexcept -> iterator final
    {
        return iterator(this, tokens_.size());
    }
    auto GeneratePrototokens(
        const identity::Nym& owner,
        const Mint& mint,
        const Amount& amount,
        const PasswordPrompt& reason) -> bool;
    auto PrimaryKey(PasswordPrompt& password) -> crypto::key::Symmetric& final;
    auto Pop() -> std::shared_ptr<Token> final;
    auto Push(std::shared_ptr<Token> token, const PasswordPrompt& reason)
        -> bool final;
    auto SecondaryKey(const identity::Nym& owner, PasswordPrompt& password)
        -> const crypto::key::Symmetric& final;

    Purse(
        const api::internal::Core& api,
        const identifier::Nym& owner,
        const identifier::Server& server,
        const proto::CashType type,
        const Mint& mint,
        OTSecret&& secondaryKeyPassword,
        std::unique_ptr<const OTSymmetricKey> secondaryKey,
        std::unique_ptr<const OTEnvelope> secondaryEncrypted);

    ~Purse() final = default;

private:
    friend opentxs::Factory;

    static const proto::SymmetricMode mode_;

    const api::internal::Core& api_;
    const VersionNumber version_;
    const proto::CashType type_;
    const OTServerID notary_;
    const OTUnitID unit_;
    proto::PurseType state_;
    OTAmount total_value_;
    Time latest_valid_from_;
    Time earliest_valid_to_;
    std::vector<OTToken> tokens_;
    mutable bool unlocked_;
    mutable OTSecret primary_key_password_;
    std::shared_ptr<OTSymmetricKey> primary_;
    std::vector<proto::Envelope> primary_passwords_;
    OTSecret secondary_key_password_;
    const std::shared_ptr<const OTSymmetricKey> secondary_;
    const std::shared_ptr<const OTEnvelope> secondary_password_;

    static auto deserialize_secondary_key(
        const api::internal::Core& api,
        const proto::Purse& serialized) noexcept(false)
        -> std::unique_ptr<const OTSymmetricKey>;
    static auto deserialize_secondary_password(
        const api::internal::Core& api,
        const proto::Purse& serialized) noexcept(false)
        -> std::unique_ptr<const OTEnvelope>;
    static auto get_passwords(const proto::Purse& in)
        -> std::vector<proto::Envelope>;

    auto clone() const noexcept -> Purse* final { return new Purse(*this); }
    auto generate_key(Secret& password) const -> OTSymmetricKey;

    void apply_times(const Token& token);
    void recalculate_times();

    Purse(
        const api::internal::Core& api,
        const identifier::Server& server,
        const identifier::UnitDefinition& unit,
        const proto::CashType type);
    Purse(
        const api::internal::Core& api,
        const VersionNumber version,
        const proto::CashType type,
        const identifier::Server& notary,
        const identifier::UnitDefinition& unit,
        const proto::PurseType state,
        const opentxs::Amount& totalValue,
        const Time validFrom,
        const Time validTo,
        const std::vector<OTToken>& tokens,
        const std::shared_ptr<OTSymmetricKey> primary,
        const std::vector<proto::Envelope>& primaryPasswords,
        const std::shared_ptr<const OTSymmetricKey> secondaryKey,
        const std::shared_ptr<const OTEnvelope> secondaryEncrypted,
        std::optional<OTSecret> secondaryKeyPassword);
    Purse(const api::internal::Core& api, const Purse& owner);
    Purse(const api::internal::Core& api, const proto::Purse& serialized);
    Purse() = delete;
    Purse(const Purse&);
    Purse(Purse&&) = delete;
    auto operator=(const Purse&) -> Purse& = delete;
    auto operator=(Purse&&) -> Purse& = delete;
};
}  // namespace opentxs::blind::implementation
