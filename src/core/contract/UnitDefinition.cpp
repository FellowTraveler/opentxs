// Copyright (c) 2010-2020 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include "0_stdafx.hpp"                      // IWYU pragma: associated
#include "1_Internal.hpp"                    // IWYU pragma: associated
#include "core/contract/UnitDefinition.hpp"  // IWYU pragma: associated

#include <algorithm>
#include <cctype>
#include <cmath>  // IWYU pragma: keep
#include <cstdint>
#include <deque>
#include <fstream>
#include <iomanip>
#include <list>
#include <memory>
#include <set>
#include <utility>

#include "2_Factory.hpp"
#include "internal/api/Api.hpp"
#include "internal/core/contract/Contract.hpp"
#include "opentxs/Pimpl.hpp"
#include "opentxs/Shared.hpp"
#include "opentxs/api/Factory.hpp"
#include "opentxs/api/Legacy.hpp"
#include "opentxs/api/Wallet.hpp"
#include "opentxs/core/Account.hpp"
#include "opentxs/core/AccountVisitor.hpp"
#include "opentxs/core/Data.hpp"
#include "opentxs/core/Log.hpp"
#include "opentxs/core/LogSource.hpp"
#include "opentxs/core/OTStorage.hpp"
#include "opentxs/core/String.hpp"
#include "opentxs/core/identifier/Server.hpp"
#include "opentxs/core/identifier/UnitDefinition.hpp"
#include "opentxs/identity/Nym.hpp"
#include "opentxs/protobuf/Check.hpp"
#include "opentxs/protobuf/Contact.hpp"
#include "opentxs/protobuf/ContractEnums.pb.h"
#include "opentxs/protobuf/Enums.pb.h"
#include "opentxs/protobuf/Nym.pb.h"
#include "opentxs/protobuf/Signature.pb.h"
#include "opentxs/protobuf/UnitDefinition.pb.h"
#include "opentxs/protobuf/verify/UnitDefinition.hpp"

#define OT_METHOD "opentxs::contract::implementation::Unit::"

inline auto separateThousands(
    std::stringstream& sss,
    std::int64_t value,
    const char* szSeparator) -> void
{
    if (value < 1000) {
        sss << value;
        return;
    }

    separateThousands(sss, value / 1000, szSeparator);
    sss << szSeparator << std::setfill('0') << std::setw(3) << value % 1000;
}

namespace opentxs
{
auto Factory::UnitDefinition(const api::Core& api) noexcept
    -> std::shared_ptr<contract::Unit>
{
    return std::make_shared<contract::blank::Unit>(api);
}

auto Factory::UnitDefinition(
    const api::internal::Core& api,
    const Nym_p& nym,
    const proto::UnitDefinition serialized) noexcept
    -> std::shared_ptr<contract::Unit>
{
    switch (serialized.type()) {
        case proto::UNITTYPE_CURRENCY: {

            return CurrencyContract(api, nym, serialized);
        }
        case proto::UNITTYPE_SECURITY: {

            return SecurityContract(api, nym, serialized);
        }
        case proto::UNITTYPE_BASKET: {

            return BasketContract(api, nym, serialized);
        }
        case proto::UNITTYPE_ERROR:
        default: {
            return {};
        }
    }
}
}  // namespace opentxs

namespace opentxs::contract
{
const VersionNumber Unit::DefaultVersion{2};
const VersionNumber Unit::MaxVersion{2};

auto Unit::formatLongAmount(
    const Amount& lValue,
    std::int32_t nFactor,
    std::int32_t nPower,
    const char* szCurrencySymbol,
    const char* szThousandSeparator,
    const char* szDecimalPoint) -> std::string
{
    std::stringstream sss;

    // Handle negative values
    if (lValue < 0) {
        sss << "-";
        lValue = -lValue;
    }

    if (nullptr != szCurrencySymbol) sss << szCurrencySymbol << " ";

    // For example, if 506 is supposed to be $5.06, then dividing by a factor of
    // 100 results in 5 dollars (integer value) and 6 cents (fractional value).

    // Handle integer value with thousand separaters
    separateThousands(sss, lValue / nFactor, szThousandSeparator);

    // Handle fractional value
    if (1 < nFactor) {
        sss << szDecimalPoint << std::setfill('0') << std::setw(nPower)
            << (lValue % nFactor);
    }

    std::string str_result(sss.str());

    return str_result;
}

auto Unit::ParseFormatted(
    std::int64_t& lResult,
    const std::string& str_input,
    std::int32_t nFactor,
    std::int32_t nPower,
    const char* szThousandSeparator,
    const char* szDecimalPoint) -> bool
{
    OT_ASSERT(nullptr != szThousandSeparator);
    OT_ASSERT(nullptr != szDecimalPoint);

    lResult = 0;

    char theSeparator = szThousandSeparator[0];
    char theDecimalPoint = szDecimalPoint[0];

    std::int64_t lDollars = 0;
    std::int64_t lCents = 0;
    std::int64_t lOutput = 0;
    std::int64_t lSign = 1;

    bool bHasEnteredDollars = false;
    bool bHasEnteredCents = false;

    std::int32_t nDigitsCollectedBeforeDot = 0;
    std::int32_t nDigitsCollectedAfterDot = 0;

    // BUG: &mp isn't used.
    // const std::moneypunct<char, false> &mp = std::use_facet<
    // std::moneypunct<char, false> >(std::locale ());

    std::deque<std::int64_t> deque_cents;

    for (std::uint32_t uIndex = 0; uIndex < str_input.length(); ++uIndex) {
        char theChar = str_input[uIndex];

        if (iscntrl(theChar))  // Break at any newline or other control
                               // character.
            break;

        if (0 == isdigit(theChar))  // if it's not a numerical digit.
        {
            if (theSeparator == theChar) continue;

            if (theDecimalPoint == theChar) {
                if (bHasEnteredCents) {
                    // There shouldn't be ANOTHER decimal point if we are
                    // already in the cents.
                    // Therefore, we're done here. Break.
                    //
                    break;
                }

                // If we HAVEN'T entered the cents yet, then this decimal point
                // marks the spot where we DO.
                //
                bHasEnteredDollars = true;
                bHasEnteredCents = true;
                continue;
            }  // theChar is the decimal point

            // Once a negative sign appears, it's negative, period.
            // If you put two or three negative signs in a row, it's STILL
            // negative.

            if ('-' == theChar) {
                lSign = -1;
                continue;
            }

            // Okay, by this point, we know it's not numerical, and it's not a
            // separator or decimal point, or sign. We allow letters and symbols
            // BEFORE the numbers start, but not AFTER (that would terminate the
            // number.) Therefore we need to see if the dollars or cents have
            // started yet. If they have, then this is the end, and we break.
            // Otherwise if they haven't, then we're still at the beginning, so
            // we continue.
            if (bHasEnteredDollars || bHasEnteredCents)
                break;
            else
                continue;
        }  // not numerical

        // By this point, we KNOW that it's a numeric digit. Are we collecting
        // cents yet? How about dollars? Also, if nPower is 2, then we only
        // collect 2 digits after the decimal point. If we've already collected
        // those, then we need to break.
        if (bHasEnteredCents) {
            ++nDigitsCollectedAfterDot;

            // If "cents" occupy 2 digits after the decimal point, and we are
            // now on the THIRD digit -- then we're done.
            if (nDigitsCollectedAfterDot > nPower) break;

            // Okay, we're in the cents, so let's add this digit...
            deque_cents.push_back(static_cast<std::int64_t>(theChar - '0'));

            continue;
        }

        // Okay, it's a digit, and we haven't started processing cents yet. How
        // about dollars?
        if (!bHasEnteredDollars) bHasEnteredDollars = true;

        ++nDigitsCollectedBeforeDot;

        // Let's add this digit...
        lDollars *=
            10;  // Multiply existing dollars by 10, and then add the new digit.
        lDollars += static_cast<std::int64_t>(theChar - '0');
    }

    // Time to put it all together...
    lOutput += lDollars;
    lOutput *=
        static_cast<std::int64_t>(nFactor);  // 1 dollar becomes 100 cents.

    std::int32_t nTempPower = nPower;

    while (nTempPower > 0) {
        --nTempPower;

        if (deque_cents.size() > 0) {
            lCents += deque_cents.front();
            deque_cents.pop_front();
        }

        lCents *= 10;
    }
    lCents /= 10;  // There won't be any rounding errors here, since the last
                   // thing we did in the loop was multiply by 10.

    lOutput += lCents;

    lResult = (lOutput * lSign);

    return true;
}

auto Unit::ValidUnits(const VersionNumber version) noexcept
    -> std::set<proto::ContactItemType>
{
    try {

        return proto::AllowedItemTypes().at(
            {implementation::Unit::unit_of_account_version_map_.at(version),
             proto::CONTACTSECTION_CONTRACT});

    } catch (...) {

        return {};
    }
}
}  // namespace opentxs::contract

namespace opentxs::contract::implementation
{
const std::map<VersionNumber, VersionNumber> Unit::unit_of_account_version_map_{
    {2, 6}};

Unit::Unit(
    const api::internal::Core& api,
    const Nym_p& nym,
    const std::string& shortname,
    const std::string& name,
    const std::string& symbol,
    const std::string& terms,
    const proto::ContactItemType unitOfAccount,
    const VersionNumber version)
    : Signable(
          api,
          nym,
          version,
          terms,
          shortname,
          api.Factory().Identifier(),
          {})
    , primary_unit_symbol_(symbol)
    , unit_of_account_(unitOfAccount)
    , primary_unit_name_(name)
    , short_name_(shortname)
{
}

Unit::Unit(
    const api::internal::Core& api,
    const Nym_p& nym,
    const SerializedType serialized)
    : Signable(
          api,
          nym,
          serialized.version(),
          serialized.terms(),
          serialized.shortname(),
          api.Factory().Identifier(serialized.id()),
          serialized.has_signature()
              ? Signatures{std::make_shared<proto::Signature>(
                    serialized.signature())}
              : Signatures{})
    , primary_unit_symbol_(serialized.symbol())
    , unit_of_account_(serialized.unitofaccount())
    , primary_unit_name_(serialized.name())
    , short_name_(serialized.shortname())
{
}

Unit::Unit(const Unit& rhs)
    : Signable(rhs)
    , primary_unit_symbol_(rhs.primary_unit_symbol_)
    , unit_of_account_(rhs.unit_of_account_)
    , primary_unit_name_(rhs.primary_unit_name_)
    , short_name_(rhs.short_name_)
{
}

auto Unit::AddAccountRecord(
    const std::string& dataFolder,
    const Account& theAccount) const -> bool
{
    Lock lock(lock_);

    if (theAccount.GetInstrumentDefinitionID() != id_) {
        LogOutput(OT_METHOD)(__FUNCTION__)(
            ": Error: theAccount doesn't have the same asset "
            "type ID as *this does.")
            .Flush();
        return false;
    }

    const auto theAcctID = Identifier::Factory(theAccount);
    const auto strAcctID = String::Factory(theAcctID);

    const auto strInstrumentDefinitionID = String::Factory(id(lock));
    auto strAcctRecordFile = String::Factory();
    strAcctRecordFile->Format("%s.a", strInstrumentDefinitionID->Get());

    OTDB::Storable* pStorable = nullptr;
    std::unique_ptr<OTDB::Storable> theAngel;
    OTDB::StringMap* pMap = nullptr;

    if (OTDB::Exists(
            api_,
            dataFolder,
            api_.Legacy().Contract(),
            strAcctRecordFile->Get(),
            "",
            ""))  // the file already exists; let's
                  // try to load it up.
        pStorable = OTDB::QueryObject(
            api_,
            OTDB::STORED_OBJ_STRING_MAP,
            dataFolder,
            api_.Legacy().Contract(),
            strAcctRecordFile->Get(),
            "",
            "");
    else  // the account records file (for this instrument definition) doesn't
          // exist.
        pStorable = OTDB::CreateObject(
            OTDB::STORED_OBJ_STRING_MAP);  // this asserts already, on failure.

    theAngel.reset(pStorable);
    pMap = (nullptr == pStorable) ? nullptr
                                  : dynamic_cast<OTDB::StringMap*>(pStorable);

    // It exists.
    //
    if (nullptr == pMap) {
        LogOutput(OT_METHOD)(__FUNCTION__)(
            ": Error: Failed trying to load or create the account records "
            "file for instrument definition: ")(strInstrumentDefinitionID)(".")
            .Flush();
        return false;
    }

    auto& theMap = pMap->the_map;
    auto map_it = theMap.find(strAcctID->Get());

    if (theMap.end() != map_it)  // we found it.
    {                            // We were ADDING IT, but it was ALREADY THERE.
        // (Thus, we're ALREADY DONE.)
        // Let's just make sure the right instrument definition ID is associated
        // with this
        // account
        // (it better be, since we loaded the account records file based on the
        // instrument definition ID as its filename...)
        //
        const std::string& str2 = map_it->second;  // Containing the instrument
                                                   // definition ID. (Just in
                                                   // case
        // someone copied the wrong file here,
        // --------------------------------          // every account should map
        // to the SAME instrument definition id.)

        if (false ==
            strInstrumentDefinitionID->Compare(str2.c_str()))  // should
                                                               // never
        // happen.
        {
            LogOutput(OT_METHOD)(__FUNCTION__)(
                ": Error: wrong instrument definition found in "
                "account records "
                "file. For instrument definition: ")(strInstrumentDefinitionID)(
                ". For account: ")(strAcctID)(
                ". Found wrong instrument definition: ")(str2)(".")
                .Flush();
            return false;
        }

        return true;  // already there (no need to add.) + the instrument
                      // definition ID
                      // matches.
    }

    // it wasn't already on the list...

    // ...so add it.
    //
    theMap[strAcctID->Get()] = strInstrumentDefinitionID->Get();

    // Then save it back to local storage:
    //
    if (!OTDB::StoreObject(
            api_,
            *pMap,
            dataFolder,
            api_.Legacy().Contract(),
            strAcctRecordFile->Get(),
            "",
            "")) {
        LogOutput(OT_METHOD)(__FUNCTION__)(
            ": Failed trying to StoreObject, while saving updated "
            "account records file for instrument definition: ")(
            strInstrumentDefinitionID)(" to contain account ID: ")(strAcctID)(
            ".")
            .Flush();
        return false;
    }

    // Okay, we saved the updated file, with the account added. (done, success.)
    //
    return true;
}

auto Unit::Contract() const -> SerializedType
{
    Lock lock(lock_);

    return contract(lock);
}

auto Unit::contract(const Lock& lock) const -> SerializedType
{
    auto contract = SigVersion(lock);

    if (1 <= signatures_.size()) {
        *(contract.mutable_signature()) = *(signatures_.front());
    }

    return contract;
}

auto Unit::DisplayStatistics(String& strContents) const -> bool
{
    std::string type = "error";

    switch (Type()) {
        case proto::UNITTYPE_CURRENCY:
            type = "error";

            break;
        case proto::UNITTYPE_SECURITY:
            type = "security";

            break;
        case proto::UNITTYPE_BASKET:
            type = "basket currency";

            break;
        default:
            break;
    }

    strContents.Concatenate(
        " Asset Type:  %s\n"
        " InstrumentDefinitionID: %s\n"
        "\n",
        type.c_str(),
        id_->str().c_str());
    return true;
}

auto Unit::EraseAccountRecord(
    const std::string& dataFolder,
    const Identifier& theAcctID) const -> bool
{
    Lock lock(lock_);

    const auto strAcctID = String::Factory(theAcctID);

    const auto strInstrumentDefinitionID = String::Factory(id(lock));
    auto strAcctRecordFile = String::Factory();
    strAcctRecordFile->Format("%s.a", strInstrumentDefinitionID->Get());

    OTDB::Storable* pStorable = nullptr;
    std::unique_ptr<OTDB::Storable> theAngel;
    OTDB::StringMap* pMap = nullptr;

    if (OTDB::Exists(
            api_,
            dataFolder,
            api_.Legacy().Contract(),
            strAcctRecordFile->Get(),
            "",
            ""))  // the file already exists; let's
                  // try to load it up.
        pStorable = OTDB::QueryObject(
            api_,
            OTDB::STORED_OBJ_STRING_MAP,
            dataFolder,
            api_.Legacy().Contract(),
            strAcctRecordFile->Get(),
            "",
            "");
    else  // the account records file (for this instrument definition) doesn't
          // exist.
        pStorable = OTDB::CreateObject(
            OTDB::STORED_OBJ_STRING_MAP);  // this asserts already, on failure.

    theAngel.reset(pStorable);
    pMap = (nullptr == pStorable) ? nullptr
                                  : dynamic_cast<OTDB::StringMap*>(pStorable);

    // It exists.
    //
    if (nullptr == pMap) {
        LogOutput(OT_METHOD)(__FUNCTION__)(
            ": Error: Failed trying to load or create the account records "
            "file for instrument definition: ")(strInstrumentDefinitionID)(".")
            .Flush();
        return false;
    }

    // Before we can erase it, let's see if it's even there....
    //
    auto& theMap = pMap->the_map;
    auto map_it = theMap.find(strAcctID->Get());

    // we found it!
    if (theMap.end() != map_it)  //  Acct ID was already there...
    {
        theMap.erase(map_it);  // remove it
    }

    // it wasn't already on the list...
    // (So it's like success, since the end result is, acct ID will not appear
    // on this list--whether
    // it was there or not beforehand, it's definitely not there now.)

    // Then save it back to local storage:
    //
    if (!OTDB::StoreObject(
            api_,
            *pMap,
            dataFolder,
            api_.Legacy().Contract(),
            strAcctRecordFile->Get(),
            "",
            "")) {
        LogOutput(OT_METHOD)(__FUNCTION__)(
            ": Failed trying to StoreObject, while saving updated "
            "account records file for instrument definition: ")(
            strInstrumentDefinitionID)(" to erase account ID: ")(strAcctID)(".")
            .Flush();
        return false;
    }

    // Okay, we saved the updated file, with the account removed. (done,
    // success.)
    //
    return true;
}

// Convert 912545 to "$9,125.45"
//
// (Assuming a Factor of 100, Decimal Power of 2, Currency Symbol of "$",
//  separator of "," and decimal point of ".")
auto Unit::FormatAmountLocale(
    const Amount& amount,
    std::string& str_output,
    const std::string& str_thousand,
    const std::string& str_decimal) const -> bool
{
    // Lookup separator and decimal point symbols based on locale.

    // Get a moneypunct facet from the global ("C") locale
    //
    // NOTE: Turns out moneypunct kind of sucks.
    // As a result, for internationalization purposes,
    // these values have to be set here before compilation.
    //
    auto strSeparator =
        String::Factory(str_thousand.empty() ? OT_THOUSANDS_SEP : str_thousand);
    auto strDecimalPoint =
        String::Factory(str_decimal.empty() ? OT_DECIMAL_POINT : str_decimal);

    // NOTE: from web searching, I've determined that locale / moneypunct has
    // internationalization problems. Therefore it looks like if you want to
    // build OT for various languages / regions, you're just going to have to
    // edit stdafx.hpp and change the OT_THOUSANDS_SEP and OT_DECIMAL_POINT
    // variables.
    //
    // The best improvement I can think on that is to check locale and then use
    // it to choose from our own list of hardcoded values. Todo.

    str_output = Unit::formatLongAmount(
        amount,
        static_cast<std::int64_t>(std::pow(10, DecimalPower())),
        DecimalPower(),
        (proto::UNITTYPE_CURRENCY == Type()) ? primary_unit_symbol_.c_str()
                                             : nullptr,
        strSeparator->Get(),
        strDecimalPoint->Get());
    return true;  // Note: might want to return false if str_output is empty.
}

// Convert 912545 to "9,125.45"
//
// (Example assumes a Factor of 100, Decimal Power of 2
//  separator of "," and decimal point of ".")
auto Unit::FormatAmountWithoutSymbolLocale(
    const Amount& amount,
    std::string& str_output,
    const std::string& str_thousand,
    const std::string& str_decimal) const -> bool
{
    // --------------------------------------------------------
    // Lookup separator and decimal point symbols based on locale.
    // --------------------------------------------------------
    // Get a moneypunct facet from the global ("C") locale
    //
    // NOTE: Turns out moneypunct kind of sucks.
    // As a result, for internationalization purposes,
    // these values have to be set here before compilation.
    //
    auto strSeparator =
        String::Factory(str_thousand.empty() ? OT_THOUSANDS_SEP : str_thousand);
    auto strDecimalPoint =
        String::Factory(str_decimal.empty() ? OT_DECIMAL_POINT : str_decimal);

    str_output = Unit::formatLongAmount(
        amount,
        static_cast<std::int64_t>(std::pow(10, DecimalPower())),
        DecimalPower(),
        nullptr,
        strSeparator->Get(),
        strDecimalPoint->Get());
    return true;  // Note: might want to return false if str_output is empty.
}

auto Unit::GetID(const Lock& lock) const -> OTIdentifier
{
    return GetID(api_, IDVersion(lock));
}

auto Unit::GetID(const api::internal::Core& api, const SerializedType& contract)
    -> OTIdentifier
{
    return api.Factory().Identifier(contract);
}

auto Unit::IDVersion(const Lock& lock) const -> SerializedType
{
    OT_ASSERT(verify_write_lock(lock));

    SerializedType contract;
    contract.set_version(version_);
    contract.clear_id();         // reinforcing that this field must be blank.
    contract.clear_signature();  // reinforcing that this field must be blank.
    contract.clear_publicnym();  // reinforcing that this field must be blank.

    if (nym_) {
        auto nymID = String::Factory();
        nym_->GetIdentifier(nymID);
        contract.set_nymid(nymID->Get());
    }

    contract.set_shortname(short_name_);
    contract.set_terms(conditions_);
    contract.set_name(primary_unit_name_);
    contract.set_symbol(primary_unit_symbol_);
    contract.set_type(Type());

    if (version_ > 1) { contract.set_unitofaccount(unit_of_account_); }

    return contract;
}

auto Unit::PublicContract() const -> SerializedType
{
    Lock lock(lock_);

    auto serialized = contract(lock);

    if (nym_) {
        auto publicNym = nym_->asPublicNym();
        *(serialized.mutable_publicnym()) = publicNym;
    }

    return serialized;
}

auto Unit::Serialize() const -> OTData
{
    Lock lock(lock_);

    return api_.Factory().Data(contract(lock));
}

auto Unit::SetAlias(const std::string& alias) -> void
{
    InitAlias(alias);
    api_.Wallet().SetUnitDefinitionAlias(
        identifier::UnitDefinition::Factory(id_->str()),
        alias);  // TODO conversion
}

auto Unit::SigVersion(const Lock& lock) const -> SerializedType
{
    auto contract = IDVersion(lock);
    contract.set_id(id(lock)->str().c_str());

    return contract;
}

// Convert "$9,125.45" to 912545.
//
// (Assuming a Factor of 100, Decimal Power of 2, separator of "," and decimal
// point of ".")
auto Unit::StringToAmountLocale(
    OTAmount& amount,
    const std::string& str_input,
    const std::string& str_thousand,
    const std::string& str_decimal) const -> bool
{
    // Lookup separator and decimal point symbols based on locale.

    // Get a moneypunct facet from the global ("C") locale
    //

    // NOTE: from web searching, I've determined that locale / moneypunct has
    // internationalization problems. Therefore it looks like if you want to
    // build OT for various languages / regions, you're just going to have to
    // edit stdafx.hpp and change the OT_THOUSANDS_SEP and OT_DECIMAL_POINT
    // variables.
    //
    // The best improvement I can think on that is to check locale and then use
    // it to choose from our own list of hardcoded values. Todo.

    auto strSeparator =
        String::Factory(str_thousand.empty() ? OT_THOUSANDS_SEP : str_thousand);
    auto strDecimalPoint =
        String::Factory(str_decimal.empty() ? OT_DECIMAL_POINT : str_decimal);

    bool bSuccess = Unit::ParseFormatted(
        amount,
        str_input,
        static_cast<std::int64_t>(std::pow(10, DecimalPower())),
        DecimalPower(),
        strSeparator->Get(),
        strDecimalPoint->Get());

    return bSuccess;
}

auto Unit::update_signature(const Lock& lock, const PasswordPrompt& reason)
    -> bool
{
    if (!Signable::update_signature(lock, reason)) { return false; }

    bool success = false;
    signatures_.clear();
    auto serialized = SigVersion(lock);
    auto& signature = *serialized.mutable_signature();
    success = nym_->Sign(
        serialized, proto::SIGROLE_UNITDEFINITION, signature, reason);

    if (success) {
        signatures_.emplace_front(new proto::Signature(signature));
    } else {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Failed to create signature.")
            .Flush();
    }

    return success;
}

auto Unit::validate(const Lock& lock) const -> bool
{
    bool validNym = false;

    if (nym_) { validNym = nym_->VerifyPseudonym(); }

    const bool validSyntax = proto::Validate(contract(lock), VERBOSE, true);

    if (1 > signatures_.size()) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Missing signature.").Flush();

        return false;
    }

    bool validSig = false;
    auto& signature = *signatures_.cbegin();

    if (signature) { validSig = verify_signature(lock, *signature); }

    return (validNym && validSyntax && validSig);
}

auto Unit::verify_signature(const Lock& lock, const proto::Signature& signature)
    const -> bool
{
    if (!Signable::verify_signature(lock, signature)) { return false; }

    auto serialized = SigVersion(lock);
    auto& sigProto = *serialized.mutable_signature();
    sigProto.CopyFrom(signature);

    return nym_->Verify(serialized, sigProto);
}

// currently only "user" accounts (normal user asset accounts) are added to
// this list Any "special" accounts, such as basket reserve accounts, or voucher
// reserve accounts, or cash reserve accounts, are not included on this list.
auto Unit::VisitAccountRecords(
    const std::string& dataFolder,
    AccountVisitor& visitor,
    const PasswordPrompt& reason) const -> bool
{
    Lock lock(lock_);
    const auto strInstrumentDefinitionID = String::Factory(id(lock));
    auto strAcctRecordFile = String::Factory();
    strAcctRecordFile->Format("%s.a", strInstrumentDefinitionID->Get());

    std::unique_ptr<OTDB::Storable> pStorable(OTDB::QueryObject(
        api_,
        OTDB::STORED_OBJ_STRING_MAP,
        dataFolder,
        api_.Legacy().Contract(),
        strAcctRecordFile->Get(),
        "",
        ""));

    auto* pMap = dynamic_cast<OTDB::StringMap*>(pStorable.get());

    // There was definitely a StringMap loaded from local storage.
    // (Even an empty one, possibly.) This is the only block that matters in
    // this function.
    //
    if (nullptr != pMap) {
        const auto& pNotaryID = visitor.GetNotaryID();

        OT_ASSERT(false == pNotaryID.empty());

        auto& theMap = pMap->the_map;

        // todo: optimize: will probably have to use a database for this,
        // std::int64_t term.
        // (What if there are a million acct IDs in this flat file? Not
        // scaleable.)
        //
        for (auto& it : theMap) {
            const std::string& str_acct_id =
                it.first;  // Containing the account ID.
            const std::string& str_instrument_definition_id =
                it.second;  // Containing the instrument definition ID. (Just in
                            // case
                            // someone copied the wrong file here...)

            if (!strInstrumentDefinitionID->Compare(
                    str_instrument_definition_id.c_str())) {
                LogOutput(OT_METHOD)(__FUNCTION__)(
                    ": Error: wrong "
                    "instrument definition ID (")(str_instrument_definition_id)(
                    ") when expecting: ")(strInstrumentDefinitionID)(".")
                    .Flush();
            } else {
                const auto& wallet = api_.Wallet();
                const auto accountID = Identifier::Factory(str_acct_id);
                auto account = wallet.Account(accountID);

                if (false == bool(account)) {
                    LogOutput(OT_METHOD)(__FUNCTION__)(
                        ": Unable to load account ")(str_acct_id)(".")
                        .Flush();

                    continue;
                }

                if (false == visitor.Trigger(account.get(), reason)) {
                    LogOutput(OT_METHOD)(__FUNCTION__)(
                        ": Error: Trigger failed for account ")(str_acct_id)
                        .Flush();
                }
            }
        }

        return true;
    }

    return true;
}
}  // namespace opentxs::contract::implementation
