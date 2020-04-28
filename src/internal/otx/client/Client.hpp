// Copyright (c) 2010-2020 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#pragma once

#include <future>
#include <memory>

#include "internal/api/client/Client.hpp"
#include "opentxs/api/Core.hpp"
#include "opentxs/api/Factory.hpp"
#include "opentxs/api/client/OTX.hpp"
#include "opentxs/core/contract/peer/PeerReply.hpp"
#include "opentxs/core/contract/peer/PeerRequest.hpp"
#include "opentxs/otx/consensus/Server.hpp"
#include "util/Blank.hpp"
#include "opentxs/core/Amount.hpp"

namespace opentxs
{

struct OT_DownloadNymboxType {
};
struct OT_GetTransactionNumbersType {
};
}  // namespace opentxs

namespace std
{
template <>
struct less<opentxs::OT_DownloadNymboxType> {
    auto operator()(
        const opentxs::OT_DownloadNymboxType&,
        const opentxs::OT_DownloadNymboxType&) const -> bool
    {
        return false;
    }
};
template <>
struct less<opentxs::OT_GetTransactionNumbersType> {
    auto operator()(
        const opentxs::OT_GetTransactionNumbersType&,
        const opentxs::OT_GetTransactionNumbersType&) const -> bool
    {
        return false;
    }
};
}  // namespace std

namespace opentxs::otx::client
{
using CheckNymTask = OTNymID;
/** DepositPaymentTask: unit id, accountID, payment */
using DepositPaymentTask =
    std::tuple<OTUnitID, OTIdentifier, std::shared_ptr<const OTPayment>>;
using DownloadContractTask = OTServerID;
#if OT_CASH
using DownloadMintTask = std::pair<OTUnitID, int>;
#endif  // OT_CASH
using DownloadNymboxTask = OT_DownloadNymboxType;
using DownloadUnitDefinitionTask = OTUnitID;
using GetTransactionNumbersTask = OT_GetTransactionNumbersType;
/** IssueUnitDefinitionTask: unit definition id, account label, claim */
using IssueUnitDefinitionTask =
    std::tuple<OTUnitID, std::string, proto::ContactItemType>;
/** MessageTask: recipientID, message */
using MessageTask = std::tuple<OTNymID, std::string, std::shared_ptr<SetID>>;
#if OT_CASH
/** PayCashTask: recipientID, workflow ID */
using PayCashTask = std::pair<OTNymID, OTIdentifier>;
#endif  // OT_CASH
/** PaymentTask: recipientID, payment */
using PaymentTask = std::pair<OTNymID, std::shared_ptr<const OTPayment>>;
/** PeerReplyTask: targetNymID, peer reply, peer request */
using PeerReplyTask = std::tuple<OTNymID, OTPeerReply, OTPeerRequest>;
/** PeerRequestTask: targetNymID, peer request */
using PeerRequestTask = std::pair<OTNymID, OTPeerRequest>;
using ProcessInboxTask = OTIdentifier;
using PublishServerContractTask = std::pair<OTServerID, bool>;
/** RegisterAccountTask: account label, unit definition id */
using RegisterAccountTask = std::pair<std::string, OTUnitID>;
using RegisterNymTask = bool;
/** SendChequeTask: sourceAccountID, targetNymID, value, memo, validFrom,
 * validTo
 */
using SendChequeTask =
    std::tuple<OTIdentifier, OTNymID, OTAmount, std::string, Time, Time>;
/** SendTransferTask: source account, destination nym, amount, memo
 */
using SendTransferTask =
    std::tuple<OTIdentifier, OTIdentifier, OTAmount, std::string>;
#if OT_CASH
/** WithdrawCashTask: Account ID, amount*/
using WithdrawCashTask = std::pair<OTIdentifier, OTAmount>;
#endif  // OT_CASH
}  // namespace opentxs::otx::client

namespace opentxs
{
template <>
struct make_blank<OTAmount> {
    static OTAmount value(const api::Core& api)
    {
        return Amount::Factory();
//      return {api.Factory().Amount()};
    }
};

template <>
struct make_blank<otx::client::DepositPaymentTask> {
    static auto value(const api::Core& api) -> otx::client::DepositPaymentTask
    {
        return {
            make_blank<OTUnitID>::value(api),
            make_blank<OTIdentifier>::value(api),
            nullptr};
    }
};
#if OT_CASH
template <>
struct make_blank<otx::client::DownloadMintTask> {
    static auto value(const api::Core& api) -> otx::client::DownloadMintTask
    {
        return {make_blank<OTUnitID>::value(api), 0};
    }
};
#endif  // OT_CASH
template <>
struct make_blank<otx::client::IssueUnitDefinitionTask> {
    static auto value(const api::Core& api)
        -> otx::client::IssueUnitDefinitionTask
    {
        return {make_blank<OTUnitID>::value(api), "", proto::CITEMTYPE_ERROR};
    }
};
template <>
struct make_blank<otx::client::MessageTask> {
    static auto value(const api::Core& api) -> otx::client::MessageTask
    {
        return {make_blank<OTNymID>::value(api), "", nullptr};
    }
};
#if OT_CASH
template <>
struct make_blank<otx::client::PayCashTask> {
    static auto value(const api::Core& api) -> otx::client::PayCashTask
    {
        return {
            make_blank<OTNymID>::value(api),
            make_blank<OTIdentifier>::value(api)};
    }
};
#endif  // OT_CASH
template <>
struct make_blank<otx::client::PaymentTask> {
    static auto value(const api::Core& api) -> otx::client::PaymentTask
    {
        return {make_blank<OTNymID>::value(api), nullptr};
    }
};
template <>
struct make_blank<otx::client::PeerReplyTask> {
    static auto value(const api::Core& api) -> otx::client::PeerReplyTask
    {
        return {
            make_blank<OTNymID>::value(api),
            api.Factory().PeerReply(),
            api.Factory().PeerRequest()};
    }
};
template <>
struct make_blank<otx::client::PeerRequestTask> {
    static auto value(const api::Core& api) -> otx::client::PeerRequestTask
    {
        return {make_blank<OTNymID>::value(api), api.Factory().PeerRequest()};
    }
};
template <>
struct make_blank<otx::client::PublishServerContractTask> {
    static auto value(const api::Core& api)
        -> otx::client::PublishServerContractTask
    {
        return {make_blank<OTServerID>::value(api), false};
    }
};
template <>
struct make_blank<otx::client::RegisterAccountTask> {
    static auto value(const api::Core& api) -> otx::client::RegisterAccountTask
    {
        return {"", make_blank<OTUnitID>::value(api)};
    }
};
template <>
struct make_blank<otx::client::SendChequeTask> {
    static auto value(const api::Core& api) -> otx::client::SendChequeTask
    {
        return {
            make_blank<OTIdentifier>::value(api),
            make_blank<OTNymID>::value(api),
            0,
            "",
            Clock::now(),
            Clock::now()};
    }
};
template <>
struct make_blank<otx::client::SendTransferTask> {
    static auto value(const api::Core& api) -> otx::client::SendTransferTask
    {
        return {
            make_blank<OTIdentifier>::value(api),
            make_blank<OTIdentifier>::value(api),
            0,
            ""};
    }
};
#if OT_CASH
template <>
struct make_blank<otx::client::WithdrawCashTask> {
    static auto value(const api::Core& api) -> otx::client::WithdrawCashTask
    {
        return {make_blank<OTIdentifier>::value(api),
                make_blank<OTAmount>::value(api)};
    }
};
#endif  // OT_CASH
}  // namespace opentxs

namespace opentxs::otx::client::internal
{
struct Operation {
    using Result = otx::context::Server::DeliveryResult;
    using Future = std::future<Result>;

    enum class Type {
        Invalid = 0,
        AddClaim,
        CheckNym,
        ConveyPayment,
        DepositCash,
        DepositCheque,
        DownloadContract,
        DownloadMint,
        GetTransactionNumbers,
        IssueUnitDefinition,
        PublishNym,
        PublishServer,
        PublishUnit,
        RefreshAccount,
        RegisterAccount,
        RegisterNym,
        RequestAdmin,
        SendCash,
        SendMessage,
        SendPeerReply,
        SendPeerRequest,
        SendTransfer,
        WithdrawCash,
    };

    virtual auto NymID() const -> const identifier::Nym& = 0;
    virtual auto ServerID() const -> const identifier::Server& = 0;

    virtual auto AddClaim(
        const proto::ContactSectionName section,
        const proto::ContactItemType type,
        const String& value,
        const bool primary) -> bool = 0;
    virtual auto ConveyPayment(
        const identifier::Nym& recipient,
        const std::shared_ptr<const OTPayment> payment) -> bool = 0;
#if OT_CASH
    virtual auto DepositCash(
        const Identifier& depositAccountID,
        const std::shared_ptr<blind::Purse> purse) -> bool = 0;
#endif
    virtual auto DepositCheque(
        const Identifier& depositAccountID,
        const std::shared_ptr<Cheque> cheque) -> bool = 0;
    virtual auto DownloadContract(
        const Identifier& ID,
        const ContractType type = ContractType::invalid) -> bool = 0;
    virtual auto GetFuture() -> Future = 0;
    virtual auto IssueUnitDefinition(
        const std::shared_ptr<const proto::UnitDefinition> unitDefinition,
        const otx::context::Server::ExtraArgs& args = {}) -> bool = 0;
    virtual void join() = 0;
    virtual auto PublishContract(const identifier::Nym& id) -> bool = 0;
    virtual auto PublishContract(const identifier::Server& id) -> bool = 0;
    virtual auto PublishContract(const identifier::UnitDefinition& id)
        -> bool = 0;
    virtual auto RequestAdmin(const String& password) -> bool = 0;
#if OT_CASH
    virtual auto SendCash(
        const identifier::Nym& recipient,
        const Identifier& workflowID) -> bool = 0;
#endif
    virtual auto SendMessage(
        const identifier::Nym& recipient,
        const String& message,
        const SetID setID = {}) -> bool = 0;
    virtual auto SendPeerReply(
        const identifier::Nym& targetNymID,
        const OTPeerReply peerreply,
        const OTPeerRequest peerrequest) -> bool = 0;
    virtual auto SendPeerRequest(
        const identifier::Nym& targetNymID,
        const OTPeerRequest peerrequest) -> bool = 0;
    virtual auto SendTransfer(
        const Identifier& sourceAccountID,
        const Identifier& destinationAccountID,
        const Amount& amount,
        const String& memo) -> bool = 0;
    virtual void SetPush(const bool enabled) = 0;
    virtual void Shutdown() = 0;
    virtual auto Start(
        const Type type,
        const otx::context::Server::ExtraArgs& args = {}) -> bool = 0;
    virtual auto Start(
        const Type type,
        const identifier::UnitDefinition& targetUnitID,
        const otx::context::Server::ExtraArgs& args = {}) -> bool = 0;
    virtual auto Start(
        const Type type,
        const identifier::Nym& targetNymID,
        const otx::context::Server::ExtraArgs& args = {}) -> bool = 0;
    virtual auto UpdateAccount(const Identifier& accountID) -> bool = 0;
#if OT_CASH
    virtual auto WithdrawCash(const Identifier& accountID, const Amount& amount)
        -> bool = 0;
#endif

    virtual ~Operation() = default;
};

struct StateMachine {
    using BackgroundTask = api::client::OTX::BackgroundTask;
    using Result = api::client::OTX::Result;
    using TaskID = api::client::OTX::TaskID;

    virtual auto api() const -> const api::internal::Core& = 0;
    virtual auto DepositPayment(const otx::client::DepositPaymentTask& params)
        const -> BackgroundTask = 0;
    virtual auto DownloadUnitDefinition(
        const otx::client::DownloadUnitDefinitionTask& params) const
        -> BackgroundTask = 0;
    virtual auto error_result() const -> Result = 0;
    virtual auto finish_task(
        const TaskID taskID,
        const bool success,
        Result&& result) const -> bool = 0;
    virtual auto next_task_id() const -> TaskID = 0;
    virtual auto RegisterAccount(const otx::client::RegisterAccountTask& params)
        const -> BackgroundTask = 0;
    virtual auto start_task(const TaskID taskID, bool success) const
        -> BackgroundTask = 0;

    virtual ~StateMachine() = default;
};
}  // namespace opentxs::otx::client::internal
