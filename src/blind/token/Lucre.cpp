// Copyright (c) 2010-2020 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include "0_stdafx.hpp"           // IWYU pragma: associated
#include "1_Internal.hpp"         // IWYU pragma: associated
#include "blind/token/Lucre.hpp"  // IWYU pragma: associated

extern "C" {
#include <openssl/bio.h>
#include <openssl/ossl_typ.h>
}

#include <algorithm>
#include <cctype>
#include <regex>
#include <stdexcept>
#include <vector>

#include "blind/Lucre.hpp"
#include "crypto/library/OpenSSL_BIO.hpp"
#include "internal/api/Api.hpp"
#include "opentxs/Pimpl.hpp"
#include "opentxs/api/Factory.hpp"
#include "opentxs/api/storage/Storage.hpp"
#include "opentxs/blind/Mint.hpp"
#include "opentxs/blind/Purse.hpp"
#include "opentxs/core/Armored.hpp"
#include "opentxs/core/Log.hpp"
#include "opentxs/core/LogSource.hpp"
#include "opentxs/core/String.hpp"
#include "opentxs/core/identifier/Server.hpp"
#include "opentxs/core/identifier/UnitDefinition.hpp"
#include "opentxs/crypto/key/Symmetric.hpp"
#include "opentxs/protobuf/CashEnums.pb.h"
#include "opentxs/protobuf/Ciphertext.pb.h"
#include "opentxs/protobuf/LucreTokenData.pb.h"
#include "opentxs/protobuf/Token.pb.h"

#define LUCRE_TOKEN_VERSION 1

#define OT_METHOD "opentxs::blind::token::implementation::Lucre::"

namespace opentxs::blind::token::implementation
{
Lucre::Lucre(
    const api::internal::Core& api,
    Purse& purse,
    const VersionNumber version,
    const proto::TokenState state,
    const std::uint64_t series,
    const Amount& value,
    const Time validFrom,
    const Time validTo,
    const String& signature,
    std::shared_ptr<proto::Ciphertext> publicKey,
    std::shared_ptr<proto::Ciphertext> privateKey,
    std::shared_ptr<proto::Ciphertext> spendable)
    : Token(
          api,
          purse,
          OT_TOKEN_VERSION,
          state,
          series,
          value,
          validFrom,
          validTo)
    , lucre_version_(version)
    , signature_(signature)
    , private_(publicKey)
    , public_(privateKey)
    , spend_(spendable)
{
}

Lucre::Lucre(const Lucre& rhs)
    : Lucre(
          rhs.api_,
          rhs.purse_,
          rhs.lucre_version_,
          rhs.state_,
          rhs.series_,
          rhs.denomination_,
          rhs.valid_from_,
          rhs.valid_to_,
          rhs.signature_,
          rhs.private_,
          rhs.public_,
          rhs.spend_)
{
}

Lucre::Lucre(const Lucre& rhs, blind::Purse& newOwner)
    : Lucre(
          rhs.api_,
          newOwner,
          rhs.lucre_version_,
          rhs.state_,
          rhs.series_,
          rhs.denomination_,
          rhs.valid_from_,
          rhs.valid_to_,
          rhs.signature_,
          rhs.private_,
          rhs.public_,
          rhs.spend_)
{
}

Lucre::Lucre(
    const api::internal::Core& api,
    Purse& purse,
    const proto::Token& in)
    : Lucre(
          api,
          purse,
          in.lucre().version(),
          in.state(),
          in.series(),
            api.Factory().Amount(static_cast<std::int64_t>(in.denomination())),
          Clock::from_time_t(in.validfrom()),
          Clock::from_time_t(in.validto()),
          String::Factory(),
          nullptr,
          nullptr,
          nullptr)
{
    const auto& lucre = in.lucre();

    if (lucre.has_signature()) {
        LogInsane(OT_METHOD)(__FUNCTION__)(": This token has a signature")
            .Flush();
        signature_->Set(lucre.signature().data(), lucre.signature().size());
    } else {
        LogInsane(OT_METHOD)(__FUNCTION__)(
            ": This token does not have a signature")
            .Flush();
    }

    if (lucre.has_privateprototoken()) {
        LogInsane(OT_METHOD)(__FUNCTION__)(
            ": This token has a private prototoken")
            .Flush();
        private_ =
            std::make_shared<proto::Ciphertext>(lucre.privateprototoken());
    } else {
        LogInsane(OT_METHOD)(__FUNCTION__)(
            ": This token does not have a private prototoken")
            .Flush();
    }

    if (lucre.has_publicprototoken()) {
        LogInsane(OT_METHOD)(__FUNCTION__)(
            ": This token has a public prototoken")
            .Flush();
        public_ = std::make_shared<proto::Ciphertext>(lucre.publicprototoken());
    } else {
        LogInsane(OT_METHOD)(__FUNCTION__)(
            ": This token does not have a public prototoken")
            .Flush();
    }

    if (lucre.has_spendable()) {
        LogInsane(OT_METHOD)(__FUNCTION__)(
            ": This token has a spendable string")
            .Flush();
        spend_ = std::make_shared<proto::Ciphertext>(lucre.spendable());
    } else {
        LogInsane(OT_METHOD)(__FUNCTION__)(
            ": This token does not have a spendable string")
            .Flush();
    }
}

Lucre::Lucre(
    const api::internal::Core& api,
    const identity::Nym& owner,
    const Mint& mint,
    const Amount& value,
    Purse& purse,
    const PasswordPrompt& reason)
    : Lucre(
          api,
          purse,
          LUCRE_TOKEN_VERSION,
          proto::TOKENSTATE_BLINDED,
          mint.GetSeries(),
          value,
          mint.GetValidFrom(),
          mint.GetValidTo(),
          String::Factory(),
          nullptr,
          nullptr,
          nullptr)
{
    const bool generated = GenerateTokenRequest(owner, mint, reason);

    if (false == generated) {
        throw std::runtime_error("Failed to generate prototoken");
    }
}

auto Lucre::AddSignature(const String& signature) -> bool
{
    if (signature.empty()) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Missing signature").Flush();

        return false;
    }

    signature_ = signature;
    state_ = proto::TOKENSTATE_SIGNED;

    return true;
}

auto Lucre::ChangeOwner(
    Purse& oldOwner,
    Purse& newOwner,
    const PasswordPrompt& reason) -> bool
{
    // NOTE: private_ is never re-encrypted

    auto oldPass = api_.Factory().PasswordPrompt(reason);
    auto newPass = api_.Factory().PasswordPrompt(reason);
    auto& oldKey = oldOwner.PrimaryKey(oldPass);
    auto& newKey = newOwner.PrimaryKey(newPass);

    if (public_) {
        if (false == reencrypt(oldKey, oldPass, newKey, newPass, *public_)) {
            LogOutput(OT_METHOD)(__FUNCTION__)(
                ": Failed to re-encrypt public prototoken")
                .Flush();

            return false;
        }
    }

    if (spend_) {
        if (false == reencrypt(oldKey, oldPass, newKey, newPass, *spend_)) {
            LogOutput(OT_METHOD)(__FUNCTION__)(
                ": Failed to re-encrypt spendable token")
                .Flush();

            return false;
        }
    }

    return true;
}

auto Lucre::GenerateTokenRequest(
    const identity::Nym& owner,
    const Mint& mint,
    const PasswordPrompt& reason) -> bool
{
#if OT_LUCRE_DEBUG
    LucreDumper setDumper;
#endif

    crypto::implementation::OpenSSL_BIO bioBank = BIO_new(BIO_s_mem());
    auto armoredMint = Armored::Factory();
    mint.GetPublic(armoredMint, denomination_);
    auto serializedMint = String::Factory(armoredMint);

    if (serializedMint->empty()) {
        LogOutput(OT_METHOD)(__FUNCTION__)(
            ": Failed to get public mint for series ")(denomination_->str())
            .Flush();

        return false;
    } else {
        LogInsane(OT_METHOD)(__FUNCTION__)(": Begin mint series ")(
            denomination_->str())
            .Flush();
        LogInsane(serializedMint).Flush();
        LogInsane(OT_METHOD)(__FUNCTION__)(": End mint").Flush();
    }

    BIO_puts(bioBank, serializedMint->Get());
    PublicBank bank;
    bank.ReadBIO(bioBank);
    crypto::implementation::OpenSSL_BIO bioCoin = BIO_new(BIO_s_mem());
    crypto::implementation::OpenSSL_BIO bioPublicCoin = BIO_new(BIO_s_mem());
    CoinRequest req(bank);
    req.WriteBIO(bioCoin);
    static_cast<PublicCoinRequest*>(&req)->WriteBIO(bioPublicCoin);
    const auto strPrivateCoin = bioCoin.ToString();
    const auto strPublicCoin = bioPublicCoin.ToString();

    if (strPrivateCoin->empty()) {
        LogOutput(OT_METHOD)(__FUNCTION__)(
            ": Failed to generate private prototoken")
            .Flush();

        return false;
    }

    if (strPublicCoin->empty()) {
        LogOutput(OT_METHOD)(__FUNCTION__)(
            ": Failed to generate public prototoken")
            .Flush();

        return false;
    }

    private_ = std::make_shared<proto::Ciphertext>();
    public_ = std::make_shared<proto::Ciphertext>();

    if (false == bool(private_)) {
        LogOutput(OT_METHOD)(__FUNCTION__)(
            ": Failed to instantiate private prototoken")
            .Flush();

        return false;
    }

    if (false == bool(public_)) {
        LogOutput(OT_METHOD)(__FUNCTION__)(
            ": Failed to instantiate public prototoken")
            .Flush();

        return false;
    }

    {
        auto password = api_.Factory().PasswordPrompt(reason);
        const auto encryptedPrivate =
            purse_.SecondaryKey(owner, password)
                .Encrypt(
                    strPrivateCoin->Bytes(), password, *private_, false, mode_);

        if (false == bool(encryptedPrivate)) {
            LogOutput(OT_METHOD)(__FUNCTION__)(
                ": Failed to encrypt private prototoken")
                .Flush();

            return false;
        }
    }

    {
        auto password = api_.Factory().PasswordPrompt(reason);
        const auto encryptedPublic = purse_.PrimaryKey(password).Encrypt(
            strPublicCoin->Bytes(), password, *public_, false, mode_);

        if (false == bool(encryptedPublic)) {
            LogOutput(OT_METHOD)(__FUNCTION__)(
                ": Failed to encrypt public prototoken")
                .Flush();

            return false;
        }
    }

    return true;
}

auto Lucre::GetPublicPrototoken(String& output, const PasswordPrompt& reason)
    -> bool
{
    if (false == bool(public_)) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Missing public prototoken")
            .Flush();

        return false;
    }

    auto& ciphertext = *public_;
    bool decrypted{false};

    try {
        auto password = api_.Factory().PasswordPrompt(reason);
        decrypted = purse_.PrimaryKey(password).Decrypt(
            ciphertext, password, output.WriteInto());
    } catch (...) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Missing primary key").Flush();

        return false;
    }

    if (false == decrypted) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Failed to decrypt prototoken")
            .Flush();
    }

    return decrypted;
}

auto Lucre::GetSpendable(String& output, const PasswordPrompt& reason) const
    -> bool
{
    if (false == bool(spend_)) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Missing spendable token").Flush();

        return false;
    }

    auto& ciphertext = *spend_;
    bool decrypted{false};

    try {
        auto password = api_.Factory().PasswordPrompt(reason);
        decrypted = purse_.PrimaryKey(password).Decrypt(
            ciphertext, password, output.WriteInto());
    } catch (...) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Missing primary key").Flush();

        return false;
    }

    if (false == decrypted) {
        LogOutput(OT_METHOD)(__FUNCTION__)(
            ": Failed to decrypt spendable token")
            .Flush();
    }

    return decrypted;
}

auto Lucre::ID(const PasswordPrompt& reason) const -> std::string
{
    auto spendable = String::Factory();

    if (false == GetSpendable(spendable, reason)) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Missing spendable string")
            .Flush();

        return {};
    }

    std::string output;
    std::regex reg("id=([A-Z0-9]*)");
    std::cmatch match{};

    if (std::regex_search(spendable->Get(), match, reg)) { output = match[1]; }

    std::transform(output.begin(), output.end(), output.begin(), [](char c) {
        return (std::toupper(c));
    });

    return output;
}

auto Lucre::IsSpent(const PasswordPrompt& reason) const -> bool
{
    switch (state_) {
        case proto::TOKENSTATE_SPENT: {
            return true;
        }
        case proto::TOKENSTATE_BLINDED:
        case proto::TOKENSTATE_SIGNED:
        case proto::TOKENSTATE_EXPIRED: {
            return false;
        }
        case proto::TOKENSTATE_READY: {
            break;
        }
        case proto::TOKENSTATE_ERROR:
        default: {
            throw std::runtime_error("invalid token state");
        }
    }

    const auto id = ID(reason);

    if (id.empty()) {
        throw std::runtime_error("failed to calculate token ID");
    }

    return api_.Storage().CheckTokenSpent(notary_, unit_, series_, id);
}

auto Lucre::MarkSpent(const PasswordPrompt& reason) -> bool
{
    if (proto::TOKENSTATE_READY != state_) {
        throw std::runtime_error("invalid token state");
    }

    bool output{false};
    const auto id = ID(reason);

    if (id.empty()) {
        throw std::runtime_error("failed to calculate token ID");
    }

    try {
        output = api_.Storage().MarkTokenSpent(notary_, unit_, series_, id);
    } catch (...) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Failed to load spendable token")
            .Flush();
    }

    if (output) { state_ = proto::TOKENSTATE_SPENT; }

    return output;
}

auto Lucre::Process(
    const identity::Nym& owner,
    const Mint& mint,
    const PasswordPrompt& reason) -> bool
{
    if (proto::TOKENSTATE_SIGNED != state_) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Incorrect token state.").Flush();

        return false;
    } else {
        LogInsane(OT_METHOD)(__FUNCTION__)(": Processing signed token").Flush();
    }

    if (signature_->empty()) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Missing signature").Flush();

        return false;
    } else {
        LogInsane(OT_METHOD)(__FUNCTION__)(": Loaded signature").Flush();
    }

    if (false == bool(private_)) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Missing encrypted prototoken")
            .Flush();

        return false;
    } else {
        LogInsane(OT_METHOD)(__FUNCTION__)(": Loaded encrypted prototoken")
            .Flush();
    }

#if OT_LUCRE_DEBUG
    LucreDumper setDumper;
#endif
    crypto::implementation::OpenSSL_BIO bioBank = BIO_new(BIO_s_mem());
    crypto::implementation::OpenSSL_BIO bioSignature = BIO_new(BIO_s_mem());
    crypto::implementation::OpenSSL_BIO bioPrivateRequest =
        BIO_new(BIO_s_mem());
    crypto::implementation::OpenSSL_BIO bioCoin = BIO_new(BIO_s_mem());
    auto armoredMint = Armored::Factory();
    mint.GetPublic(armoredMint, denomination_);
    auto serializedMint = String::Factory(armoredMint);

    if (serializedMint->empty()) {
        LogOutput(OT_METHOD)(__FUNCTION__)(
            ": Failed to get public mint for series ")(denomination_->str())
            .Flush();

        return false;
    } else {
        LogInsane(OT_METHOD)(__FUNCTION__)(": Begin mint series ")(
            denomination_->str())
            .Flush();
        LogInsane(serializedMint).Flush();
        LogInsane(OT_METHOD)(__FUNCTION__)(": End mint").Flush();
    }

    BIO_puts(bioBank, serializedMint->Get());
    BIO_puts(bioSignature, signature_->Get());
    auto prototoken = String::Factory();

    try {
        auto password = api_.Factory().PasswordPrompt(reason);
        auto& key = purse_.SecondaryKey(owner, password);
        const auto decrypted =
            key.Decrypt(*private_, password, prototoken->WriteInto());

        if (false == decrypted) {
            LogOutput(OT_METHOD)(__FUNCTION__)(": Failed to decrypt prototoken")
                .Flush();

            return false;
        } else {
            LogInsane(OT_METHOD)(__FUNCTION__)(": Prototoken decrypted")
                .Flush();
        }
    } catch (...) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Failed to get secondary key.")
            .Flush();

        return false;
    }

    if (prototoken->empty()) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Missing prototoken").Flush();

        return false;
    } else {
        LogInsane(OT_METHOD)(__FUNCTION__)(": Prototoken ready:").Flush();
    }

    BIO_puts(bioPrivateRequest, prototoken->Get());
    PublicBank bank(bioBank);
    CoinRequest req(bioPrivateRequest);
    ReadNumber(bioSignature, "request=");
    BIGNUM* bnSignature = ReadNumber(bioSignature, "signature=");
    DumpNumber("signature=", bnSignature);
    Coin coin;
    req.ProcessResponse(&coin, bank, bnSignature);
    coin.WriteBIO(bioCoin);
    const auto spend = bioCoin.ToString();

    if (spend->empty()) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Failed to read token").Flush();

        return false;
    } else {
        LogInsane(OT_METHOD)(__FUNCTION__)(": Obtained spendable token")
            .Flush();
    }

    spend_ = std::make_shared<proto::Ciphertext>();

    if (false == bool(spend_)) {
        LogOutput(OT_METHOD)(__FUNCTION__)(
            ": Failed to instantiate spendable ciphertext")
            .Flush();

        return false;
    }

    try {
        auto password = api_.Factory().PasswordPrompt(reason);
        auto& key = purse_.PrimaryKey(password);
        const auto encrypted =
            key.Encrypt(spend->Bytes(), password, *spend_, false, mode_);

        if (false == encrypted) {
            LogOutput(OT_METHOD)(__FUNCTION__)(
                ": Failed to encrypt spendable token")
                .Flush();

            return false;
        }
    } catch (...) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Failed to get primary key.")
            .Flush();

        return false;
    }

    state_ = proto::TOKENSTATE_READY;
    private_.reset();
    public_.reset();

    return true;
}

auto Lucre::Serialize() const -> proto::Token
{
    auto output = Token::Serialize();
    auto& lucre = *output.mutable_lucre();
    lucre.set_version(lucre_version_);

    switch (state_) {
        case proto::TOKENSTATE_BLINDED: {
            serialize_private(lucre);
            serialize_public(lucre);
        } break;
        case proto::TOKENSTATE_SIGNED: {
            serialize_private(lucre);
            serialize_public(lucre);
            serialize_signature(lucre);
        } break;
        case proto::TOKENSTATE_READY:
        case proto::TOKENSTATE_SPENT: {
            serialize_spendable(lucre);
        } break;
        case proto::TOKENSTATE_EXPIRED: {
            if (false == signature_->empty()) { serialize_signature(lucre); }

            if (private_) { serialize_private(lucre); }

            if (public_) { serialize_public(lucre); }

            if (spend_) { serialize_spendable(lucre); }
        } break;
        default: {
            throw std::runtime_error("invalid token state");
        }
    }

    return output;
}

void Lucre::serialize_private(proto::LucreTokenData& lucre) const
{
    if (false == bool(private_)) {
        throw std::runtime_error("missing private prototoken");
    }

    *lucre.mutable_privateprototoken() = *private_;
}

void Lucre::serialize_public(proto::LucreTokenData& lucre) const
{
    if (false == bool(public_)) {
        throw std::runtime_error("missing public prototoken");
    }

    *lucre.mutable_publicprototoken() = *public_;
}

void Lucre::serialize_signature(proto::LucreTokenData& lucre) const
{
    if (signature_->empty()) { throw std::runtime_error("missing signature"); }

    lucre.set_signature(signature_->Get(), signature_->GetLength());
}

void Lucre::serialize_spendable(proto::LucreTokenData& lucre) const
{
    if (false == bool(spend_)) {
        throw std::runtime_error("missing spendable token");
    }

    *lucre.mutable_spendable() = *spend_;
}
}  // namespace opentxs::blind::token::implementation
