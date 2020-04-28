// Copyright (c) 2010-2020 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include "0_stdafx.hpp"          // IWYU pragma: associated
#include "1_Internal.hpp"        // IWYU pragma: associated
#include "blind/mint/Lucre.hpp"  // IWYU pragma: associated

extern "C" {
#include <openssl/bio.h>
#include <openssl/bn.h>
#include <openssl/ossl_typ.h>
}

#include <utility>

#include "2_Factory.hpp"
#include "blind/Lucre.hpp"
#include "blind/Mint.hpp"
#include "blind/token/Lucre.hpp"
#include "crypto/library/OpenSSL_BIO.hpp"
#include "internal/api/Api.hpp"
#include "opentxs/Pimpl.hpp"
#include "opentxs/api/Factory.hpp"
#include "opentxs/blind/Token.hpp"
#include "opentxs/core/Armored.hpp"
#include "opentxs/core/Contract.hpp"
#include "opentxs/core/Log.hpp"
#include "opentxs/core/LogSource.hpp"
#include "opentxs/core/String.hpp"
#include "opentxs/core/identifier/Nym.hpp"
#include "opentxs/crypto/Envelope.hpp"
#include "opentxs/identity/Nym.hpp"
#include "opentxs/protobuf/CashEnums.pb.h"

#ifdef __APPLE__
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#endif

#define OT_METHOD "opentxs::blind::mint::implementation::Lucre::"

namespace opentxs
{
auto Factory::MintLucre(const api::internal::Core& core) -> blind::Mint*
{
    return new blind::mint::implementation::Lucre(core);
}

auto Factory::MintLucre(
    const api::internal::Core& core,
    const String& strNotaryID,
    const String& strInstrumentDefinitionID) -> blind::Mint*
{
    return new blind::mint::implementation::Lucre(
        core, strNotaryID, strInstrumentDefinitionID);
}

auto Factory::MintLucre(
    const api::internal::Core& core,
    const String& strNotaryID,
    const String& strServerNymID,
    const String& strInstrumentDefinitionID) -> blind::Mint*
{
    return new blind::mint::implementation::Lucre(
        core, strNotaryID, strServerNymID, strInstrumentDefinitionID);
}
}  // namespace opentxs

namespace opentxs::blind::mint::implementation
{
Lucre::Lucre(const api::internal::Core& core)
    : Contract(core)
    , Mint(core)
{
}

Lucre::Lucre(
    const api::internal::Core& core,
    const String& strNotaryID,
    const String& strInstrumentDefinitionID)
    : Contract(core, strInstrumentDefinitionID)
    , Mint(core, strNotaryID, strInstrumentDefinitionID)
{
}

Lucre::Lucre(
    const api::internal::Core& core,
    const String& strNotaryID,
    const String& strServerNymID,
    const String& strInstrumentDefinitionID)
    : Contract(core, strInstrumentDefinitionID)
    , Mint(core, strNotaryID, strServerNymID, strInstrumentDefinitionID)
{
}

// The mint has a different key pair for each denomination.
// Pass the actual denomination such as 5, 10, 20, 50, 100...
auto Lucre::AddDenomination(
    const identity::Nym& theNotary,
    const Amount& denomination,
    const std::size_t keySize,
    const PasswordPrompt& reason) -> bool
{
    bool bReturnValue = false;

    // Let's make sure it doesn't already exist
    auto theArmor = Armored::Factory();
    if (GetPublic(theArmor, denomination)) {
        LogOutput(OT_METHOD)(__FUNCTION__)(
            ": Error: Denomination public already exists in AddDenomination.")
            .Flush();
        return false;
    }
    if (GetPrivate(theArmor, denomination)) {
        LogOutput(OT_METHOD)(__FUNCTION__)(
            ": Error: Denomination private already exists in "
            "AddDenomination.")
            .Flush();
        return false;
    }

    if ((keySize / 8) < (MIN_COIN_LENGTH + DIGEST_LENGTH)) {

        LogOutput(OT_METHOD)(__FUNCTION__)(": Prime must be at least ")(
            (MIN_COIN_LENGTH + DIGEST_LENGTH) * 8)(" bits.")
            .Flush();
        return false;
    }

    if (keySize % 8) {
        LogOutput(OT_METHOD)(__FUNCTION__)(
            ": Prime length must be a multiple of 8.")
            .Flush();
        return false;
    }

#if OT_LUCRE_DEBUG
#ifdef _WIN32
    BIO* out = BIO_new_file("openssl.dump", "w");
    assert(out);
    SetDumper(out);
#else
    SetMonitor(stderr);
#endif
#endif

    crypto::implementation::OpenSSL_BIO bio = BIO_new(BIO_s_mem());
    crypto::implementation::OpenSSL_BIO bioPublic = BIO_new(BIO_s_mem());

    // Generate the mint private key information
    Bank bank(keySize / 8);
    bank.WriteBIO(bio);

    // Generate the mint public key information
    PublicBank pbank(bank);
    pbank.WriteBIO(bioPublic);
    const auto strPrivateBank = bio.ToString();
    const auto strPublicBank = bioPublic.ToString();

    if (strPrivateBank->empty()) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Failed to generate private mint")
            .Flush();

        return false;
    }

    if (strPublicBank->empty()) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Failed to generate public mint")
            .Flush();

        return false;
    }

    auto pPublic = Armored::Factory();
    auto pPrivate = Armored::Factory();

    // Set the public bank info onto pPublic
    pPublic->SetString(strPublicBank, true);  // linebreaks = true

    // Seal the private bank info up into an encrypted Envelope
    // and set it onto pPrivate
    auto envelope = api_.Factory().Envelope();
    envelope->Seal(theNotary, strPrivateBank->Bytes(), reason);
    // TODO check the return values on these twofunctions
    envelope->Armored(pPrivate);

    // Add the new key pair to the maps, using denomination as the key
    m_mapPublic.emplace(denomination, std::move(pPublic));
    m_mapPrivate.emplace(denomination, std::move(pPrivate));

    // Grab the Server Nym ID and save it with this Mint
    theNotary.GetIdentifier(m_ServerNymID);
    m_nDenominationCount++;
    bReturnValue = true;
    LogDetail(OT_METHOD)(__FUNCTION__)(": Successfully added denomination: ")(
        denomination.str())
        .Flush();

    return bReturnValue;
}

#if OT_CRYPTO_USING_OPENSSL

// Lucre step 3: the mint signs the token
//
auto Lucre::SignToken(
    const identity::Nym& notary,
    blind::Token& token,
    const PasswordPrompt& reason) -> bool
{
#if OT_LUCRE_DEBUG
    LucreDumper setDumper;
#endif

    if (proto::CASHTYPE_LUCRE != token.Type()) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Incorrect token type").Flush();

        return false;
    } else {
        LogInsane(OT_METHOD)(__FUNCTION__)(": Signing a lucre token").Flush();
    }

    auto& lToken = dynamic_cast<blind::token::implementation::Lucre&>(token);
    crypto::implementation::OpenSSL_BIO bioBank = BIO_new(BIO_s_mem());
    crypto::implementation::OpenSSL_BIO bioRequest = BIO_new(BIO_s_mem());
    crypto::implementation::OpenSSL_BIO bioSignature = BIO_new(BIO_s_mem());

    auto armoredPrivate = Armored::Factory();

    if (false == GetPrivate(armoredPrivate, lToken.Value())) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Failed to load private key")
            .Flush();

        return false;
    } else {
        LogInsane(OT_METHOD)(__FUNCTION__)(": Loaded private mint key").Flush();
    }

    auto privateKey = String::Factory();

    try {
        auto envelope = api_.Factory().Envelope(armoredPrivate);

        if (false == envelope->Open(notary, privateKey->WriteInto(), reason)) {
            LogOutput(OT_METHOD)(__FUNCTION__)(
                ": Failed to decrypt private key")
                .Flush();

            return false;
        } else {
            LogInsane(OT_METHOD)(__FUNCTION__)(": Decrypted private mint key")
                .Flush();
        }
    } catch (...) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Failed to decode ciphertext")
            .Flush();

        return false;
    }

    BIO_puts(bioBank, privateKey->Get());
    Bank bank(bioBank);
    auto prototoken = String::Factory();

    if (false == lToken.GetPublicPrototoken(prototoken, reason)) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Failed to extract prototoken")
            .Flush();

        return false;
    } else {
        LogInsane(OT_METHOD)(__FUNCTION__)(": Extracted prototoken").Flush();
    }

    BIO_puts(bioRequest, prototoken->Get());
    PublicCoinRequest req(bioRequest);
    BIGNUM* bnSignature = bank.SignRequest(req);

    if (nullptr == bnSignature) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Failed to sign prototoken")
            .Flush();

        return false;
    } else {
        LogInsane(OT_METHOD)(__FUNCTION__)(": Signed prototoken").Flush();
    }

    req.WriteBIO(bioSignature);
    DumpNumber(bioSignature, "signature=", bnSignature);
    BN_free(bnSignature);
    char sig_buf[1024]{};
    auto sig_len = BIO_read(bioSignature, sig_buf, 1023);
    sig_buf[sig_len] = '\0';

    if (0 == sig_len) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Failed to copy signature")
            .Flush();

        return false;
    } else {
        LogInsane(OT_METHOD)(__FUNCTION__)(": Signature copied").Flush();
    }

    auto signature = String::Factory(sig_buf);

    if (false == lToken.AddSignature(signature)) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Failed to set signature").Flush();

        return false;
    } else {
        LogInsane(OT_METHOD)(__FUNCTION__)(": Signature serialized").Flush();
    }

    return true;
}

auto Lucre::VerifyToken(
    const identity::Nym& notary,
    const blind::Token& token,
    const PasswordPrompt& reason) -> bool
{

    if (proto::CASHTYPE_LUCRE != token.Type()) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Incorrect token type").Flush();

        return false;
    }

    const auto& lucreToken =
        dynamic_cast<const blind::token::implementation::Lucre&>(token);

#if OT_LUCRE_DEBUG
    LucreDumper setDumper;
#endif

    crypto::implementation::OpenSSL_BIO bioBank = BIO_new(BIO_s_mem());
    crypto::implementation::OpenSSL_BIO bioCoin = BIO_new(BIO_s_mem());
    auto spendable = String::Factory();

    if (false == lucreToken.GetSpendable(spendable, reason)) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Failed to extract").Flush();

        return false;
    }

    BIO_puts(bioCoin, spendable->Get());
    auto armoredPrivate = Armored::Factory();
    GetPrivate(armoredPrivate, token.Value());
    auto privateKey = String::Factory();

    try {
        auto envelope = api_.Factory().Envelope(armoredPrivate);

        if (false == envelope->Open(notary, privateKey->WriteInto(), reason)) {
            LogOutput(OT_METHOD)(__FUNCTION__)(
                ": Failed to decrypt private mint key")
                .Flush();

            return false;
        }
    } catch (...) {
        LogOutput(OT_METHOD)(__FUNCTION__)(
            ": Failed to decode private mint key")
            .Flush();

        return false;
    }

    BIO_puts(bioBank, privateKey->Get());
    Bank bank(bioBank);
    Coin coin(bioCoin);

    return bank.Verify(coin);
}

#endif  // OT_CRYPTO_USING_OPENSSL
}  // namespace opentxs::blind::mint::implementation
