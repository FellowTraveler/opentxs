// Copyright (c) 2010-2020 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef OPENTXS_API_CLIENT_SERVERACTION_HPP
#define OPENTXS_API_CLIENT_SERVERACTION_HPP

// IWYU pragma: no_include "opentxs/Proto.hpp"

#include "opentxs/Forward.hpp"  // IWYU pragma: associated

#include <chrono>
#include <string>

#include "opentxs/Pimpl.hpp"
#include "opentxs/Proto.hpp"
#include "opentxs/Types.hpp"

namespace opentxs
{
namespace proto
{
class UnitDefinition;
}  // namespace proto
}  // namespace opentxs

namespace opentxs
{
namespace api
{
namespace client
{
class [[deprecated]] ServerAction
{
public:
    using Action = Pimpl<opentxs::client::ServerAction>;

    OPENTXS_EXPORT virtual Action ActivateSmartContract(
        const PasswordPrompt& reason,
        const identifier::Nym& localNymID,
        const identifier::Server& serverID,
        const Identifier& accountID,
        const std::string& agentName,
        std::unique_ptr<OTSmartContract>& contract) const = 0;
    OPENTXS_EXPORT virtual Action AdjustUsageCredits(
        const PasswordPrompt& reason,
        const identifier::Nym& localNymID,
        const identifier::Server& serverID,
        const identifier::Nym& targetNymID,
        const Amount& adjustment) const = 0;
    OPENTXS_EXPORT virtual Action CancelPaymentPlan(
        const PasswordPrompt& reason,
        const identifier::Nym& localNymID,
        const identifier::Server& serverID,
        std::unique_ptr<OTPaymentPlan>& plan) const = 0;
    OPENTXS_EXPORT virtual Action CreateMarketOffer(
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
        const Amount& activationPrice) const = 0;
    OPENTXS_EXPORT virtual Action DepositPaymentPlan(
        const PasswordPrompt& reason,
        const identifier::Nym& localNymID,
        const identifier::Server& serverID,
        std::unique_ptr<OTPaymentPlan>& plan) const = 0;
    OPENTXS_EXPORT virtual Action DownloadMarketList(
        const PasswordPrompt& reason,
        const identifier::Nym& localNymID,
        const identifier::Server& serverID) const = 0;
    OPENTXS_EXPORT virtual Action DownloadMarketOffers(
        const PasswordPrompt& reason,
        const identifier::Nym& localNymID,
        const identifier::Server& serverID,
        const Identifier& marketID,
        const Amount& depth) const = 0;
    OPENTXS_EXPORT virtual Action DownloadMarketRecentTrades(
        const PasswordPrompt& reason,
        const identifier::Nym& localNymID,
        const identifier::Server& serverID,
        const Identifier& marketID) const = 0;
    OPENTXS_EXPORT virtual Action DownloadNymMarketOffers(
        const PasswordPrompt& reason,
        const identifier::Nym& localNymID,
        const identifier::Server& serverID) const = 0;
    OPENTXS_EXPORT virtual Action ExchangeBasketCurrency(
        const PasswordPrompt& reason,
        const identifier::Nym& localNymID,
        const identifier::Server& serverID,
        const identifier::UnitDefinition& instrumentDefinitionID,
        const Identifier& accountID,
        const Identifier& basketID,
        const bool direction) const = 0;
    OPENTXS_EXPORT virtual Action IssueBasketCurrency(
        const PasswordPrompt& reason,
        const identifier::Nym& localNymID,
        const identifier::Server& serverID,
        const proto::UnitDefinition& basket,
        const std::string& label = "") const = 0;
    OPENTXS_EXPORT virtual Action KillMarketOffer(
        const PasswordPrompt& reason,
        const identifier::Nym& localNymID,
        const identifier::Server& serverID,
        const Identifier& accountID,
        const TransactionNumber number) const = 0;
    OPENTXS_EXPORT virtual Action KillPaymentPlan(
        const PasswordPrompt& reason,
        const identifier::Nym& localNymID,
        const identifier::Server& serverID,
        const Identifier& accountID,
        const TransactionNumber number) const = 0;
    OPENTXS_EXPORT virtual Action PayDividend(
        const PasswordPrompt& reason,
        const identifier::Nym& localNymID,
        const identifier::Server& serverID,
        const identifier::UnitDefinition& instrumentDefinitionID,
        const Identifier& accountID,
        const std::string& memo,
        const Amount& amountPerShare) const = 0;
    OPENTXS_EXPORT virtual Action TriggerClause(
        const PasswordPrompt& reason,
        const identifier::Nym& localNymID,
        const identifier::Server& serverID,
        const TransactionNumber transactionNumber,
        const std::string& clause,
        const std::string& parameter) const = 0;
    OPENTXS_EXPORT virtual Action UnregisterAccount(
        const PasswordPrompt& reason,
        const identifier::Nym& localNymID,
        const identifier::Server& serverID,
        const Identifier& accountID) const = 0;
    OPENTXS_EXPORT virtual Action UnregisterNym(
        const PasswordPrompt& reason,
        const identifier::Nym& localNymID,
        const identifier::Server& serverID) const = 0;
    OPENTXS_EXPORT virtual Action WithdrawVoucher(
        const PasswordPrompt& reason,
        const identifier::Nym& localNymID,
        const identifier::Server& serverID,
        const Identifier& accountID,
        const identifier::Nym& recipientNymID,
        const Amount& amount,
        const std::string& memo) const = 0;

    OPENTXS_EXPORT virtual ~ServerAction() = default;

protected:
    ServerAction() = default;

private:
    ServerAction(const ServerAction&) = delete;
    ServerAction(ServerAction&&) = delete;
    ServerAction& operator=(const ServerAction&) = delete;
    ServerAction& operator=(ServerAction&&) = delete;
};
}  // namespace client
}  // namespace api
}  // namespace opentxs
#endif
