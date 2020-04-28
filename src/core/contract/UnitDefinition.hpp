// Copyright (c) 2010-2020 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#pragma once

#include <cstdint>
#include <map>
#include <string>

#include "core/contract/Signable.hpp"
#include "opentxs/Proto.hpp"
#include "opentxs/Types.hpp"
#include "opentxs/core/Account.hpp"
#include "opentxs/core/Data.hpp"
#include "opentxs/core/Identifier.hpp"
#include "opentxs/core/contract/UnitDefinition.hpp"
#include "opentxs/protobuf/ContactEnums.pb.h"
#include "opentxs/protobuf/ContractEnums.pb.h"

namespace opentxs
{
namespace api
{
namespace internal
{
struct Core;
}  // namespace internal

class Core;
}  // namespace api

namespace proto
{
class Signature;
}  // namespace proto

class AccountVisitor;
class Factory;
class PasswordPrompt;
class String;
}  // namespace opentxs

namespace opentxs::contract::implementation
{
class Unit : virtual public contract::Unit,
             public opentxs::contract::implementation::Signable
{
public:
    static const std::map<VersionNumber, VersionNumber>
        unit_of_account_version_map_;

    static auto GetID(
        const api::internal::Core& api,
        const SerializedType& contract) -> OTIdentifier;

    auto AddAccountRecord(
        const std::string& dataFolder,
        const Account& theAccount) const -> bool override;
    auto Contract() const -> SerializedType override;
    auto DecimalPower() const -> std::int32_t override { return 0; }
    auto DisplayStatistics(String& strContents) const -> bool override;
    auto EraseAccountRecord(
        const std::string& dataFolder,
        const Identifier& theAcctID) const -> bool override;
    auto FormatAmountLocale(
        const Amount& amount,
        std::string& str_output,
        const std::string& str_thousand,
        const std::string& str_decimal) const -> bool override;
    auto FormatAmountWithoutSymbolLocale(
        const Amount& amount,
        std::string& str_output,
        const std::string& str_thousand,
        const std::string& str_decimal) const -> bool override;
    auto FractionalUnitName() const -> std::string override { return ""; }
    auto GetCurrencyName() const -> const std::string& override
    {
        return primary_unit_name_;
    }
    auto GetCurrencySymbol() const -> const std::string& override
    {
        return primary_unit_symbol_;
    }
    auto Name() const -> std::string override { return short_name_; }
    auto PublicContract() const -> SerializedType override;
    auto Serialize() const -> OTData override;
    auto StringToAmountLocale(
        OTAmount& amount,
        const std::string& str_input,
        const std::string& str_thousand,
        const std::string& str_decimal) const -> bool override;
    auto TLA() const -> std::string override { return short_name_; }
    auto Type() const -> proto::UnitType override = 0;
    auto UnitOfAccount() const -> proto::ContactItemType override
    {
        return unit_of_account_;
    }
    auto VisitAccountRecords(
        const std::string& dataFolder,
        AccountVisitor& visitor,
        const PasswordPrompt& reason) const -> bool override;

    void InitAlias(const std::string& alias) final
    {
        contract::implementation::Signable::SetAlias(alias);
    }
    void SetAlias(const std::string& alias) override;

    ~Unit() override = default;

protected:
    const std::string primary_unit_symbol_;
    const proto::ContactItemType unit_of_account_;

    virtual auto IDVersion(const Lock& lock) const -> SerializedType;
    virtual auto SigVersion(const Lock& lock) const -> SerializedType;
    auto validate(const Lock& lock) const -> bool override;

    auto update_signature(const Lock& lock, const PasswordPrompt& reason)
        -> bool override;

    Unit(
        const api::internal::Core& api,
        const Nym_p& nym,
        const std::string& shortname,
        const std::string& name,
        const std::string& symbol,
        const std::string& terms,
        const proto::ContactItemType unitOfAccount,
        const VersionNumber version);
    Unit(
        const api::internal::Core& api,
        const Nym_p& nym,
        const SerializedType serialized);
    Unit(const Unit&);

private:
    friend opentxs::Factory;

    const std::string primary_unit_name_;
    const std::string short_name_;

    auto contract(const Lock& lock) const -> SerializedType;
    auto GetID(const Lock& lock) const -> OTIdentifier override;
    auto verify_signature(const Lock& lock, const proto::Signature& signature)
        const -> bool override;

    Unit(Unit&&) = delete;
    auto operator=(const Unit&) -> Unit& = delete;
    auto operator=(Unit&&) -> Unit& = delete;
};
}  // namespace opentxs::contract::implementation
