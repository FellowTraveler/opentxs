// Copyright (c) 2010-2020 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef OPENTXS_CORE_CONTRACT_UNITDEFINITION_HPP
#define OPENTXS_CORE_CONTRACT_UNITDEFINITION_HPP

// IWYU pragma: no_include "opentxs/Proto.hpp"

#include "opentxs/Forward.hpp"  // IWYU pragma: associated

#include "opentxs/core/Amount.hpp"

#include <cstdint>
#include <string>

#include "opentxs/Proto.hpp"
#include "opentxs/SharedPimpl.hpp"
#include "opentxs/core/contract/Signable.hpp"
#include "opentxs/protobuf/ContractEnums.pb.h"

namespace opentxs
{
namespace contract
{
class Unit;
}  // namespace contract

namespace proto
{
class UnitDefinition;
}  // namespace proto

using OTUnitDefinition = SharedPimpl<contract::Unit>;
}  // namespace opentxs

namespace opentxs
{
namespace contract
{
class Unit : virtual public opentxs::contract::Signable
{
public:
    using SerializedType = proto::UnitDefinition;

    OPENTXS_EXPORT static const VersionNumber DefaultVersion;
    OPENTXS_EXPORT static const VersionNumber MaxVersion;

    OPENTXS_EXPORT static std::string formatLongAmount(
        const Amount& lValue,
        const std::int32_t nFactor = 100,
        const std::int32_t nPower = 2,
        const char* szCurrencySymbol = "",
        const char* szThousandSeparator = ",",
        const char* szDecimalPoint = ".");
    OPENTXS_EXPORT static bool ParseFormatted(
        OTAmount& lResult,
        const std::string& str_input,
        const std::int32_t nFactor = 100,
        const std::int32_t nPower = 2,
        const char* szThousandSeparator = ",",
        const char* szDecimalPoint = ".");
    OPENTXS_EXPORT static std::set<proto::ContactItemType> ValidUnits(
        const VersionNumber version = DefaultVersion) noexcept;

    OPENTXS_EXPORT virtual bool AddAccountRecord(
        const std::string& dataFolder,
        const Account& theAccount) const = 0;
    OPENTXS_EXPORT virtual SerializedType Contract() const = 0;
    OPENTXS_EXPORT virtual std::int32_t DecimalPower() const = 0;
    OPENTXS_EXPORT virtual bool DisplayStatistics(
        String& strContents) const = 0;
    OPENTXS_EXPORT virtual bool EraseAccountRecord(
        const std::string& dataFolder,
        const Identifier& theAcctID) const = 0;
    OPENTXS_EXPORT virtual bool FormatAmountLocale(
        const Amount& amount,
        std::string& str_output,
        const std::string& str_thousand,
        const std::string& str_decimal) const = 0;
    OPENTXS_EXPORT virtual bool FormatAmountWithoutSymbolLocale(
        const Amount& amount,
        std::string& str_output,
        const std::string& str_thousand,
        const std::string& str_decimal) const = 0;
    OPENTXS_EXPORT virtual std::string FractionalUnitName() const = 0;
    OPENTXS_EXPORT virtual const std::string& GetCurrencyName() const = 0;
    OPENTXS_EXPORT virtual const std::string& GetCurrencySymbol() const = 0;
    OPENTXS_EXPORT virtual SerializedType PublicContract() const = 0;
    OPENTXS_EXPORT virtual bool StringToAmountLocale(
        OTAmount& amount,
        const std::string& str_input,
        const std::string& str_thousand,
        const std::string& str_decimal) const = 0;
    OPENTXS_EXPORT virtual std::string TLA() const = 0;
    OPENTXS_EXPORT virtual proto::UnitType Type() const = 0;
    OPENTXS_EXPORT virtual proto::ContactItemType UnitOfAccount() const = 0;
    OPENTXS_EXPORT virtual bool VisitAccountRecords(
        const std::string& dataFolder,
        AccountVisitor& visitor,
        const PasswordPrompt& reason) const = 0;

    OPENTXS_EXPORT virtual void InitAlias(const std::string& alias) = 0;

    OPENTXS_EXPORT ~Unit() override = default;

protected:
    Unit() noexcept = default;

private:
    friend OTUnitDefinition;

#ifndef _WIN32
    OPENTXS_EXPORT Unit* clone() const noexcept override = 0;
#endif

    Unit(const Unit&) = delete;
    Unit(Unit&&) = delete;
    Unit& operator=(const Unit&) = delete;
    Unit& operator=(Unit&&) = delete;
};
}  // namespace contract
}  // namespace opentxs
#endif
