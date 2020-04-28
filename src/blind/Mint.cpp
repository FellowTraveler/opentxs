// Copyright (c) 2010-2020 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include "0_stdafx.hpp"    // IWYU pragma: associated
#include "1_Internal.hpp"  // IWYU pragma: associated
#include "blind/Mint.hpp"  // IWYU pragma: associated

#include <irrxml/irrXML.hpp>
#include <chrono>
#include <cstdlib>
#include <memory>
#include <string>
#include <utility>

#include "internal/api/Api.hpp"
#include "opentxs/Exclusive.hpp"
#include "opentxs/Pimpl.hpp"
#include "opentxs/api/Factory.hpp"
#include "opentxs/api/Legacy.hpp"
#include "opentxs/api/Wallet.hpp"
#include "opentxs/core/Account.hpp"
#include "opentxs/core/Armored.hpp"
#include "opentxs/core/Contract.hpp"
#include "opentxs/core/Log.hpp"
#include "opentxs/core/LogSource.hpp"
#include "opentxs/core/OTStorage.hpp"
#include "opentxs/core/String.hpp"
#include "opentxs/core/StringXML.hpp"
#include "opentxs/core/util/Common.hpp"
#include "opentxs/core/util/Tag.hpp"
#include "opentxs/identity/Nym.hpp"

#define OT_METHOD "opentxs::blind::mint::implementation::Mint::"

namespace opentxs::blind::mint::implementation
{
Mint::Mint(
    const api::internal::Core& core,
    const String& strNotaryID,
    const String& strServerNymID,
    const String& strInstrumentDefinitionID)
    : m_mapPrivate()
    , m_mapPublic()
    , m_NotaryID(api_.Factory().ServerID(strNotaryID))
    , m_ServerNymID(api_.Factory().NymID(strServerNymID))
    , m_InstrumentDefinitionID(api_.Factory().UnitID(strInstrumentDefinitionID))
    , m_nDenominationCount(0)
    , m_bSavePrivateKeys(false)
    , m_nSeries(0)
    , m_VALID_FROM(Time::min())
    , m_VALID_TO(Time::min())
    , m_EXPIRATION(Time::min())
    , m_CashAccountID(api_.Factory().Identifier())
{
    m_strFoldername->Set(api_.Legacy().Mint());
    m_strFilename->Format(
        "%s%s%s",
        strNotaryID.Get(),
        PathSeparator(),
        strInstrumentDefinitionID.Get());

    InitMint();
}

Mint::Mint(
    const api::internal::Core& core,
    const String& strNotaryID,
    const String& strInstrumentDefinitionID)
    : m_mapPrivate()
    , m_mapPublic()
    , m_NotaryID(api_.Factory().ServerID(strNotaryID))
    , m_ServerNymID(api_.Factory().NymID())
    , m_InstrumentDefinitionID(api_.Factory().UnitID(strInstrumentDefinitionID))
    , m_nDenominationCount(0)
    , m_bSavePrivateKeys(false)
    , m_nSeries(0)
    , m_VALID_FROM(Time::min())
    , m_VALID_TO(Time::min())
    , m_EXPIRATION(Time::min())
    , m_CashAccountID(api_.Factory().Identifier())
{
    m_strFoldername->Set(api_.Legacy().Mint());
    m_strFilename->Format(
        "%s%s%s",
        strNotaryID.Get(),
        PathSeparator(),
        strInstrumentDefinitionID.Get());

    InitMint();
}

Mint::Mint(const api::internal::Core& core)
    : m_mapPrivate()
    , m_mapPublic()
    , m_NotaryID(api_.Factory().ServerID())
    , m_ServerNymID(api_.Factory().NymID())
    , m_InstrumentDefinitionID(api_.Factory().UnitID())
    , m_nDenominationCount(0)
    , m_bSavePrivateKeys(false)
    , m_nSeries(0)
    , m_VALID_FROM(Time::min())
    , m_VALID_TO(Time::min())
    , m_EXPIRATION(Time::min())
    , m_CashAccountID(api_.Factory().Identifier())
{
    InitMint();
}

// Verify the current date against the VALID FROM / EXPIRATION dates.
// (As opposed to tokens, which are verified against the valid from/to dates.)
auto Mint::Expired() const -> bool
{
    const auto CURRENT_TIME = Clock::now();

    if ((CURRENT_TIME >= m_VALID_FROM) && (CURRENT_TIME <= m_EXPIRATION)) {
        return false;
    } else {
        return true;
    }
}

void Mint::ReleaseDenominations()
{
    m_mapPublic.clear();
    m_mapPrivate.clear();
}

// If you want to load a certain Mint from string, then
// you will call LoadContractFromString() (say). Well this Releases the
// contract,
// before loading it, which calls InitMint() to zero out all the important
// pieces of
// data.
//
void Mint::Release_Mint()
{
    ReleaseDenominations();
    m_CashAccountID->Release();
}

void Mint::Release()
{
    Release_Mint();
    // I overrode the parent, so now I give him a chance to clean up.
    Contract::Release();
    InitMint();
}

void Mint::InitMint()
{
    m_strContractType->Set("MINT");

    m_nDenominationCount = 0;

    m_bSavePrivateKeys =
        false;  // Determines whether it serializes private keys (no if false)

    // Mints expire and new ones are rotated in.
    // All tokens have the same series, and validity dates,
    // of the mint that created them.
    m_nSeries = 0;
    m_VALID_FROM = Time::min();
    m_VALID_TO = Time::min();
    m_EXPIRATION = Time::min();
}

auto Mint::LoadContract() -> bool { return LoadMint(); }

auto Mint::LoadMint(const char* szAppend) -> bool  // todo: server should
                                                   // always pass something
                                                   // here. client never
                                                   // should. Enforcement?
{
    if (!m_strFoldername->Exists()) m_strFoldername->Set(api_.Legacy().Mint());

    const auto strNotaryID = String::Factory(m_NotaryID),
               strInstrumentDefinitionID =
                   String::Factory(m_InstrumentDefinitionID);

    if (!m_strFilename->Exists()) {
        if (nullptr != szAppend)
            m_strFilename->Format(
                "%s%s%s%s",
                strNotaryID->Get(),
                PathSeparator(),  // server appends ".1"
                                  // or ".PUBLIC" here.
                strInstrumentDefinitionID->Get(),
                szAppend);
        else
            m_strFilename->Format(
                "%s%s%s",
                strNotaryID->Get(),
                PathSeparator(),
                strInstrumentDefinitionID->Get());  // client uses only
                                                    // instrument definition
                                                    // id, no append.
    }

    auto strFilename = String::Factory();
    if (nullptr != szAppend)
        strFilename->Format(
            "%s%s",
            strInstrumentDefinitionID->Get(),
            szAppend);  // server side
    else
        strFilename =
            String::Factory(strInstrumentDefinitionID->Get());  // client side

    const char* szFolder1name = api_.Legacy().Mint();  // "mints"
    const char* szFolder2name = strNotaryID->Get();    // "mints/NOTARY_ID"
    const char* szFilename =
        strFilename
            ->Get();  // "mints/NOTARY_ID/INSTRUMENT_DEFINITION_ID<szAppend>"

    if (!OTDB::Exists(
            api_,
            api_.DataFolder(),
            szFolder1name,
            szFolder2name,
            szFilename,
            "")) {
        LogDetail(OT_METHOD)(__FUNCTION__)(": File does not exist: ")(
            szFolder1name)(PathSeparator())(szFolder2name)(PathSeparator())(
            szFilename)
            .Flush();
        return false;
    }

    std::string strFileContents(OTDB::QueryPlainString(
        api_,
        api_.DataFolder(),
        szFolder1name,
        szFolder2name,
        szFilename,
        ""));  // <=== LOADING FROM
               // DATA STORE.

    if (strFileContents.length() < 2) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Error reading file: ")(
            szFolder1name)(PathSeparator())(szFolder2name)(PathSeparator())(
            szFilename)(".")
            .Flush();
        return false;
    }

    // NOTE: No need to worry about the OT ARMORED file format, since
    // LoadContractFromString already handles that internally.

    auto strRawFile = String::Factory(strFileContents.c_str());

    bool bSuccess = LoadContractFromString(
        strRawFile);  // Note: This handles OT ARMORED file format.

    return bSuccess;
}

auto Mint::SaveMint(const char* szAppend) -> bool
{
    if (!m_strFoldername->Exists()) m_strFoldername->Set(api_.Legacy().Mint());

    const auto strNotaryID = String::Factory(m_NotaryID),
               strInstrumentDefinitionID =
                   String::Factory(m_InstrumentDefinitionID);

    if (!m_strFilename->Exists()) {
        if (nullptr != szAppend)
            m_strFilename->Format(
                "%s%s%s%s",
                strNotaryID->Get(),
                PathSeparator(),  // server side
                strInstrumentDefinitionID->Get(),
                szAppend);
        else
            m_strFilename->Format(
                "%s%s%s",
                strNotaryID->Get(),
                PathSeparator(),
                strInstrumentDefinitionID->Get());  // client side
    }

    auto strFilename = String::Factory();
    if (nullptr != szAppend)
        strFilename->Format("%s%s", strInstrumentDefinitionID->Get(), szAppend);
    else
        strFilename = String::Factory(strInstrumentDefinitionID->Get());

    const char* szFolder1name = api_.Legacy().Mint();
    const char* szFolder2name = strNotaryID->Get();
    const char* szFilename = strFilename->Get();

    auto strRawFile = String::Factory();

    if (!SaveContractRaw(strRawFile)) {
        LogOutput(OT_METHOD)(__FUNCTION__)(
            ": Error saving Mintfile (to string): ")(szFolder1name)(
            PathSeparator())(szFolder2name)(PathSeparator())(szFilename)(".")
            .Flush();
        return false;
    }

    auto strFinal = String::Factory();
    auto ascTemp = Armored::Factory(strRawFile);

    if (false ==
        ascTemp->WriteArmoredString(strFinal, m_strContractType->Get())) {
        LogOutput(OT_METHOD)(__FUNCTION__)(
            ": Error saving mint (Failed writing armored "
            "string): ")(szFolder1name)(PathSeparator())(szFolder2name)(
            PathSeparator())(szFilename)(".")
            .Flush();
        return false;
    }

    bool bSaved = OTDB::StorePlainString(
        api_,
        strFinal->Get(),
        api_.DataFolder(),
        szFolder1name,
        szFolder2name,
        szFilename,
        "");  // <=== SAVING TO LOCAL DATA STORE.
    if (!bSaved) {
        if (nullptr != szAppend)
            LogOutput(OT_METHOD)(__FUNCTION__)(": Error writing to file: ")(
                szFolder1name)(PathSeparator())(szFolder2name)(PathSeparator())(
                szFilename)(szAppend)(".")
                .Flush();
        else
            LogOutput(OT_METHOD)(__FUNCTION__)(": Error writing to file: ")(
                szFolder1name)(PathSeparator())(szFolder2name)(PathSeparator())(
                szFilename)(".")
                .Flush();

        return false;
    }

    return true;
}

// Make sure this contract checks out. Very high level.
// Verifies ID and signature.
auto Mint::VerifyMint(const identity::Nym& theOperator) -> bool
{
    // Make sure that the supposed Contract ID that was set is actually
    // a hash of the contract file, signatures and all.
    if (!VerifyContractID()) {
        LogOutput(OT_METHOD)(__FUNCTION__)(
            ": Error comparing Mint ID to Asset Contract ID.")
            .Flush();
        return false;
    } else if (!VerifySignature(theOperator)) {
        LogOutput(OT_METHOD)(__FUNCTION__)(
            ": Error verifying signature on mint.")
            .Flush();
        return false;
    }

    LogDebug(OT_METHOD)(__FUNCTION__)(": We now know that...").Flush();
    LogDebug(OT_METHOD)(__FUNCTION__)(": 1. The Asset Contract ID matches the "
                                      "Mint ID loaded from the Mint file.")
        .Flush();
    LogDebug(OT_METHOD)(__FUNCTION__)(": 2. The SIGNATURE VERIFIED.").Flush();
    return true;
}

// Unlike other contracts, which calculate their ID from a hash of the file
// itself, a mint has
// the same ID as its Asset Contract.  When we open the Mint file, we read the
// Instrument Definition ID
// from it and then verify that it matches what we were expecting from the asset
// type.
auto Mint::VerifyContractID() const -> bool
{
    // I use the == operator here because there is no != operator at this time.
    // That's why you see the ! outside the parenthesis.
    if (!(m_ID == m_InstrumentDefinitionID)) {
        auto str1 = String::Factory(m_ID),
             str2 = String::Factory(m_InstrumentDefinitionID);

        LogOutput(OT_METHOD)(__FUNCTION__)(
            ": Mint ID does NOT match Instrument Definition. ")(str1)(" | ")(
            str2)(".")
            .Flush();
        //                "\nRAW FILE:\n--->" << m_strRawFile << "<---"
        return false;
    } else {
        auto str1 = String::Factory(m_ID);
        LogVerbose(OT_METHOD)(__FUNCTION__)(
            ": Mint ID *SUCCESSFUL* match to Asset Contract ID: ")(str1)
            .Flush();
        return true;
    }
}

// The mint has a different key pair for each denomination.
// Pass in the actual denomination such as 5, 10, 20, 50, 100...
auto Mint::GetPrivate(Armored& theArmor, const Amount& lDenomination) const
    -> bool
{
    try {
        theArmor.Set(m_mapPrivate.at(denomination));

        return true;
    } catch (...) {
        LogTrace(OT_METHOD)(__FUNCTION__)(": Denomination ")
            (denomination.str())(" not found")
            .Flush();

        return false;
    }
}

// The mint has a different key pair for each denomination.
// Pass in the actual denomination such as 5, 10, 20, 50, 100...
auto Mint::GetPublic(Armored& theArmor, const Amount& lDenomination) const
    -> bool
{
    try {
        theArmor.Set(m_mapPublic.at(denomination));

        return true;
    } catch (...) {
        LogTrace(OT_METHOD)(__FUNCTION__)(": Denomination ")
            (denomination.str())(" not found")
            .Flush();

        return false;
    }
}

// If you need to withdraw a specific amount, pass it in here and the
// mint will return the largest denomination that is equal to or smaller
// than the amount.
// Then you can subtract the denomination from the amount and call this method
// again, and again, until it reaches 0, in order to create all the necessary
// tokens to reach the full withdrawal amount.
auto Mint::GetLargestDenomination(const Amount& amount) const -> OTAmount
{
    for (std::int32_t nIndex = GetDenominationCount() - 1; nIndex >= 0;
         nIndex--) {
        const OTAmount denomination =
            api_.Factory().Amount(GetDenomination(nIndex));

        if (denomination <= amount) return denomination;
    }

    return api_.Factory().Amount();
}

// If you call GetDenominationCount, you can then use this method
// to look up a denomination by index.
// You could also iterate through them by index.
auto Mint::GetDenomination(std::int32_t nIndex) const -> OTAmount
{
    // index out of bounds.
    if (nIndex > (m_nDenominationCount - 1)) { return api_.Factory().Amount(); }

    std::int32_t nIterateIndex = 0;

    for (auto it = m_mapPublic.begin(); it != m_mapPublic.end();
         ++it, nIterateIndex++) {

        if (nIndex == nIterateIndex) return it->first;
    }

    return api_.Factory().Amount();
}

// The default behavior of this function does NOT save the private keys. It only
// serializes the public keys, and it is safe to send the object to the client.
// If the server needs to save the private keys, then call SetSavePrivateKeys()
// first.
void Mint::UpdateContents(const PasswordPrompt& reason)
{
    auto NOTARY_ID = String::Factory(m_NotaryID),
         NOTARY_NYM_ID = String::Factory(m_ServerNymID),
         INSTRUMENT_DEFINITION_ID = String::Factory(m_InstrumentDefinitionID),
         CASH_ACCOUNT_ID = String::Factory(m_CashAccountID);

    // I release this because I'm about to repopulate it.
    m_xmlUnsigned->Release();

    Tag tag("mint");

    tag.add_attribute("version", m_strVersion->Get());
    tag.add_attribute("notaryID", NOTARY_ID->Get());
    tag.add_attribute("serverNymID", NOTARY_NYM_ID->Get());
    tag.add_attribute(
        "instrumentDefinitionID", INSTRUMENT_DEFINITION_ID->Get());
    tag.add_attribute("cashAcctID", CASH_ACCOUNT_ID->Get());
    tag.add_attribute("series", std::to_string(m_nSeries));
    tag.add_attribute("expiration", formatTimestamp(m_EXPIRATION));
    tag.add_attribute("validFrom", formatTimestamp(m_VALID_FROM));
    tag.add_attribute("validTo", formatTimestamp(m_VALID_TO));

    if (m_nDenominationCount) {
        if (m_bSavePrivateKeys) {
            m_bSavePrivateKeys = false;  // reset this back to false again. Use
                                         // SetSavePrivateKeys() to set it true.

            for (auto& it : m_mapPrivate) {
                TagPtr tagPrivateInfo(
                    new Tag("mintPrivateInfo", it.second->Get()));
                tagPrivateInfo->add_attribute(
                    "denomination", it.first->str().c_str());
                tag.add_tag(tagPrivateInfo);
            }
        }
        for (auto& it : m_mapPublic) {
            TagPtr tagPublicInfo(new Tag("mintPublicInfo", it.second->Get()));
            tagPublicInfo->add_attribute(
                "denomination", it.first->str().c_str());
            tag.add_tag(tagPublicInfo);
        }
    }

    std::string str_result;
    tag.output(str_result);

    m_xmlUnsigned->Concatenate("%s", str_result.c_str());
}

// return -1 if error, 0 if nothing, and 1 if the node was processed.
auto Mint::ProcessXMLNode(irr::io::IrrXMLReader*& xml) -> std::int32_t
{
    std::int32_t nReturnVal = 0;

    const auto strNodeName = String::Factory(xml->getNodeName());

    // Here we call the parent class first.
    // If the node is found there, or there is some error,
    // then we just return either way.  But if it comes back
    // as '0', then nothing happened, and we'll continue executing.
    //
    // -- Note you can choose not to call the parent if
    // you don't want to use any of those xml tags.
    // As I do below, in the case of OTAccount.
    // if (nReturnVal = ot_super::ProcessXMLNode(xml))
    //    return nReturnVal;

    if (strNodeName->Compare("mint")) {
        auto strNotaryID = String::Factory(),
             strServerNymID = String::Factory(),
             strInstrumentDefinitionID = String::Factory(),
             strCashAcctID = String::Factory();

        m_strVersion = String::Factory(xml->getAttributeValue("version"));
        strNotaryID = String::Factory(xml->getAttributeValue("notaryID"));
        strServerNymID = String::Factory(xml->getAttributeValue("serverNymID"));
        strInstrumentDefinitionID =
            String::Factory(xml->getAttributeValue("instrumentDefinitionID"));
        strCashAcctID = String::Factory(xml->getAttributeValue("cashAcctID"));

        m_nSeries = atoi(xml->getAttributeValue("series"));
        m_EXPIRATION = parseTimestamp(xml->getAttributeValue("expiration"));

        m_VALID_FROM = parseTimestamp(xml->getAttributeValue("validFrom"));
        m_VALID_TO = parseTimestamp(xml->getAttributeValue("validTo"));

        m_NotaryID->SetString(strNotaryID);
        m_ServerNymID->SetString(strServerNymID);
        m_InstrumentDefinitionID->SetString(strInstrumentDefinitionID);
        m_CashAccountID->SetString(strCashAcctID);

        LogDetail(OT_METHOD)(__FUNCTION__)
            //    "\n===> Loading XML for mint into memory structures..."
            (": Mint version: ")(m_strVersion)(" Notary ID: ")(strNotaryID)(
                " Instrument Definition ID: ")(strInstrumentDefinitionID)(
                " Cash Acct ID: ")(strCashAcctID)(
                (m_CashAccountID->empty()) ? "FAILURE" : "SUCCESS")(
                " loading Cash Account into memory for pointer: ")(
                "Mint::m_pReserveAcct ")(" Series: ")(m_nSeries)(
                " Expiration: ")(m_EXPIRATION)(" Valid From: ")(m_VALID_FROM)(
                " Valid To: ")(m_VALID_TO)
                .Flush();

        nReturnVal = 1;
    } else if (strNodeName->Compare("mintPrivateInfo")) {
        OTAmount denomination =
            api_.Factory().Amount(xml->getAttributeValue("denomination"));
        auto pArmor = Armored::Factory();

        if (!Contract::LoadEncodedTextField(xml, pArmor) || !pArmor->Exists()) {
            LogOutput(OT_METHOD)(__FUNCTION__)(": Error: mintPrivateInfo field "
                                               "without value.")
                .Flush();

            return (-1);  // error condition
        } else {
            LogTrace(OT_METHOD)(__FUNCTION__)(
                ": Loading private key for denomination ")(denomination->str())
                .Flush();
            m_mapPrivate.emplace(std::move(denomination), std::move(pArmor));
        }

        return 1;
    } else if (strNodeName->Compare("mintPublicInfo")) {
        OTAmount denomination =
            api_.Factory().Amount(xml->getAttributeValue("denomination"));

        auto pArmor = Armored::Factory();

        if (!Contract::LoadEncodedTextField(xml, pArmor) || !pArmor->Exists()) {
            LogOutput(OT_METHOD)(__FUNCTION__)(": Error: mintPublicInfo field "
                                               "without value.")
                .Flush();

            return (-1);  // error condition
        } else {
            LogTrace(OT_METHOD)(__FUNCTION__)(
                ": Loading public key for denomination ")(denomination->str())
                .Flush();
            m_mapPublic.emplace(std::move(denomination), std::move(pArmor));

            // Whether client or server, both sides have public. Each public
            // denomination should increment this count.
            m_nDenominationCount++;
        }

        return 1;
    }

    return nReturnVal;
}

/*
 // Just make sure theMessage has these members populated:
 //
 // theMessage.m_strNymID;
 // theMessage.m_strInstrumentDefinitionID;
 // theMessage.m_strNotaryID;

 // static method (call it without an instance, using notation:
 OTAccount::GenerateNewAccount)
 OTAccount * OTAccount::GenerateNewAccount(    const Identifier& theNymID,
 const identifier::Server& theNotaryID,
                                            const Nym & theServerNym,
 const OTMessage & theMessage,
                                            const OTAccount::AccountType
 eAcctType=OTAccount::user)


 // The above method uses this one internally...
 bool OTAccount::GenerateNewAccount(const Nym & theServer, const
 OTMessage & theMessage,
                                    const OTAccount::AccountType
 eAcctType=OTAccount::user)


 OTAccount * pAcct = nullptr;
 pAcct = OTAccount::LoadExistingAccount(ACCOUNT_ID, NOTARY_ID);
 */

// Lucre step 1: generate new mint
// Make sure the issuer here has a private key
// theMint.GenerateNewMint(nSeries, VALID_FROM, VALID_TO,
// INSTRUMENT_DEFINITION_ID, m_nymServer,
// 1, 5, 10, 20, 50, 100, 500, 1000, 10000, 100000);
void Mint::GenerateNewMint(
    const api::Wallet& wallet,
    const std::int32_t nSeries,
    const Time VALID_FROM,
    const Time VALID_TO,
    const Time MINT_EXPIRATION,
    const identifier::UnitDefinition& theInstrumentDefinitionID,
    const identifier::Server& theNotaryID,
    const identity::Nym& theNotary,
    const Amount& nDenom1,
    const Amount& nDenom2,
    const Amount& nDenom3,
    const Amount& nDenom4,
    const Amount& nDenom5,
    const Amount& nDenom6,
    const Amount& nDenom7,
    const Amount& nDenom8,
    const Amount& nDenom9,
    const Amount& nDenom10,
    const std::size_t keySize,
    const PasswordPrompt& reason)
{
    Release();
    m_InstrumentDefinitionID = theInstrumentDefinitionID;
    m_NotaryID = theNotaryID;
    const auto& NOTARY_NYM_ID = theNotary.ID();
    m_ServerNymID = NOTARY_NYM_ID;
    m_nSeries = nSeries;
    m_VALID_FROM = VALID_FROM;
    m_VALID_TO = VALID_TO;
    m_EXPIRATION = MINT_EXPIRATION;
    auto account = wallet.CreateAccount(
        NOTARY_NYM_ID,
        theNotaryID,
        theInstrumentDefinitionID,
        theNotary,
        Account::mint,
        0,
        reason);

    if (account) {
        account.get().GetIdentifier(m_CashAccountID);
        LogDetail(OT_METHOD)(__FUNCTION__)(
            ": Successfully created cash reserve account for new mint.")
            .Flush();
    } else {
        LogNormal(OT_METHOD)(__FUNCTION__)(
            ": Error creating cash reserve account for new mint.")
            .Flush();
    }

    account.Release();

    if (nDenom1 != api_.Factory().Amount()) { AddDenomination(theNotary, nDenom1, keySize, reason); }
    if (nDenom2 != api_.Factory().Amount()) { AddDenomination(theNotary, nDenom2, keySize, reason); }
    if (nDenom3 != api_.Factory().Amount()) { AddDenomination(theNotary, nDenom3, keySize, reason); }
    if (nDenom4 != api_.Factory().Amount()) { AddDenomination(theNotary, nDenom4, keySize, reason); }
    if (nDenom5 != api_.Factory().Amount()) { AddDenomination(theNotary, nDenom5, keySize, reason); }
    if (nDenom6 != api_.Factory().Amount()) { AddDenomination(theNotary, nDenom6, keySize, reason); }
    if (nDenom7 != api_.Factory().Amount()) { AddDenomination(theNotary, nDenom7, keySize, reason); }
    if (nDenom8 != api_.Factory().Amount()) { AddDenomination(theNotary, nDenom8, keySize, reason); }
    if (nDenom9 != api_.Factory().Amount()) { AddDenomination(theNotary, nDenom9, keySize, reason); }
    if (nDenom10 != api_.Factory().Amount()) {
        AddDenomination(theNotary, nDenom10, keySize, reason);
    }
}

Mint::~Mint() { Release_Mint(); }
}  // namespace opentxs::blind::mint::implementation
