// Copyright (c) 2010-2020 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include "0_stdafx.hpp"     // IWYU pragma: associated
#include "1_Internal.hpp"   // IWYU pragma: associated
#include "blind/Purse.hpp"  // IWYU pragma: associated

#include <algorithm>
#include <chrono>
#include <set>
#include <stdexcept>
#include <type_traits>
#include <utility>
#include <vector>

#include "2_Factory.hpp"
#include "blind/Token.hpp"
#include "internal/api/Api.hpp"
#include "internal/api/server/Server.hpp"
#include "opentxs/Pimpl.hpp"
#include "opentxs/api/Factory.hpp"
#include "opentxs/api/crypto/Symmetric.hpp"
#include "opentxs/blind/Mint.hpp"
#include "opentxs/blind/Purse.hpp"
#include "opentxs/blind/Token.hpp"
#include "opentxs/core/Amount.hpp"
#include "opentxs/core/Log.hpp"
#include "opentxs/core/LogSource.hpp"
#include "opentxs/core/PasswordPrompt.hpp"
#include "opentxs/core/Secret.hpp"
#include "opentxs/core/identifier/Server.hpp"
#include "opentxs/core/identifier/UnitDefinition.hpp"
#include "opentxs/crypto/key/Symmetric.hpp"
#include "opentxs/identity/Nym.hpp"
#include "opentxs/otx/consensus/Server.hpp"
#include "opentxs/protobuf/CashEnums.pb.h"
#include "opentxs/protobuf/Enums.pb.h"
#include "opentxs/protobuf/Envelope.pb.h"
#include "opentxs/protobuf/Purse.pb.h"
#include "opentxs/protobuf/Token.pb.h"

#define OT_PURSE_VERSION 1

#define OT_METHOD "opentxs::blind::implementation::Purse::"

namespace opentxs
{
using ReturnType = blind::implementation::Purse;

auto Factory::Purse(
    const api::internal::Core& api,
    const otx::context::Server& context,
    const proto::CashType type,
    const blind::Mint& mint,
    const Amount& totalValue,
    const opentxs::PasswordPrompt& reason) -> blind::Purse*
{
    return Purse(
        api,
        *context.Nym(),
        context.Notary(),
        context.RemoteNym(),
        type,
        mint,
        totalValue,
        reason);
}

auto Factory::Purse(
    const api::internal::Core& api,
    const identity::Nym& nym,
    const identifier::Server& server,
    const identity::Nym& serverNym,
    const proto::CashType type,
    const blind::Mint& mint,
    const opentxs::Amount& totalValue,
    const opentxs::PasswordPrompt& reason) -> blind::Purse*
{
    auto pEnvelope = std::make_unique<OTEnvelope>(api.Factory().Envelope());

    OT_ASSERT(pEnvelope);

    auto& envelope = pEnvelope->get();
    auto pSecondaryPassword = api.Factory().Secret(0);
    auto& secondaryPassword = pSecondaryPassword.get();
    secondaryPassword.Randomize(32);
    auto password = api.Factory().PasswordPrompt(reason);
    password->SetPassword(secondaryPassword);
    auto pSecondaryKey =
        std::make_unique<OTSymmetricKey>(api.Symmetric().Key(password));

    OT_ASSERT(pSecondaryKey);

    auto locked = envelope.Seal(nym, secondaryPassword.Bytes(), reason);

    if (false == locked) { return nullptr; }

    auto output = std::make_unique<ReturnType>(
        api,
        nym.ID(),
        server,
        type,
        mint,
        std::move(pSecondaryPassword),
        std::move(pSecondaryKey),
        std::move(pEnvelope));

    if (nullptr == output) { return nullptr; }

    auto& purse = *output;

    locked = purse.AddNym(serverNym, reason);
    locked = purse.AddNym(nym, reason);

    if (false == locked) { return nullptr; }

    const auto generated =
        purse.GeneratePrototokens(nym, mint, totalValue, reason);

    if (false == generated) { return nullptr; }

    return output.release();
}

auto Factory::Purse(
    const api::internal::Core& api,
    const proto::Purse& serialized) -> blind::Purse*
{
    return new blind::implementation::Purse(api, serialized);
}

auto Factory::Purse(
    const api::internal::Core& api,
    const blind::Purse& request,
    const identity::Nym& requester,
    const opentxs::PasswordPrompt& reason) -> blind::Purse*
{
    auto* output = new blind::implementation::Purse(
        api, dynamic_cast<const blind::implementation::Purse&>(request));

    if (nullptr == output) { return nullptr; }

    auto& purse = *output;
    auto locked = purse.AddNym(requester, reason);

    if (false == locked) { return nullptr; }

    return output;
}

auto Factory::Purse(
    const api::internal::Core& api,
    const identity::Nym& owner,
    const identifier::Server& server,
    const identifier::UnitDefinition& unit,
    const proto::CashType type,
    const opentxs::PasswordPrompt& reason) -> blind::Purse*
{
    auto* output = new blind::implementation::Purse(api, server, unit, type);

    if (nullptr == output) { return nullptr; }

    const auto added = output->AddNym(owner, reason);

    if (false == added) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Failed to encrypt purse").Flush();

        return nullptr;
    }

    return output;
}
}  // namespace opentxs

namespace opentxs::blind::implementation
{
const proto::SymmetricMode Purse::mode_{proto::SMODE_CHACHA20POLY1305};

Purse::Purse(
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
    std::optional<OTSecret> secondaryKeyPassword)
    : blind::Purse()
    , api_(api)
    , version_(version)
    , type_(type)
    , notary_(notary)
    , unit_(unit)
    , state_(state)
    , total_value_(totalValue)
    , latest_valid_from_(validFrom)
    , earliest_valid_to_(validTo)
    , tokens_(tokens)
    , unlocked_(false)
    , primary_key_password_(api_.Factory().Secret(0))
    , primary_(primary)
    , primary_passwords_(primaryPasswords)
    , secondary_key_password_(
          secondaryKeyPassword.has_value()
              ? std::move(secondaryKeyPassword.value())
              : api_.Factory().Secret(0))
    , secondary_(std::move(secondaryKey))
    , secondary_password_(std::move(secondaryEncrypted))
{
}

Purse::Purse(const Purse& rhs)
    : Purse(
          rhs.api_,
          rhs.version_,
          rhs.type_,
          rhs.notary_,
          rhs.unit_,
          rhs.state_,
          rhs.total_value_,
          rhs.latest_valid_from_,
          rhs.earliest_valid_to_,
          rhs.tokens_,
          rhs.primary_,
          rhs.primary_passwords_,
          rhs.secondary_,
          rhs.secondary_password_,
          {})
{
}

Purse::Purse(
    const api::internal::Core& api,
    const identifier::Nym& owner,
    const identifier::Server& server,
    const proto::CashType type,
    const Mint& mint,
    OTSecret&& secondaryKeyPassword,
    std::unique_ptr<const OTSymmetricKey> secondaryKey,
    std::unique_ptr<const OTEnvelope> secondaryEncrypted)
    : Purse(
          api,
          OT_PURSE_VERSION,
          type,
          server,
          mint.InstrumentDefinitionID(),
          proto::PURSETYPE_REQUEST,
          api_.Factory().Amount(),
          Time::min(),
          Time::max(),
          {},
          nullptr,
          {},
          std::move(secondaryKey),
          std::move(secondaryEncrypted),
          std::move(secondaryKeyPassword))
{
    auto primary = generate_key(primary_key_password_);
    primary_.reset(new OTSymmetricKey(std::move(primary)));
    unlocked_ = true;

    OT_ASSERT(primary_);
    OT_ASSERT(secondary_);
    OT_ASSERT(secondary_password_);
}

Purse::Purse(
    const api::internal::Core& api,
    const identifier::Server& server,
    const identifier::UnitDefinition& unit,
    const proto::CashType type)
    : Purse(
          api,
          OT_PURSE_VERSION,
          type,
          server,
          unit,
          proto::PURSETYPE_NORMAL,
          api.Factory().Amount(),
          Time::min(),
          Time::max(),
          {},
          nullptr,
          {},
          nullptr,
          nullptr,
          {})
{
    auto primary = generate_key(primary_key_password_);
    primary_.reset(new OTSymmetricKey(std::move(primary)));
    unlocked_ = true;

    OT_ASSERT(primary_);
}

Purse::Purse(const api::internal::Core& api, const proto::Purse& in)
    : Purse(
          api,
          in.version(),
          in.type(),
          identifier::Server::Factory(in.notary()),
          identifier::UnitDefinition::Factory(in.mint()),
          in.state(),
          api_.Factory().Amount(in.totalvalue()),
          Clock::from_time_t(in.latestvalidfrom()),
          Clock::from_time_t(in.earliestvalidto()),
          {},
          nullptr,
          get_passwords(in),
          deserialize_secondary_key(api, in),
          deserialize_secondary_password(api, in),
          {})
{
    auto primary =
        api.Symmetric().Key(in.primarykey(), proto::SMODE_CHACHA20POLY1305);
    primary_.reset(new OTSymmetricKey(std::move(primary)));

    OT_ASSERT(primary_);

    for (const auto& serialized : in.token()) {
        tokens_.emplace_back(Factory::Token(api_, *this, serialized).release());
    }
}

Purse::Purse(const api::internal::Core& api, const Purse& owner)
    : Purse(
          api,
          owner.version_,
          owner.type_,
          owner.notary_,
          owner.unit_,
          proto::PURSETYPE_ISSUE,
          api_.Factory().Amount(),
          Time::min(),
          Time::max(),
          {},
          nullptr,
          {},
          owner.secondary_,
          owner.secondary_password_,
          {})
{
    auto primary = generate_key(primary_key_password_);
    primary_.reset(new OTSymmetricKey(std::move(primary)));
    unlocked_ = true;

    OT_ASSERT(primary_);
}

auto Purse::AddNym(const identity::Nym& nym, const PasswordPrompt& reason)
    -> bool
{
    if (false == unlocked_) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Purse is locked").Flush();

        return false;
    }

    primary_passwords_.emplace_back();
    auto& sessionKey = *primary_passwords_.rbegin();

    if (false == bool(primary_)) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Missing primary key").Flush();

        return false;
    }

    auto envelope = api_.Factory().Envelope();

    if (envelope->Seal(nym, primary_key_password_->Bytes(), reason)) {
        sessionKey = envelope->Serialize();
    } else {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Failed to add nym").Flush();
        primary_passwords_.pop_back();

        return false;
    }

    return true;
}

void Purse::apply_times(const Token& token)
{
    latest_valid_from_ = std::max(latest_valid_from_, token.ValidFrom());
    earliest_valid_to_ = std::min(earliest_valid_to_, token.ValidTo());
}

auto Purse::deserialize_secondary_key(
    const api::internal::Core& api,
    const proto::Purse& in) noexcept(false)
    -> std::unique_ptr<const OTSymmetricKey>
{
    switch (in.state()) {
        case proto::PURSETYPE_REQUEST:
        case proto::PURSETYPE_ISSUE: {
            auto output = std::make_unique<OTSymmetricKey>(api.Symmetric().Key(
                in.secondarykey(), proto::SMODE_CHACHA20POLY1305));

            if (false == bool(output)) {
                LogOutput(OT_METHOD)(__FUNCTION__)(
                    ": Invalid serialized secondary key")
                    .Flush();

                throw std::runtime_error("Invalid serialized secondary key");
            }

            return std::move(output);
        }
        case proto::PURSETYPE_NORMAL: {
        } break;
        default: {
            LogOutput(OT_METHOD)(__FUNCTION__)(": Invalid purse state").Flush();

            throw std::runtime_error("invalid purse state");
        }
    }

    return {};
}

auto Purse::deserialize_secondary_password(
    const api::internal::Core& api,
    const proto::Purse& in) noexcept(false) -> std::unique_ptr<const OTEnvelope>
{
    switch (in.state()) {
        case proto::PURSETYPE_REQUEST:
        case proto::PURSETYPE_ISSUE: {
            auto output = std::make_unique<OTEnvelope>(
                api.Factory().Envelope(in.secondarypassword()));

            if (false == bool(output)) {
                LogOutput(OT_METHOD)(__FUNCTION__)(
                    ": Invalid serialized secondary password")
                    .Flush();

                throw std::runtime_error(
                    "Invalid serialized secondary password");
            }

            return std::move(output);
        }
        case proto::PURSETYPE_NORMAL: {
        } break;
        default: {
            LogOutput(OT_METHOD)(__FUNCTION__)(": Invalid purse state").Flush();

            throw std::runtime_error("invalid purse state");
        }
    }

    return {};
}

auto Purse::generate_key(Secret& password) const -> OTSymmetricKey
{
    password.Randomize(32);
    auto keyPassword = api_.Factory().PasswordPrompt("");
    keyPassword->SetPassword(password);

    return api_.Symmetric().Key(keyPassword, mode_);
}

// TODO replace this algorithm with one that will ensure all spends up to and
// including the specified amount are possible
auto Purse::GeneratePrototokens(
    const identity::Nym& owner,
    const Mint& mint,
    const opentxs::Amount& amount,
    const opentxs::PasswordPrompt& reason) -> bool
{
    OTAmount workingAmount(api_.Factory().Amount(amount));
    OTAmount tokenAmount{api_.Factory().Amount(mint.GetLargestDenomination(workingAmount))};

    while (tokenAmount > api_.Factory().Amount()) {
        try {
            workingAmount -= tokenAmount;
            std::shared_ptr<Token> pToken{
                Factory::Token(api_, owner, mint, tokenAmount, *this, reason)};

            if (false == bool(pToken)) {
                LogOutput(OT_METHOD)(__FUNCTION__)(
                    ": Failed to generate prototoken")
                    .Flush();

                return {};
            }

            if (false == Push(pToken, reason)) { return false; }

            tokenAmount = api_.Factory().Amount(mint.GetLargestDenomination(workingAmount));
        } catch (const std::exception& e) {
            LogOutput(OT_METHOD)(__FUNCTION__)(":")(e.what()).Flush();

            return false;
        }
    }

    return total_value_ == amount;
}

auto Purse::get_passwords(const proto::Purse& in)
    -> std::vector<proto::Envelope>
{
    auto output = std::vector<proto::Envelope>{};

    for (const auto& password : in.primarypassword()) {
        output.emplace_back(password);
    }

    return output;
}

auto Purse::PrimaryKey(PasswordPrompt& password) -> crypto::key::Symmetric&
{
    if (false == bool(primary_)) { throw std::out_of_range("No primary key"); }

    if (primary_passwords_.empty()) {
        throw std::out_of_range("No session keys");
    }

    if (false == unlocked_) { throw std::out_of_range("Purse is locked"); }

    password.SetPassword(primary_key_password_);

    return primary_->get();
}

auto Purse::Pop() -> std::shared_ptr<Token>
{
    if (0 == tokens_.size()) {
        LogTrace(OT_METHOD)(__FUNCTION__)(": Purse is empty").Flush();

        return {};
    }

    const auto& existing = dynamic_cast<const token::implementation::Token&>(
        tokens_.crbegin()->get());
    std::shared_ptr<Token> pToken{existing.clone()};

    if (false == bool(pToken)) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Failed to get token").Flush();

        return {};
    }

    auto& token = *pToken;
    tokens_.pop_back();
    total_value_ -= token.Value();
    recalculate_times();

    return pToken;
}

auto Purse::Process(
    const identity::Nym& owner,
    const Mint& mint,
    const opentxs::PasswordPrompt& reason) -> bool
{
    if (proto::PURSETYPE_ISSUE != state_) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Incorrect purse state").Flush();

        return false;
    }

    bool processed{true};

    for (auto& token : tokens_) {
        processed &= token->Process(owner, mint, reason);
    }

    if (processed) {
        state_ = proto::PURSETYPE_NORMAL;
        const_cast<std::shared_ptr<const OTEnvelope>&>(secondary_password_)
            .reset();
        const_cast<std::shared_ptr<const OTSymmetricKey>&>(secondary_).reset();
        secondary_key_password_ = api_.Factory().Secret(0);
    } else {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Failed to process token").Flush();
    }

    return processed;
}

auto Purse::Push(
    std::shared_ptr<Token> original,
    const opentxs::PasswordPrompt& reason) -> bool
{
    if (false == bool(original)) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Invalid token").Flush();

        return false;
    }

    if (false == bool(primary_)) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Missing primary key").Flush();

        return false;
    }

    if (false == unlocked_) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Purse is locked").Flush();

        return false;
    }

    auto copy = Factory::Token(*original, *this);

    OT_ASSERT(copy);

    auto& token = *copy;

    if (false == token.ChangeOwner(original->Owner(), *this, reason)) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Failed to encrypt token").Flush();

        return false;
    }

    switch (token.State()) {
        case proto::TOKENSTATE_BLINDED:
        case proto::TOKENSTATE_SIGNED:
        case proto::TOKENSTATE_READY: {
            total_value_ += token.Value();
            apply_times(token);
        } break;
        default: {
        }
    }

    tokens_.emplace(tokens_.begin(), copy.release());

    return true;
}

// TODO let's do this in constant time someday
void Purse::recalculate_times()
{
    latest_valid_from_ = Time::min();
    earliest_valid_to_ = Time::max();

    for (const auto& token : tokens_) { apply_times(token); }
}

auto Purse::SecondaryKey(
    const identity::Nym& owner,
    PasswordPrompt& passwordOut) -> const crypto::key::Symmetric&
{
    if (false == bool(secondary_)) {
        throw std::out_of_range("No secondary key");
    }

    if (false == bool(secondary_password_)) {
        throw std::out_of_range("No secondary key password");
    }

    auto& secondaryKey = secondary_->get();
    const auto& envelope = secondary_password_->get();
    const auto decrypted = envelope.Open(
        owner,
        secondary_key_password_->WriteInto(Secret::Mode::Mem),
        passwordOut);

    if (false == decrypted) {
        throw std::out_of_range("Failed to decrypt key password");
    }

    passwordOut.SetPassword(secondary_key_password_);
    const auto unlocked = secondaryKey.Unlock(passwordOut);

    if (false == unlocked) {
        throw std::out_of_range("Failed to unlock key password");
    }

    return secondaryKey;
}

auto Purse::Serialize() const -> proto::Purse
{
    proto::Purse output{};
    output.set_version(version_);
    output.set_type(type_);
    output.set_state(state_);
    output.set_notary(notary_->str());
    output.set_mint(unit_->str());
    output.set_totalvalue(std::stoll(total_value_->str()));
    output.set_latestvalidfrom(Clock::to_time_t(latest_valid_from_));
    output.set_earliestvalidto(Clock::to_time_t(earliest_valid_to_));

    for (const auto& token : tokens_) {
        *output.add_token() = token->Serialize();
    }

    if (false == bool(primary_)) {
        throw std::runtime_error("missing primary key");
    }

    if (false == primary_->get().Serialize(*output.mutable_primarykey())) {
        throw std::runtime_error("failed to serialize primary key");
    }

    for (const auto& password : primary_passwords_) {
        *output.add_primarypassword() = password;
    }

    switch (state_) {
        case proto::PURSETYPE_REQUEST:
        case proto::PURSETYPE_ISSUE: {
            if (false == bool(secondary_)) {
                throw std::runtime_error("missing secondary key");
            }

            if (!secondary_->get().Serialize(*output.mutable_secondarykey())) {
                throw std::runtime_error("failed to serialize secondary key");
            }

            if (false == bool(secondary_password_)) {
                throw std::runtime_error("missing secondary password");
            }

            *output.mutable_secondarypassword() =
                secondary_password_->get().Serialize();
        } break;
        case proto::PURSETYPE_NORMAL: {
        } break;
        default: {
            throw std::runtime_error("invalid purse state");
        }
    }

    return output;
}

auto Purse::Unlock(
    const identity::Nym& nym,
    const opentxs::PasswordPrompt& reason) const -> bool
{
    if (primary_passwords_.empty()) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": No session keys found").Flush();

        return false;
    }

    if (false == bool(primary_)) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Missing primary key").Flush();

        return false;
    }

    const auto primary = *primary_;
    auto password = api_.Factory().Secret(0);

    for (const auto& sessionKey : primary_passwords_) {
        try {
            const auto envelope = api_.Factory().Envelope(sessionKey);
            const auto opened = envelope->Open(
                nym, password->WriteInto(Secret::Mode::Mem), reason);

            if (opened) {
                auto unlocker =
                    api_.Factory().PasswordPrompt(reason.GetDisplayString());
                unlocker->SetPassword(password);
                unlocked_ = primary->Unlock(unlocker);

                if (unlocked_) {
                    primary_key_password_ = password;
                    break;
                } else {
                    LogOutput(OT_METHOD)(__FUNCTION__)(
                        ": Decrypted password does not unlock the primary key")
                        .Flush();
                }
            }
        } catch (...) {
            LogOutput(OT_METHOD)(__FUNCTION__)(": Invalid session key").Flush();

            continue;
        }
    }

    if (false == unlocked_) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Nym ")(nym.ID())(
            " can not decrypt any session key in the purse.")
            .Flush();
    }

    return unlocked_;
}

auto Purse::Verify(const api::server::internal::Manager& server) const -> bool
{
    OTAmount total{api_.Factory().Amount()};
    auto validFrom{Time::min()};
    auto validTo{Time::max()};
    std::set<proto::TokenState> allowedStates{};

    switch (state_) {
        case proto::PURSETYPE_REQUEST: {
            allowedStates.insert(proto::TOKENSTATE_BLINDED);
        } break;
        case proto::PURSETYPE_ISSUE: {
            allowedStates.insert(proto::TOKENSTATE_SIGNED);
        } break;
        case proto::PURSETYPE_NORMAL: {
            allowedStates.insert(proto::TOKENSTATE_READY);
            allowedStates.insert(proto::TOKENSTATE_SPENT);
            allowedStates.insert(proto::TOKENSTATE_EXPIRED);
        } break;
        default: {
            LogOutput(OT_METHOD)(__FUNCTION__)(": Invalid purse state.")
                .Flush();

            return false;
        }
    }

    for (const auto& pToken : tokens_) {
        const auto& token = pToken.get();

        if (type_ != token.Type()) {
            LogOutput(OT_METHOD)(__FUNCTION__)(
                ": Token type does not match purse type.")
                .Flush();

            return false;
        }

        if (notary_ != token.Notary()) {
            LogOutput(OT_METHOD)(__FUNCTION__)(
                ": Token notary does not match purse notary.")
                .Flush();

            return false;
        }

        if (unit_ != token.Unit()) {
            LogOutput(OT_METHOD)(__FUNCTION__)(
                ": Token unit does not match purse unit.")
                .Flush();

            return false;
        }

        if (0 == allowedStates.count(token.State())) {
            LogOutput(OT_METHOD)(__FUNCTION__)(": Incorrect token state")
                .Flush();

            return false;
        }

        auto pMint = server.GetPrivateMint(unit_, token.Series());

        if (false == bool(pMint)) {
            LogOutput(OT_METHOD)(__FUNCTION__)(": Incorrect token series")
                .Flush();

            return false;
        }

        auto& mint = *pMint;

        if (mint.Expired() && (token.State() != proto::TOKENSTATE_EXPIRED)) {
            LogOutput(OT_METHOD)(__FUNCTION__)(": Token is expired").Flush();

            return false;
        }

        if (token.ValidFrom() != mint.GetValidFrom()) {
            LogOutput(OT_METHOD)(__FUNCTION__)(": Incorrect token valid from")
                .Flush();

            return false;
        }

        if (token.ValidTo() != mint.GetValidTo()) {
            LogOutput(OT_METHOD)(__FUNCTION__)(": Incorrect token valid to")
                .Flush();

            return false;
        }

        validFrom = std::max(validFrom, token.ValidFrom());
        validTo = std::min(validTo, token.ValidFrom());

        switch (token.State()) {
            case proto::TOKENSTATE_BLINDED:
            case proto::TOKENSTATE_SIGNED:
            case proto::TOKENSTATE_READY: {
                total += token.Value();
                [[fallthrough]];
            }
            case proto::TOKENSTATE_SPENT:
            case proto::TOKENSTATE_EXPIRED: {
            } break;
            default: {
                LogOutput(OT_METHOD)(__FUNCTION__)(": Invalid token state")
                    .Flush();

                return false;
            }
        }
    }

    if (total_value_ != total) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Incorrect purse value").Flush();

        return false;
    }

    if (latest_valid_from_ != validFrom) {
        LogOutput(OT_METHOD)(__FUNCTION__)(
            ": Incorrect purse latest valid from")
            .Flush();

        return false;
    }

    if (earliest_valid_to_ != validTo) {
        LogOutput(OT_METHOD)(__FUNCTION__)(
            ": Incorrect purse earliest valid to")
            .Flush();

        return false;
    }

    return true;
}
}  // namespace opentxs::blind::implementation
