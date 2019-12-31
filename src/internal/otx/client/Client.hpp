// Copyright (c) 2010-2020 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#pragma once

#include "Internal.hpp"

#include "opentxs/api/Core.hpp"
#include "opentxs/api/Factory.hpp"
#include "opentxs/api/client/OTX.hpp"
#include "opentxs/core/contract/peer/PeerReply.hpp"
#include "opentxs/core/contract/peer/PeerRequest.hpp"
#include "opentxs/consensus/ServerContext.hpp"

#include "internal/api/client/Client.hpp"

#include <future>
#include <memory>

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
    bool operator()(
        const opentxs::OT_DownloadNymboxType&,
        const opentxs::OT_DownloadNymboxType&) const
    {
        return false;
    }
};
template <>
struct less<opentxs::OT_GetTransactionNumbersType> {
    bool operator()(
        const opentxs::OT_GetTransactionNumbersType&,
        const opentxs::OT_GetTransactionNumbersType&) const
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
    std::tuple<OTIdentifier, OTNymID, Amount, std::string, Time, Time>;
/** SendInvoiceTask: sourceAccountID, targetNymID, value, memo, validFrom,
 * validTo.  Note that "sourceAccountID" is where the payment will be SENT
 * since the "source" is the guy who drafted the invoice.
 */
using SendInvoiceTask =
    std::tuple<OTIdentifier, OTNymID, Amount, std::string, Time, Time>;
/** SendVoucherTask: sourceAccountID, targetNymID, value, memo, validFrom,
 * validTo
 */
using SendVoucherTask =
    std::tuple<OTIdentifier, OTNymID, Amount, std::string, Time, Time>;
/** SendTransferTask: source account, destination account, amount, memo
 */
using SendTransferTask =
    std::tuple<OTIdentifier, OTIdentifier, Amount, std::string>;
#if OT_CASH
/** WithdrawCashTask: Account ID, amount*/
using WithdrawCashTask = std::pair<OTIdentifier, Amount>;
#endif  // OT_CASH
/** WithdrawVoucherTask: sourceAccountID, targetNymID, value, memo, validFrom,
 * validTo */
using WithdrawVoucherTask =
    std::tuple<OTIdentifier, OTNymID, Amount, std::string, Time, Time>;
}  // namespace opentxs::otx::client

namespace opentxs
{
template <>
struct make_blank<otx::client::DepositPaymentTask> {
    static otx::client::DepositPaymentTask value(const api::Core& api)
    {
        return {make_blank<OTUnitID>::value(api),
                make_blank<OTIdentifier>::value(api),
                nullptr};
    }
};
#if OT_CASH
template <>
struct make_blank<otx::client::DownloadMintTask> {
    static otx::client::DownloadMintTask value(const api::Core& api)
    {
        return {make_blank<OTUnitID>::value(api), 0};
    }
};
#endif  // OT_CASH
template <>
struct make_blank<otx::client::IssueUnitDefinitionTask> {
    static otx::client::IssueUnitDefinitionTask value(const api::Core& api)
    {
        return {make_blank<OTUnitID>::value(api), "", proto::CITEMTYPE_ERROR};
    }
};
template <>
struct make_blank<otx::client::MessageTask> {
    static otx::client::MessageTask value(const api::Core& api)
    {
        return {make_blank<OTNymID>::value(api), "", nullptr};
    }
};
#if OT_CASH
template <>
struct make_blank<otx::client::PayCashTask> {
    static otx::client::PayCashTask value(const api::Core& api)
    {
        return {make_blank<OTNymID>::value(api),
                make_blank<OTIdentifier>::value(api)};
    }
};
#endif  // OT_CASH
template <>
struct make_blank<otx::client::PaymentTask> {
    static otx::client::PaymentTask value(const api::Core& api)
    {
        return {make_blank<OTNymID>::value(api), nullptr};
    }
};
template <>
struct make_blank<otx::client::PeerReplyTask> {
    static otx::client::PeerReplyTask value(const api::Core& api)
    {
        return {make_blank<OTNymID>::value(api),
                api.Factory().PeerReply(),
                api.Factory().PeerRequest()};
    }
};
template <>
struct make_blank<otx::client::PeerRequestTask> {
    static otx::client::PeerRequestTask value(const api::Core& api)
    {
        return {make_blank<OTNymID>::value(api), api.Factory().PeerRequest()};
    }
};
template <>
struct make_blank<otx::client::PublishServerContractTask> {
    static otx::client::PublishServerContractTask value(const api::Core& api)
    {
        return {make_blank<OTServerID>::value(api), false};
    }
};
template <>
struct make_blank<otx::client::RegisterAccountTask> {
    static otx::client::RegisterAccountTask value(const api::Core& api)
    {
        return {"", make_blank<OTUnitID>::value(api)};
    }
};
template <>
struct make_blank<otx::client::SendChequeTask> {
    static otx::client::SendChequeTask value(const api::Core& api)
    {
        return {make_blank<OTIdentifier>::value(api),
                make_blank<OTNymID>::value(api),
                0,
                "",
                Clock::now(),
                Clock::now()};
    }
};
template <>
struct make_blank<otx::client::SendInvoiceTask> {
    static otx::client::SendInvoiceTask value(const api::Core& api)
    {
        return {make_blank<OTIdentifier>::value(api),
                make_blank<OTNymID>::value(api),
                0,
                "",
                Clock::now(),
                Clock::now()};
    }
};
template <>
struct make_blank<otx::client::SendVoucherTask> {
    static otx::client::SendVoucherTask value(const api::Core& api)
    {
        return {make_blank<OTIdentifier>::value(api),
                make_blank<OTNymID>::value(api),
                0,
                "",
                Clock::now(),
                Clock::now()};
    }
};
template <>
struct make_blank<otx::client::SendTransferTask> {
    static otx::client::SendTransferTask value(const api::Core& api)
    {
        return {make_blank<OTIdentifier>::value(api),
                make_blank<OTIdentifier>::value(api),
                0,
                ""};
    }
};
#if OT_CASH
template <>
struct make_blank<otx::client::WithdrawCashTask> {
    static otx::client::WithdrawCashTask value(const api::Core& api)
    {
        return {make_blank<OTIdentifier>::value(api), 0};
    }
};
#endif  // OT_CASH
template <>
struct make_blank<otx::client::WithdrawVoucherTask> {
    static otx::client::WithdrawVoucherTask value(const api::Core& api)
    {
        return {make_blank<OTIdentifier>::value(api),
                make_blank<OTNymID>::value(api),
                0,
                "",
                Clock::now(),
                Clock::now()};
    }
};
}  // namespace opentxs

namespace opentxs::otx::client::internal
{
struct Operation {
    using Result = ServerContext::DeliveryResult;
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
        PayInvoice,
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
        WithdrawVoucher,
    };

    virtual const identifier::Nym& NymID() const = 0;
    virtual const identifier::Server& ServerID() const = 0;

    virtual bool AddClaim(
        const proto::ContactSectionName section,
        const proto::ContactItemType type,
        const String& value,
        const bool primary) = 0;
    virtual bool ConveyPayment(
        const identifier::Nym& recipient,
        const std::shared_ptr<const OTPayment> payment) = 0;
#if OT_CASH
    virtual bool DepositCash(
        const Identifier& depositAccountID,
        const std::shared_ptr<blind::Purse> purse) = 0;
#endif
    virtual bool DepositCheque(
        const Identifier& depositAccountID,
        const std::shared_ptr<Cheque> cheque) = 0;
    virtual bool PayInvoice(
        const Identifier& depositAccountID,
        const std::shared_ptr<Cheque> invoice) = 0;
    virtual bool DownloadContract(
        const Identifier& ID,
        const ContractType type = ContractType::invalid) = 0;
    virtual Future GetFuture() = 0;
    virtual bool IssueUnitDefinition(
        const std::shared_ptr<const proto::UnitDefinition> unitDefinition,
        const ServerContext::ExtraArgs& args = {}) = 0;
    virtual void join() = 0;
    virtual bool PublishContract(const identifier::Nym& id) = 0;
    virtual bool PublishContract(const identifier::Server& id) = 0;
    virtual bool PublishContract(const identifier::UnitDefinition& id) = 0;
    virtual bool RequestAdmin(const String& password) = 0;
#if OT_CASH
    virtual bool SendCash(
        const identifier::Nym& recipient,
        const Identifier& workflowID) = 0;
#endif
    virtual bool SendMessage(
        const identifier::Nym& recipient,
        const String& message,
        const SetID setID = {}) = 0;
    virtual bool SendPeerReply(
        const identifier::Nym& targetNymID,
        const OTPeerReply peerreply,
        const OTPeerRequest peerrequest) = 0;
    virtual bool SendPeerRequest(
        const identifier::Nym& targetNymID,
        const OTPeerRequest peerrequest) = 0;
    virtual bool SendTransfer(
        const Identifier& sourceAccountID,
        const Identifier& destinationAccountID,
        const Amount amount,
        const String& memo) = 0;
    virtual void SetPush(const bool enabled) = 0;
    virtual void Shutdown() = 0;
    virtual bool Start(
        const Type type,
        const ServerContext::ExtraArgs& args = {}) = 0;
    virtual bool Start(
        const Type type,
        const identifier::UnitDefinition& targetUnitID,
        const ServerContext::ExtraArgs& args = {}) = 0;
    virtual bool Start(
        const Type type,
        const identifier::Nym& targetNymID,
        const ServerContext::ExtraArgs& args = {}) = 0;
    virtual bool UpdateAccount(const Identifier& accountID) = 0;
#if OT_CASH
    virtual bool WithdrawCash(
        const Identifier& accountID,
        const Amount amount) = 0;
#endif
    virtual bool WithdrawVoucher(
        const Identifier& sourceAccountID,
        const Identifier& recipientContactID,
        const Amount amount,
        const std::string& memo,
        const Time validFrom,
        const Time validTo) = 0;

    virtual ~Operation() = default;
};

struct StateMachine {
    using BackgroundTask = api::client::OTX::BackgroundTask;
    using Result = api::client::OTX::Result;
    using TaskID = api::client::OTX::TaskID;
    using Finish = std::function<bool(const TaskID, const bool, Result&&)>;

    virtual const api::internal::Core& api() const = 0;
    virtual BackgroundTask DepositPayment(
        const otx::client::DepositPaymentTask& params) const = 0;
    virtual BackgroundTask DownloadUnitDefinition(
        const otx::client::DownloadUnitDefinitionTask& params) const = 0;
    virtual Result error_result() const = 0;
    virtual bool finish_task(
        const TaskID taskID,
        const bool success,
        Result&& result) const = 0;
    virtual TaskID next_task_id() const = 0;
    virtual BackgroundTask RegisterAccount(
        const otx::client::RegisterAccountTask& params) const = 0;
    virtual BackgroundTask start_task(
        const TaskID taskID,
        bool success,
        Finish finish = {}) const = 0;

    virtual ~StateMachine() = default;
};
}  // namespace opentxs::otx::client::internal
