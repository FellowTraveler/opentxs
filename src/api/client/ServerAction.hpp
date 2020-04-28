// Copyright (c) 2010-2020 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

// IWYU pragma: private
// IWYU pragma: friend ".*src/api/client/ServerAction.cpp"

#pragma once

#include <chrono>
#include <memory>
#include <string>

#include "opentxs/Types.hpp"
#include "opentxs/api/client/ServerAction.hpp"

namespace opentxs
{
namespace api
{
namespace client
{
namespace internal
{
struct Manager;
}  // namespace internal
}  // namespace client
}  // namespace api

namespace identifier
{
class Nym;
class Server;
class UnitDefinition;
}  // namespace identifier

namespace proto
{
class UnitDefinition;
}  // namespace proto

class Identifier;
class OTPaymentPlan;
class OTSmartContract;
class PasswordPrompt;
}  // namespace opentxs

namespace opentxs::api::client::implementation
{
class ServerAction final : virtual public opentxs::api::client::ServerAction
{
public:
    auto ActivateSmartContract(
        const PasswordPrompt& reason,
        const identifier::Nym& localNymID,
        const identifier::Server& serverID,
        const Identifier& accountID,
        const std::string& agentName,
        std::unique_ptr<OTSmartContract>& contract) const -> Action final;
    auto AdjustUsageCredits(
        const PasswordPrompt& reason,
        const identifier::Nym& localNymID,
        const identifier::Server& serverID,
        const identifier::Nym& targetNymID,
        const Amount& adjustment) const -> Action final;
    auto CancelPaymentPlan(
        const PasswordPrompt& reason,
        const identifier::Nym& localNymID,
        const identifier::Server& serverID,
        std::unique_ptr<OTPaymentPlan>& plan) const -> Action final;
    auto CreateMarketOffer(
        const PasswordPrompt& reason,
        const Identifier& assetAccountID,
        const Identifier& currencyAccountID,
        const Amount& scale,
        const Amount& increment,
        const Amount& quantity,
        const Amount& price,
        const bool selling,
        const std::chrono::seconds lifetime,
        const std::string& stopSign,
        const Amount& activationPrice) const -> Action final;
    auto DepositPaymentPlan(
        const PasswordPrompt& reason,
        const identifier::Nym& localNymID,
        const identifier::Server& serverID,
        std::unique_ptr<OTPaymentPlan>& plan) const -> Action final;
    auto DownloadMarketList(
        const PasswordPrompt& reason,
        const identifier::Nym& localNymID,
        const identifier::Server& serverID) const -> Action final;
    auto DownloadMarketOffers(
        const PasswordPrompt& reason,
        const identifier::Nym& localNymID,
        const identifier::Server& serverID,
        const Identifier& marketID,
        const Amount& depth) const -> Action final;
    auto DownloadMarketRecentTrades(
        const PasswordPrompt& reason,
        const identifier::Nym& localNymID,
        const identifier::Server& serverID,
        const Identifier& marketID) const -> Action final;
    auto DownloadNymMarketOffers(
        const PasswordPrompt& reason,
        const identifier::Nym& localNymID,
        const identifier::Server& serverID) const -> Action final;
    auto ExchangeBasketCurrency(
        const PasswordPrompt& reason,
        const identifier::Nym& localNymID,
        const identifier::Server& serverID,
        const identifier::UnitDefinition& instrumentDefinitionID,
        const Identifier& accountID,
        const Identifier& basketID,
        const bool direction) const -> Action final;
    auto IssueBasketCurrency(
        const PasswordPrompt& reason,
        const identifier::Nym& localNymID,
        const identifier::Server& serverID,
        const proto::UnitDefinition& basket,
        const std::string& label) const -> Action final;
    auto KillMarketOffer(
        const PasswordPrompt& reason,
        const identifier::Nym& localNymID,
        const identifier::Server& serverID,
        const Identifier& accountID,
        const TransactionNumber number) const -> Action final;
    auto KillPaymentPlan(
        const PasswordPrompt& reason,
        const identifier::Nym& localNymID,
        const identifier::Server& serverID,
        const Identifier& accountID,
        const TransactionNumber number) const -> Action final;
    auto PayDividend(
        const PasswordPrompt& reason,
        const identifier::Nym& localNymID,
        const identifier::Server& serverID,
        const identifier::UnitDefinition& instrumentDefinitionID,
        const Identifier& accountID,
        const std::string& memo,
        const Amount& amountPerShare) const -> Action final;
    auto TriggerClause(
        const PasswordPrompt& reason,
        const identifier::Nym& localNymID,
        const identifier::Server& serverID,
        const TransactionNumber transactionNumber,
        const std::string& clause,
        const std::string& parameter) const -> Action final;
    auto UnregisterAccount(
        const PasswordPrompt& reason,
        const identifier::Nym& localNymID,
        const identifier::Server& serverID,
        const Identifier& accountID) const -> Action final;
    auto UnregisterNym(
        const PasswordPrompt& reason,
        const identifier::Nym& localNymID,
        const identifier::Server& serverID) const -> Action final;
    auto WithdrawVoucher(
        const PasswordPrompt& reason,
        const identifier::Nym& localNymID,
        const identifier::Server& serverID,
        const Identifier& accountID,
        const identifier::Nym& recipientNymID,
        const Amount& amount,
        const std::string& memo) const -> Action final;

    ServerAction(
        const api::client::internal::Manager& api,
        const ContextLockCallback& lockCallback);

    ~ServerAction() final = default;

private:
    const api::client::internal::Manager& api_;
    ContextLockCallback lock_callback_;

    ServerAction() = delete;
    ServerAction(const ServerAction&) = delete;
    ServerAction(ServerAction&&) = delete;
    auto operator=(const ServerAction&) -> ServerAction& = delete;
    auto operator=(ServerAction&&) -> ServerAction& = delete;
};
}  // namespace opentxs::api::client::implementation
