// Copyright (c) 2010-2020 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include "0_stdafx.hpp"             // IWYU pragma: associated
#include "1_Internal.hpp"           // IWYU pragma: associated
#include "api/client/Workflow.hpp"  // IWYU pragma: associated

#include <algorithm>
#include <chrono>
#include <iterator>
#include <memory>
#include <type_traits>

#include "internal/api/Api.hpp"
#include "internal/api/client/Factory.hpp"
#include "opentxs/Forward.hpp"
#include "opentxs/Pimpl.hpp"
#include "opentxs/Proto.tpp"
#include "opentxs/api/Endpoints.hpp"
#include "opentxs/api/Factory.hpp"
#include "opentxs/api/client/Activity.hpp"
#include "opentxs/api/client/Contacts.hpp"
#include "opentxs/api/client/Workflow.hpp"
#include "opentxs/api/storage/Storage.hpp"
#if OT_CASH
#include "opentxs/blind/Purse.hpp"
#endif  // OT_CASH
#include "opentxs/core/Cheque.hpp"
#include "opentxs/core/Data.hpp"
#include "opentxs/core/Identifier.hpp"
#include "opentxs/core/Log.hpp"
#include "opentxs/core/LogSource.hpp"
#include "opentxs/core/Message.hpp"
#include "opentxs/core/OTTransaction.hpp"
#include "opentxs/core/String.hpp"
#include "opentxs/core/identifier/Nym.hpp"
#include "opentxs/core/identifier/Server.hpp"
#include "opentxs/core/identifier/UnitDefinition.hpp"
#include "opentxs/network/zeromq/Context.hpp"
#include "opentxs/network/zeromq/Message.hpp"
#include "opentxs/network/zeromq/socket/Publish.hpp"
#include "opentxs/network/zeromq/socket/Push.hpp"
#include "opentxs/network/zeromq/socket/Socket.hpp"
#include "opentxs/protobuf/AccountEvent.pb.h"
#include "opentxs/protobuf/Check.hpp"
#include "opentxs/protobuf/InstrumentRevision.pb.h"
#include "opentxs/protobuf/PaymentEvent.pb.h"
#include "opentxs/protobuf/PaymentWorkflow.pb.h"
#include "opentxs/protobuf/PaymentWorkflowEnums.pb.h"
#include "opentxs/protobuf/Purse.pb.h"
#include "opentxs/protobuf/RPCEnums.pb.h"
#include "opentxs/protobuf/RPCPush.pb.h"
#include "opentxs/protobuf/verify/PaymentWorkflow.hpp"
#include "opentxs/protobuf/verify/RPCPush.hpp"
#include "opentxs/util/WorkType.hpp"

#define RPC_ACCOUNT_EVENT_VERSION 2
#define RPC_PUSH_VERSION 1

#define OT_METHOD "opentxs::api::client::implementation::Workflow::"

namespace zmq = opentxs::network::zeromq;

namespace opentxs::factory
{
auto Workflow(
    const api::internal::Core& api,
    const api::client::Activity& activity,
    const api::client::Contacts& contact) -> api::client::Workflow*
{
    return new api::client::implementation::Workflow(api, activity, contact);
}
}  // namespace opentxs::factory

namespace opentxs::api::client
{
#if OT_CASH
auto Workflow::ContainsCash(const proto::PaymentWorkflow& workflow) -> bool
{
    switch (workflow.type()) {
        case proto::PAYMENTWORKFLOWTYPE_OUTGOINGCASH:
        case proto::PAYMENTWORKFLOWTYPE_INCOMINGCASH: {
            return true;
        }
        case proto::PAYMENTWORKFLOWTYPE_ERROR:
        case proto::PAYMENTWORKFLOWTYPE_OUTGOINGCHEQUE:
        case proto::PAYMENTWORKFLOWTYPE_INCOMINGCHEQUE:
        case proto::PAYMENTWORKFLOWTYPE_OUTGOINGINVOICE:
        case proto::PAYMENTWORKFLOWTYPE_INCOMINGINVOICE:
        case proto::PAYMENTWORKFLOWTYPE_OUTGOINGTRANSFER:
        case proto::PAYMENTWORKFLOWTYPE_INCOMINGTRANSFER:
        case proto::PAYMENTWORKFLOWTYPE_INTERNALTRANSFER:
        default: {
        }
    }

    return false;
}
#endif

auto Workflow::ContainsCheque(const proto::PaymentWorkflow& workflow) -> bool
{
    switch (workflow.type()) {
        case proto::PAYMENTWORKFLOWTYPE_OUTGOINGCHEQUE:
        case proto::PAYMENTWORKFLOWTYPE_INCOMINGCHEQUE:
        case proto::PAYMENTWORKFLOWTYPE_OUTGOINGINVOICE:
        case proto::PAYMENTWORKFLOWTYPE_INCOMINGINVOICE: {

            return true;
        }
        case proto::PAYMENTWORKFLOWTYPE_ERROR:
        case proto::PAYMENTWORKFLOWTYPE_OUTGOINGTRANSFER:
        case proto::PAYMENTWORKFLOWTYPE_INCOMINGTRANSFER:
        case proto::PAYMENTWORKFLOWTYPE_INTERNALTRANSFER:
        case proto::PAYMENTWORKFLOWTYPE_OUTGOINGCASH:
        case proto::PAYMENTWORKFLOWTYPE_INCOMINGCASH:
        default: {
        }
    }

    return false;
}

auto Workflow::ContainsTransfer(const proto::PaymentWorkflow& workflow) -> bool
{
    switch (workflow.type()) {
        case proto::PAYMENTWORKFLOWTYPE_OUTGOINGTRANSFER:
        case proto::PAYMENTWORKFLOWTYPE_INCOMINGTRANSFER:
        case proto::PAYMENTWORKFLOWTYPE_INTERNALTRANSFER: {

            return true;
        }
        case proto::PAYMENTWORKFLOWTYPE_ERROR:
        case proto::PAYMENTWORKFLOWTYPE_OUTGOINGCHEQUE:
        case proto::PAYMENTWORKFLOWTYPE_INCOMINGCHEQUE:
        case proto::PAYMENTWORKFLOWTYPE_OUTGOINGINVOICE:
        case proto::PAYMENTWORKFLOWTYPE_INCOMINGINVOICE:
        case proto::PAYMENTWORKFLOWTYPE_OUTGOINGCASH:
        case proto::PAYMENTWORKFLOWTYPE_INCOMINGCASH:
        default: {
        }
    }

    return false;
}

auto Workflow::ExtractCheque(const proto::PaymentWorkflow& workflow)
    -> std::string
{
    if (false == ContainsCheque(workflow)) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Wrong workflow type").Flush();

        return {};
    }

    if (1 != workflow.source().size()) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Invalid workflow").Flush();

        return {};
    }

    return workflow.source(0).item();
}

#if OT_CASH
auto Workflow::ExtractPurse(const proto::PaymentWorkflow& workflow)
    -> std::unique_ptr<proto::Purse>
{
    if (false == ContainsCash(workflow)) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Wrong workflow type").Flush();

        return {};
    }

    if (1 != workflow.source().size()) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Invalid workflow").Flush();

        return {};
    }

    auto output = std::make_unique<proto::Purse>();

    OT_ASSERT(output);

    const auto& serialized = workflow.source(0).item();
    *output = proto::Factory<proto::Purse>(serialized);

    return output;
}
#endif

auto Workflow::ExtractTransfer(const proto::PaymentWorkflow& workflow)
    -> std::string
{
    if (false == ContainsTransfer(workflow)) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Wrong workflow type").Flush();

        return {};
    }

    if (1 != workflow.source().size()) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Invalid workflow").Flush();

        return {};
    }

    return workflow.source(0).item();
}

auto Workflow::InstantiateCheque(
    const api::internal::Core& core,
    const proto::PaymentWorkflow& workflow) -> Workflow::Cheque
{
    Cheque output{proto::PAYMENTWORKFLOWSTATE_ERROR, nullptr};
    auto& [state, cheque] = output;

    switch (workflow.type()) {
        case proto::PAYMENTWORKFLOWTYPE_OUTGOINGCHEQUE:
        case proto::PAYMENTWORKFLOWTYPE_INCOMINGCHEQUE:
        case proto::PAYMENTWORKFLOWTYPE_OUTGOINGINVOICE:
        case proto::PAYMENTWORKFLOWTYPE_INCOMINGINVOICE: {
            cheque.reset(core.Factory().Cheque().release());

            OT_ASSERT(cheque)

            const auto serialized = ExtractCheque(workflow);

            if (serialized.empty()) { return output; }

            const auto loaded = cheque->LoadContractFromString(
                String::Factory(serialized.c_str()));

            if (false == loaded) {
                LogOutput(OT_METHOD)(__FUNCTION__)(
                    ": Failed to instantiate cheque")
                    .Flush();
                cheque.reset();

                return output;
            }

            state = workflow.state();
        } break;
        case proto::PAYMENTWORKFLOWTYPE_ERROR:
        case proto::PAYMENTWORKFLOWTYPE_OUTGOINGTRANSFER:
        case proto::PAYMENTWORKFLOWTYPE_INCOMINGTRANSFER:
        case proto::PAYMENTWORKFLOWTYPE_INTERNALTRANSFER:
        case proto::PAYMENTWORKFLOWTYPE_OUTGOINGCASH:
        case proto::PAYMENTWORKFLOWTYPE_INCOMINGCASH:
        default: {
            LogOutput(OT_METHOD)(__FUNCTION__)(": Incorrect workflow type")
                .Flush();
        }
    }

    return output;
}

#if OT_CASH
auto Workflow::InstantiatePurse(
    const api::internal::Core& core,
    const proto::PaymentWorkflow& workflow) -> Workflow::Purse
{
    Purse output{proto::PAYMENTWORKFLOWSTATE_ERROR, nullptr};
    auto& [state, purse] = output;

    switch (workflow.type()) {
        case proto::PAYMENTWORKFLOWTYPE_OUTGOINGCASH:
        case proto::PAYMENTWORKFLOWTYPE_INCOMINGCASH: {
            const auto serialized = ExtractPurse(workflow);

            if (false == bool(serialized)) { return output; }

            purse.reset(core.Factory().Purse(*serialized).release());

            if (false == bool(purse)) {
                LogOutput(OT_METHOD)(__FUNCTION__)(
                    ": Failed to instantiate purse")
                    .Flush();
                purse.reset();

                return output;
            }

            state = workflow.state();
        } break;
        case proto::PAYMENTWORKFLOWTYPE_ERROR:
        case proto::PAYMENTWORKFLOWTYPE_OUTGOINGCHEQUE:
        case proto::PAYMENTWORKFLOWTYPE_INCOMINGCHEQUE:
        case proto::PAYMENTWORKFLOWTYPE_OUTGOINGINVOICE:
        case proto::PAYMENTWORKFLOWTYPE_INCOMINGINVOICE:
        case proto::PAYMENTWORKFLOWTYPE_OUTGOINGTRANSFER:
        case proto::PAYMENTWORKFLOWTYPE_INCOMINGTRANSFER:
        case proto::PAYMENTWORKFLOWTYPE_INTERNALTRANSFER:
        default: {
            LogOutput(OT_METHOD)(__FUNCTION__)(": Incorrect workflow type")
                .Flush();
        }
    }

    return output;
}
#endif

auto Workflow::InstantiateTransfer(
    const api::internal::Core& core,
    const proto::PaymentWorkflow& workflow) -> Workflow::Transfer
{
    Transfer output{proto::PAYMENTWORKFLOWSTATE_ERROR, nullptr};
    auto& [state, transfer] = output;

    switch (workflow.type()) {
        case proto::PAYMENTWORKFLOWTYPE_OUTGOINGTRANSFER:
        case proto::PAYMENTWORKFLOWTYPE_INCOMINGTRANSFER:
        case proto::PAYMENTWORKFLOWTYPE_INTERNALTRANSFER: {
            const auto serialized = ExtractTransfer(workflow);

            if (serialized.empty()) { return output; }

            transfer.reset(core.Factory().Item(serialized).release());

            if (false == bool(transfer)) {
                LogOutput(OT_METHOD)(__FUNCTION__)(
                    ": Failed to instantiate transfer")
                    .Flush();
                transfer.reset();

                return output;
            }

            state = workflow.state();
        } break;

        case proto::PAYMENTWORKFLOWTYPE_ERROR:
        case proto::PAYMENTWORKFLOWTYPE_OUTGOINGCHEQUE:
        case proto::PAYMENTWORKFLOWTYPE_INCOMINGCHEQUE:
        case proto::PAYMENTWORKFLOWTYPE_OUTGOINGINVOICE:
        case proto::PAYMENTWORKFLOWTYPE_INCOMINGINVOICE:
        case proto::PAYMENTWORKFLOWTYPE_OUTGOINGCASH:
        case proto::PAYMENTWORKFLOWTYPE_INCOMINGCASH:
        default: {
            LogOutput(OT_METHOD)(__FUNCTION__)(": Incorrect workflow type")
                .Flush();
        }
    }

    return output;
}

auto Workflow::UUID(
    const api::internal::Core& core,
    const proto::PaymentWorkflow& workflow) -> OTIdentifier
{
    auto output = Identifier::Factory();
    auto notaryID = Identifier::Factory();
    TransactionNumber number{0};

    switch (workflow.type()) {
        case proto::PAYMENTWORKFLOWTYPE_OUTGOINGCHEQUE:
        case proto::PAYMENTWORKFLOWTYPE_INCOMINGCHEQUE:
        case proto::PAYMENTWORKFLOWTYPE_OUTGOINGINVOICE:
        case proto::PAYMENTWORKFLOWTYPE_INCOMINGINVOICE: {
            [[maybe_unused]] auto [state, cheque] =
                InstantiateCheque(core, workflow);

            if (false == bool(cheque)) {
                LogOutput(OT_METHOD)(__FUNCTION__)(": Invalid cheque").Flush();

                return output;
            }

            notaryID = cheque->GetNotaryID();
            number = cheque->GetTransactionNum();
        } break;
        case proto::PAYMENTWORKFLOWTYPE_OUTGOINGTRANSFER:
        case proto::PAYMENTWORKFLOWTYPE_INCOMINGTRANSFER:
        case proto::PAYMENTWORKFLOWTYPE_INTERNALTRANSFER: {
            [[maybe_unused]] auto [state, transfer] =
                InstantiateTransfer(core, workflow);

            if (false == bool(transfer)) {
                LogOutput(OT_METHOD)(__FUNCTION__)(": Invalid transfer")
                    .Flush();

                return output;
            }

            notaryID = transfer->GetPurportedNotaryID();
            number = transfer->GetTransactionNum();
        } break;
        case proto::PAYMENTWORKFLOWTYPE_OUTGOINGCASH:
        case proto::PAYMENTWORKFLOWTYPE_INCOMINGCASH: {
            // TODO
        } break;
        default: {
            LogOutput(OT_METHOD)(__FUNCTION__)(": Unknown workflow type")
                .Flush();
        }
    }

    return UUID(notaryID, number);
}

auto Workflow::UUID(const Identifier& notary, const TransactionNumber& number)
    -> OTIdentifier
{
    LogTrace(OT_METHOD)(__FUNCTION__)(": UUID for notary ")(notary)(
        " and transaction number ")(number)(" is ");
    OTData preimage{notary};
    preimage->Concatenate(&number, sizeof(number));
    auto output = Identifier::Factory();
    output->CalculateDigest(preimage->Bytes());
    LogTrace(output).Flush();

    return output;
}

namespace implementation
{
const Workflow::VersionMap Workflow::versions_{
    {proto::PAYMENTWORKFLOWTYPE_OUTGOINGCHEQUE, {1, 1, 1}},
    {proto::PAYMENTWORKFLOWTYPE_INCOMINGCHEQUE, {1, 1, 1}},
    {proto::PAYMENTWORKFLOWTYPE_OUTGOINGTRANSFER, {2, 1, 2}},
    {proto::PAYMENTWORKFLOWTYPE_INCOMINGTRANSFER, {2, 1, 2}},
    {proto::PAYMENTWORKFLOWTYPE_INTERNALTRANSFER, {2, 1, 2}},
    {proto::PAYMENTWORKFLOWTYPE_OUTGOINGCASH, {3, 1, 3}},
    {proto::PAYMENTWORKFLOWTYPE_INCOMINGCASH, {3, 1, 3}},
};

Workflow::Workflow(
    const api::internal::Core& api,
    const api::client::Activity& activity,
    const api::client::Contacts& contact)
    : api_(api)
    , activity_(activity)
    , contact_(contact)
    , account_publisher_(api_.ZeroMQ().PublishSocket())
    , rpc_publisher_(
          api_.ZeroMQ().PushSocket(zmq::socket::Socket::Direction::Connect))
    , workflow_locks_()
{
    // WARNING: do not access api_.Wallet() during construction
    const auto endpoint = api_.Endpoints().WorkflowAccountUpdate();
    LogDetail(OT_METHOD)(__FUNCTION__)(": Binding to ")(endpoint).Flush();
    auto bound = account_publisher_->Start(endpoint);

    OT_ASSERT(bound)

    bound = rpc_publisher_->Start(
        api_.ZeroMQ().BuildEndpoint("rpc/push/internal", -1, 1));

    OT_ASSERT(bound)
}

auto Workflow::AbortTransfer(
    const identifier::Nym& nymID,
    const Item& transfer,
    const Message& reply) const -> bool
{
    if (false == isTransfer(transfer)) { return false; }

    const bool isInternal = isInternalTransfer(
        transfer.GetRealAccountID(), transfer.GetDestinationAcctID());
    const std::set<proto::PaymentWorkflowType> type{
        isInternal ? proto::PAYMENTWORKFLOWTYPE_INTERNALTRANSFER
                   : proto::PAYMENTWORKFLOWTYPE_OUTGOINGTRANSFER};
    Lock global(lock_);
    const auto workflow = get_workflow(global, type, nymID.str(), transfer);

    if (false == bool(workflow)) {
        LogOutput(OT_METHOD)(__FUNCTION__)(
            ": Workflow for this transfer does not exist.")
            .Flush();

        return false;
    }

    auto lock = get_workflow_lock(global, workflow->id());

    if (false == can_abort_transfer(*workflow)) { return false; }

    return add_transfer_event(
        lock,
        nymID.str(),
        "",
        *workflow,
        proto::PAYMENTWORKFLOWSTATE_ABORTED,
        proto::PAYMENTEVENTTYPE_ABORT,
        (isInternal
             ? versions_.at(proto::PAYMENTWORKFLOWTYPE_INTERNALTRANSFER).event_
             : versions_.at(proto::PAYMENTWORKFLOWTYPE_OUTGOINGTRANSFER)
                   .event_),
        reply,
        transfer.GetRealAccountID(),
        true);
}

// Works for Incoming and Internal transfer workflows.
auto Workflow::AcceptTransfer(
    const identifier::Nym& nymID,
    const identifier::Server& notaryID,
    const OTTransaction& pending,
    const Message& reply) const -> bool
{
    const auto transfer = extract_transfer_from_pending(pending);

    if (false == bool(transfer)) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Invalid transaction").Flush();

        return false;
    }

    const auto senderNymID = transfer->GetNymID().str();
    const auto recipientNymID = pending.GetNymID().str();
    const auto& accountID = pending.GetPurportedAccountID();

    if (pending.GetNymID() != nymID) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Invalid recipient").Flush();

        return false;
    }

    const bool isInternal = (0 == senderNymID.compare(recipientNymID));

    // Ignore this event for internal transfers.
    if (isInternal) { return true; }

    const std::set<proto::PaymentWorkflowType> type{
        proto::PAYMENTWORKFLOWTYPE_INCOMINGTRANSFER};
    Lock global(lock_);
    const auto workflow = get_workflow(global, type, nymID.str(), *transfer);

    if (false == bool(workflow)) {
        LogOutput(OT_METHOD)(__FUNCTION__)(
            ": Workflow for this transfer does not exist.")
            .Flush();

        return false;
    }

    auto lock = get_workflow_lock(global, workflow->id());

    if (false == can_accept_transfer(*workflow)) { return false; }

    return add_transfer_event(
        lock,
        nymID.str(),
        senderNymID,
        *workflow,
        proto::PAYMENTWORKFLOWSTATE_COMPLETED,
        proto::PAYMENTEVENTTYPE_ACCEPT,
        versions_.at(proto::PAYMENTWORKFLOWTYPE_OUTGOINGTRANSFER).event_,
        reply,
        accountID,
        true);
}

auto Workflow::AcknowledgeTransfer(
    const identifier::Nym& nymID,
    const Item& transfer,
    const Message& reply) const -> bool
{
    if (false == isTransfer(transfer)) { return false; }

    const bool isInternal = isInternalTransfer(
        transfer.GetRealAccountID(), transfer.GetDestinationAcctID());
    const std::set<proto::PaymentWorkflowType> type{
        isInternal ? proto::PAYMENTWORKFLOWTYPE_INTERNALTRANSFER
                   : proto::PAYMENTWORKFLOWTYPE_OUTGOINGTRANSFER};
    Lock global(lock_);
    const auto workflow = get_workflow(global, type, nymID.str(), transfer);

    if (false == bool(workflow)) {
        LogOutput(OT_METHOD)(__FUNCTION__)(
            ": Workflow for this transfer does not exist.")
            .Flush();

        return false;
    }

    auto lock = get_workflow_lock(global, workflow->id());

    if (false == can_acknowledge_transfer(*workflow)) { return false; }

    // For internal transfers it's possible that a push notification already
    // advanced the state to conveyed before the sender received the
    // acknowledgement. The timing of those two events is indeterminate,
    // therefore if the state has already advanced, add the acknowledge event
    // but do not change the state.
    const proto::PaymentWorkflowState state =
        (proto::PAYMENTWORKFLOWSTATE_CONVEYED == workflow->state())
            ? proto::PAYMENTWORKFLOWSTATE_CONVEYED
            : proto::PAYMENTWORKFLOWSTATE_ACKNOWLEDGED;

    return add_transfer_event(
        lock,
        nymID.str(),
        "",
        *workflow,
        state,
        proto::PAYMENTEVENTTYPE_ACKNOWLEDGE,
        (isInternal
             ? versions_.at(proto::PAYMENTWORKFLOWTYPE_INTERNALTRANSFER).event_
             : versions_.at(proto::PAYMENTWORKFLOWTYPE_OUTGOINGTRANSFER)
                   .event_),
        reply,
        transfer.GetRealAccountID(),
        true);
}

#if OT_CASH
auto Workflow::AllocateCash(
    const identifier::Nym& id,
    const blind::Purse& purse) const -> OTIdentifier
{
    Lock global(lock_);
    auto workflowID = Identifier::Random();
    proto::PaymentWorkflow workflow{};
    workflow.set_version(
        versions_.at(proto::PAYMENTWORKFLOWTYPE_OUTGOINGCASH).workflow_);
    workflow.set_id(workflowID->str());
    workflow.set_type(proto::PAYMENTWORKFLOWTYPE_OUTGOINGCASH);
    workflow.set_state(proto::PAYMENTWORKFLOWSTATE_UNSENT);
    auto& source = *(workflow.add_source());
    source.set_version(
        versions_.at(proto::PAYMENTWORKFLOWTYPE_OUTGOINGCASH).source_);
    source.set_id(workflowID->str());
    source.set_revision(1);
    source.set_item(proto::ToString(purse.Serialize()));
    workflow.set_notary(purse.Notary().str());
    auto& event = *workflow.add_event();
    event.set_version(
        versions_.at(proto::PAYMENTWORKFLOWTYPE_OUTGOINGCASH).event_);
    event.set_time(Clock::to_time_t(Clock::now()));
    event.set_type(proto::PAYMENTEVENTTYPE_CREATE);
    event.set_method(proto::TRANSPORTMETHOD_NONE);
    event.set_success(true);
    workflow.add_unit(purse.Unit().str());
    const auto saved = save_workflow(id.str(), workflow);

    if (false == saved) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Failed to save workflow").Flush();

        return Identifier::Factory();
    }

    return workflowID;
}
#endif

auto Workflow::add_cheque_event(
    const eLock& lock,
    const std::string& nymID,
    const std::string&,
    proto::PaymentWorkflow& workflow,
    const proto::PaymentWorkflowState newState,
    const proto::PaymentEventType newEventType,
    const VersionNumber version,
    const Message& request,
    const Message* reply,
    const Identifier& account) const -> bool
{
    const bool haveReply = (nullptr != reply);
    const bool success = cheque_deposit_success(reply);

    if (success) {
        workflow.set_state(newState);

        if ((false == account.empty()) && (0 == workflow.account_size())) {
            workflow.add_account(account.str());
        }
    }

    auto& event = *(workflow.add_event());
    event.set_version(version);
    event.set_type(newEventType);
    event.add_item(String::Factory(request)->Get());
    event.set_method(proto::TRANSPORTMETHOD_OT);
    event.set_transport(request.m_strNotaryID->Get());

    switch (newEventType) {
        case proto::PAYMENTEVENTTYPE_CANCEL:
        case proto::PAYMENTEVENTTYPE_COMPLETE: {
        } break;
        case proto::PAYMENTEVENTTYPE_CONVEY:
        case proto::PAYMENTEVENTTYPE_ACCEPT: {
            event.set_nym(request.m_strNymID2->Get());
        } break;
        case proto::PAYMENTEVENTTYPE_ERROR:
        case proto::PAYMENTEVENTTYPE_CREATE:
        default: {
            OT_FAIL
        }
    }

    event.set_success(success);

    if (haveReply) {
        event.add_item(String::Factory(*reply)->Get());
        event.set_time(reply->m_lTime);
    } else {
        event.set_time(request.m_lTime);
    }

    if (false == account.empty()) {
        workflow.set_notary(
            api_.Storage().AccountServer(Identifier::Factory(account))->str());
    }

    return save_workflow(nymID, account, workflow);
}

// Only used for ClearCheque
auto Workflow::add_cheque_event(
    const eLock& lock,
    const std::string& nymID,
    const Identifier& accountID,
    proto::PaymentWorkflow& workflow,
    const proto::PaymentWorkflowState newState,
    const proto::PaymentEventType newEventType,
    const VersionNumber version,
    const identifier::Nym& recipientNymID,
    const OTTransaction& receipt,
    const Time time) const -> bool
{
    auto message = String::Factory();
    receipt.SaveContractRaw(message);
    workflow.set_state(newState);
    auto& event = *(workflow.add_event());
    event.set_version(version);
    event.set_type(newEventType);
    event.add_item(message->Get());
    event.set_time(Clock::to_time_t(time));
    event.set_method(proto::TRANSPORTMETHOD_OT);
    event.set_transport(receipt.GetRealNotaryID().str());
    event.set_nym(recipientNymID.str());
    event.set_success(true);

    if (0 == workflow.party_size()) {
        workflow.add_party(recipientNymID.str());
    }

    return save_workflow(nymID, accountID, workflow);
}

auto Workflow::add_transfer_event(
    const eLock& lock,
    const std::string& nymID,
    const std::string& eventNym,
    proto::PaymentWorkflow& workflow,
    const proto::PaymentWorkflowState newState,
    const proto::PaymentEventType newEventType,
    const VersionNumber version,
    const Message& message,
    const Identifier& account,
    const bool success) const -> bool
{
    if (success) { workflow.set_state(newState); }

    auto& event = *(workflow.add_event());
    event.set_version(version);
    event.set_type(newEventType);
    event.add_item(String::Factory(message)->Get());
    event.set_method(proto::TRANSPORTMETHOD_OT);
    event.set_transport(message.m_strNotaryID->Get());

    switch (newEventType) {
        case proto::PAYMENTEVENTTYPE_CONVEY:
        case proto::PAYMENTEVENTTYPE_ACCEPT:
        case proto::PAYMENTEVENTTYPE_COMPLETE:
        case proto::PAYMENTEVENTTYPE_ABORT:
        case proto::PAYMENTEVENTTYPE_ACKNOWLEDGE: {
            // TODO
        } break;
        case proto::PAYMENTEVENTTYPE_ERROR:
        case proto::PAYMENTEVENTTYPE_CREATE:
        case proto::PAYMENTEVENTTYPE_CANCEL:
        default: {
            OT_FAIL
        }
    }

    event.set_success(success);
    event.set_time(message.m_lTime);

    if (0 == workflow.party_size() && (false == eventNym.empty())) {
        workflow.add_party(eventNym);
    }

    return save_workflow(nymID, account, workflow);
}

auto Workflow::add_transfer_event(
    const eLock& lock,
    const std::string& nymID,
    const std::string& notaryID,
    const std::string& eventNym,
    proto::PaymentWorkflow& workflow,
    const proto::PaymentWorkflowState newState,
    const proto::PaymentEventType newEventType,
    const VersionNumber version,
    const OTTransaction& receipt,
    const Identifier& account,
    const bool success) const -> bool
{
    if (success) { workflow.set_state(newState); }

    auto& event = *(workflow.add_event());
    event.set_version(version);
    event.set_type(newEventType);
    event.add_item(String::Factory(receipt)->Get());
    event.set_method(proto::TRANSPORTMETHOD_OT);
    event.set_transport(notaryID);

    switch (newEventType) {
        case proto::PAYMENTEVENTTYPE_CONVEY:
        case proto::PAYMENTEVENTTYPE_ACCEPT:
        case proto::PAYMENTEVENTTYPE_COMPLETE:
        case proto::PAYMENTEVENTTYPE_ABORT:
        case proto::PAYMENTEVENTTYPE_ACKNOWLEDGE: {
            // TODO
        } break;
        case proto::PAYMENTEVENTTYPE_ERROR:
        case proto::PAYMENTEVENTTYPE_CREATE:
        case proto::PAYMENTEVENTTYPE_CANCEL:
        default: {
            OT_FAIL
        }
    }

    event.set_success(success);
    event.set_time(Clock::to_time_t(Clock::now()));

    if (0 == workflow.party_size() && (false == eventNym.empty())) {
        workflow.add_party(eventNym);
    }

    return save_workflow(nymID, account, workflow);
}

auto Workflow::can_abort_transfer(const proto::PaymentWorkflow& workflow)
    -> bool
{
    bool correctState{false};

    switch (workflow.state()) {
        case proto::PAYMENTWORKFLOWSTATE_INITIATED: {
            correctState = true;
        } break;
        default: {
        }
    }

    if (false == correctState) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Incorrect workflow state.")
            .Flush();

        return false;
    }

    return true;
}

auto Workflow::can_accept_cheque(const proto::PaymentWorkflow& workflow) -> bool
{
    bool correctState{false};

    switch (workflow.state()) {
        case proto::PAYMENTWORKFLOWSTATE_EXPIRED:
        case proto::PAYMENTWORKFLOWSTATE_CONVEYED: {
            correctState = true;
        } break;
        default: {
        }
    }

    if (false == correctState) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Incorrect workflow state.")
            .Flush();

        return false;
    }

    return true;
}

auto Workflow::can_accept_transfer(const proto::PaymentWorkflow& workflow)
    -> bool
{
    bool correctState{false};

    switch (workflow.state()) {
        case proto::PAYMENTWORKFLOWSTATE_CONVEYED: {
            correctState = true;
        } break;
        default: {
        }
    }

    if (false == correctState) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Incorrect workflow state.")
            .Flush();

        return false;
    }

    return true;
}

auto Workflow::can_acknowledge_transfer(const proto::PaymentWorkflow& workflow)
    -> bool
{
    bool correctState{false};

    switch (workflow.state()) {
        case proto::PAYMENTWORKFLOWSTATE_INITIATED:
        case proto::PAYMENTWORKFLOWSTATE_CONVEYED: {
            correctState = true;
        } break;
        default: {
        }
    }

    if (false == correctState) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Incorrect workflow state (")(
            workflow.state())(")")
            .Flush();

        return false;
    }

    return true;
}

auto Workflow::can_cancel_cheque(const proto::PaymentWorkflow& workflow) -> bool
{
    bool correctState{false};

    switch (workflow.state()) {
        case proto::PAYMENTWORKFLOWSTATE_UNSENT:
        case proto::PAYMENTWORKFLOWSTATE_CONVEYED: {
            correctState = true;
        } break;
        default: {
        }
    }

    if (false == correctState) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Incorrect workflow state.")
            .Flush();

        return false;
    }

    return true;
}

auto Workflow::can_clear_transfer(const proto::PaymentWorkflow& workflow)
    -> bool
{
    bool correctState{false};

    switch (workflow.type()) {
        case proto::PAYMENTWORKFLOWTYPE_OUTGOINGTRANSFER: {
            correctState =
                (proto::PAYMENTWORKFLOWSTATE_ACKNOWLEDGED == workflow.state());
        } break;
        case proto::PAYMENTWORKFLOWTYPE_INTERNALTRANSFER: {
            correctState =
                (proto::PAYMENTWORKFLOWSTATE_CONVEYED == workflow.state());
        } break;
        default: {
        }
    }

    if (false == correctState) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Incorrect workflow state.")
            .Flush();

        return false;
    }

    return true;
}

auto Workflow::can_complete_transfer(const proto::PaymentWorkflow& workflow)
    -> bool
{
    if (proto::PAYMENTWORKFLOWSTATE_ACCEPTED != workflow.state()) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Incorrect workflow state (")(
            workflow.state())(")")
            .Flush();

        return false;
    }

    return true;
}

#if OT_CASH
auto Workflow::can_convey_cash(const proto::PaymentWorkflow& workflow) -> bool
{
    if (proto::PAYMENTWORKFLOWSTATE_EXPIRED == workflow.state()) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Incorrect workflow state.")
            .Flush();

        return false;
    }

    return true;
}
#endif

auto Workflow::can_convey_cheque(const proto::PaymentWorkflow& workflow) -> bool
{
    if (proto::PAYMENTWORKFLOWSTATE_UNSENT != workflow.state()) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Incorrect workflow state.")
            .Flush();

        return false;
    }

    return true;
}

auto Workflow::can_convey_transfer(const proto::PaymentWorkflow& workflow)
    -> bool
{
    switch (workflow.state()) {
        case proto::PAYMENTWORKFLOWSTATE_INITIATED:
        case proto::PAYMENTWORKFLOWSTATE_ACKNOWLEDGED: {

            return true;
        }
        case proto::PAYMENTWORKFLOWSTATE_CONVEYED: {
            break;
        }
        default: {
            LogOutput(OT_METHOD)(__FUNCTION__)(": Incorrect workflow state.")
                .Flush();
        }
    }

    return false;
}

auto Workflow::can_deposit_cheque(const proto::PaymentWorkflow& workflow)
    -> bool
{
    if (proto::PAYMENTWORKFLOWSTATE_CONVEYED != workflow.state()) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Incorrect workflow state.")
            .Flush();

        return false;
    }

    return true;
}

auto Workflow::can_expire_cheque(
    const opentxs::Cheque& cheque,
    const proto::PaymentWorkflow& workflow) -> bool
{
    bool correctState{false};

    switch (workflow.type()) {
        case proto::PAYMENTWORKFLOWTYPE_OUTGOINGCHEQUE: {
            switch (workflow.state()) {
                case proto::PAYMENTWORKFLOWSTATE_UNSENT:
                case proto::PAYMENTWORKFLOWSTATE_CONVEYED: {
                    correctState = true;
                } break;
                default: {
                }
            }
        } break;
        case proto::PAYMENTWORKFLOWTYPE_INCOMINGCHEQUE: {
            switch (workflow.state()) {
                case proto::PAYMENTWORKFLOWSTATE_CONVEYED: {
                    correctState = true;
                } break;
                default: {
                }
            }
        } break;
        default: {
            OT_FAIL
        }
    }

    if (false == correctState) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Incorrect workflow state.")
            .Flush();

        return false;
    }

    if (Clock::now() < cheque.GetValidTo()) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Can not expire valid cheque.")
            .Flush();

        return false;
    }

    return true;
}

auto Workflow::can_finish_cheque(const proto::PaymentWorkflow& workflow) -> bool
{
    if (proto::PAYMENTWORKFLOWSTATE_ACCEPTED != workflow.state()) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Incorrect workflow state.")
            .Flush();

        return false;
    }

    return true;
}

auto Workflow::CancelCheque(
    const opentxs::Cheque& cheque,
    const Message& request,
    const Message* reply) const -> bool
{
    if (false == isCheque(cheque)) { return false; }

    const auto nymID = cheque.GetSenderNymID().str();
    Lock global(lock_);
    const auto workflow = get_workflow(
        global, {proto::PAYMENTWORKFLOWTYPE_OUTGOINGCHEQUE}, nymID, cheque);

    if (false == bool(workflow)) {
        LogOutput(OT_METHOD)(__FUNCTION__)(
            ": Workflow for this cheque does not exist.")
            .Flush();

        return false;
    }

    auto lock = get_workflow_lock(global, workflow->id());

    if (false == can_cancel_cheque(*workflow)) { return false; }

    static const auto accountID = api_.Factory().Identifier();

    return add_cheque_event(
        lock,
        nymID,
        "",
        *workflow,
        proto::PAYMENTWORKFLOWSTATE_CANCELLED,
        proto::PAYMENTEVENTTYPE_CANCEL,
        versions_.at(proto::PAYMENTWORKFLOWTYPE_OUTGOINGCHEQUE).event_,
        request,
        reply,
        accountID);
}

auto Workflow::cheque_deposit_success(const Message* message) -> bool
{
    if (nullptr == message) { return false; }

    // TODO this might not be sufficient

    return message->m_bSuccess;
}

auto Workflow::ClearCheque(
    const identifier::Nym& recipientNymID,
    const OTTransaction& receipt) const -> bool
{
    if (recipientNymID.empty()) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Invalid cheque recipient")
            .Flush();

        return false;
    }

    auto cheque{api_.Factory().Cheque(receipt)};

    if (false == bool(cheque)) {
        LogOutput(OT_METHOD)(__FUNCTION__)(
            ": Failed to load cheque from receipt.")
            .Flush();

        return false;
    }

    if (false == isCheque(*cheque)) { return false; }

    const auto nymID = cheque->GetSenderNymID().str();
    Lock global(lock_);
    const auto workflow = get_workflow(
        global, {proto::PAYMENTWORKFLOWTYPE_OUTGOINGCHEQUE}, nymID, *cheque);

    if (false == bool(workflow)) {
        LogOutput(OT_METHOD)(__FUNCTION__)(
            ": Workflow for this cheque does not exist.")
            .Flush();

        return false;
    }

    auto lock = get_workflow_lock(global, workflow->id());

    if (false == can_accept_cheque(*workflow)) { return false; }

    OT_ASSERT(1 == workflow->account_size())

    const bool needNym = (0 == workflow->party_size());
    const auto time = Clock::now();
    const auto output = add_cheque_event(
        lock,
        nymID,
        api_.Factory().Identifier(workflow->account(0)),
        *workflow,
        proto::PAYMENTWORKFLOWSTATE_ACCEPTED,
        proto::PAYMENTEVENTTYPE_ACCEPT,
        versions_.at(proto::PAYMENTWORKFLOWTYPE_OUTGOINGCHEQUE).event_,
        recipientNymID,
        receipt,
        time);

    if (needNym) {
        update_activity(
            cheque->GetSenderNymID(),
            recipientNymID,
            Identifier::Factory(*cheque),
            Identifier::Factory(workflow->id()),
            StorageBox::OUTGOINGCHEQUE,
            extract_conveyed_time(*workflow));
    }

    update_rpc(
        nymID,
        cheque->GetRecipientNymID().str(),
        cheque->SourceAccountID().str(),
        proto::ACCOUNTEVENT_OUTGOINGCHEQUE,
        workflow->id(),
        cheque->GetAmount() * api_.Factory().Amount(-1),
        api_.Factory().Amount(),
        time,
        cheque->GetMemo().Get());

    return output;
}

auto Workflow::ClearTransfer(
    const identifier::Nym& nymID,
    const identifier::Server& notaryID,
    const OTTransaction& receipt) const -> bool
{
    auto depositorNymID = identifier::Nym::Factory();
    const auto transfer =
        extract_transfer_from_receipt(receipt, depositorNymID);

    if (false == bool(transfer)) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Invalid transfer").Flush();

        return false;
    }

    if (depositorNymID->empty()) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Missing recipient").Flush();

        return false;
    }

    contact_.NymToContact(depositorNymID);
    const auto& accountID = transfer->GetPurportedAccountID();

    if (accountID.empty()) {
        LogOutput(OT_METHOD)(__FUNCTION__)(
            ": Transfer does not contain source account ID")
            .Flush();

        return false;
    }

    const auto& destinationAccountID = transfer->GetDestinationAcctID();

    if (destinationAccountID.empty()) {
        LogOutput(OT_METHOD)(__FUNCTION__)(
            ": Transfer does not contain destination account ID")
            .Flush();

        return false;
    }

    const bool isInternal = isInternalTransfer(accountID, destinationAccountID);
    const std::set<proto::PaymentWorkflowType> type{
        isInternal ? proto::PAYMENTWORKFLOWTYPE_INTERNALTRANSFER
                   : proto::PAYMENTWORKFLOWTYPE_OUTGOINGTRANSFER};
    Lock global(lock_);
    const auto workflow = get_workflow(global, type, nymID.str(), *transfer);

    if (false == bool(workflow)) {
        LogOutput(OT_METHOD)(__FUNCTION__)(
            ": Workflow for this transfer does not exist.")
            .Flush();

        return false;
    }

    auto lock = get_workflow_lock(global, workflow->id());

    if (false == can_clear_transfer(*workflow)) { return false; }

    const auto output = add_transfer_event(
        lock,
        nymID.str(),
        notaryID.str(),
        (isInternal ? std::string{""} : depositorNymID->str()),
        *workflow,
        proto::PAYMENTWORKFLOWSTATE_ACCEPTED,
        proto::PAYMENTEVENTTYPE_ACCEPT,
        (isInternal
             ? versions_.at(proto::PAYMENTWORKFLOWTYPE_INTERNALTRANSFER).event_
             : versions_.at(proto::PAYMENTWORKFLOWTYPE_OUTGOINGTRANSFER)
                   .event_),
        receipt,
        accountID,
        true);

    if (output) {
        const auto time = extract_conveyed_time(*workflow);
        auto note = String::Factory();
        transfer->GetNote(note);
        update_activity(
            nymID,
            depositorNymID,
            Identifier::Factory(*transfer),
            Identifier::Factory(workflow->id()),
            StorageBox::OUTGOINGTRANSFER,
            time);
        update_rpc(
            nymID.str(),
            depositorNymID->str(),
            accountID.str(),
            proto::ACCOUNTEVENT_OUTGOINGTRANSFER,
            workflow->id(),
            transfer->GetAmount(),
            api_.Factory().Amount(),
            time,
            note->Get());
    }

    return output;
}

// Works for outgoing and internal transfer workflows.
auto Workflow::CompleteTransfer(
    const identifier::Nym& nymID,
    const identifier::Server& notaryID,
    const OTTransaction& receipt,
    const Message& reply) const -> bool
{
    auto depositorNymID = Identifier::Factory();
    const auto transfer =
        extract_transfer_from_receipt(receipt, depositorNymID);

    if (false == bool(transfer)) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Invalid transfer").Flush();

        return false;
    }

    const auto& accountID = transfer->GetPurportedAccountID();

    if (accountID.empty()) {
        LogOutput(OT_METHOD)(__FUNCTION__)(
            ": Transfer does not contain source account ID")
            .Flush();

        return false;
    }

    const auto& destinationAccountID = transfer->GetDestinationAcctID();

    if (destinationAccountID.empty()) {
        LogOutput(OT_METHOD)(__FUNCTION__)(
            ": Transfer does not contain destination account ID")
            .Flush();

        return false;
    }

    const bool isInternal = isInternalTransfer(accountID, destinationAccountID);
    const std::set<proto::PaymentWorkflowType> type{
        isInternal ? proto::PAYMENTWORKFLOWTYPE_INTERNALTRANSFER
                   : proto::PAYMENTWORKFLOWTYPE_OUTGOINGTRANSFER};
    Lock global(lock_);
    const auto workflow = get_workflow(global, type, nymID.str(), *transfer);

    if (false == bool(workflow)) {
        LogOutput(OT_METHOD)(__FUNCTION__)(
            ": Workflow for this transfer does not exist.")
            .Flush();

        return false;
    }

    auto lock = get_workflow_lock(global, workflow->id());

    if (false == can_complete_transfer(*workflow)) { return false; }

    return add_transfer_event(
        lock,
        nymID.str(),
        notaryID.str(),
        (isInternal ? std::string{""} : depositorNymID->str()),
        *workflow,
        proto::PAYMENTWORKFLOWSTATE_COMPLETED,
        proto::PAYMENTEVENTTYPE_COMPLETE,
        (isInternal
             ? versions_.at(proto::PAYMENTWORKFLOWTYPE_INTERNALTRANSFER).event_
             : versions_.at(proto::PAYMENTWORKFLOWTYPE_OUTGOINGTRANSFER)
                   .event_),
        receipt,
        transfer->GetRealAccountID(),
        true);
}

// NOTE: Since this is an INCOMING transfer, then we need to CREATE its
// corresponding transfer workflow, since it does not already exist.
//
// (Whereas if this had been an INTERNAL transfer, then it would ALREADY
// have been created, and thus we'd need to GET the existing workflow, and
// then add the new event to it).
auto Workflow::convey_incoming_transfer(
    const identifier::Nym& nymID,
    const identifier::Server& notaryID,
    const OTTransaction& pending,
    const std::string& senderNymID,
    const std::string& recipientNymID,
    const Item& transfer) const -> OTIdentifier
{
    Lock global(lock_);
    const auto existing = get_workflow(
        global,
        {proto::PAYMENTWORKFLOWTYPE_INCOMINGTRANSFER},
        nymID.str(),
        transfer);

    if (existing) {
        LogOutput(OT_METHOD)(__FUNCTION__)(
            ": Workflow for this transfer already exist.")
            .Flush();

        return Identifier::Factory(existing->id());
    }

    const auto& accountID = pending.GetPurportedAccountID();
    const auto [workflowID, workflow] = create_transfer(
        global,
        nymID.str(),
        transfer,
        proto::PAYMENTWORKFLOWTYPE_INCOMINGTRANSFER,
        proto::PAYMENTWORKFLOWSTATE_CONVEYED,
        versions_.at(proto::PAYMENTWORKFLOWTYPE_INCOMINGTRANSFER).workflow_,
        versions_.at(proto::PAYMENTWORKFLOWTYPE_INCOMINGTRANSFER).source_,
        versions_.at(proto::PAYMENTWORKFLOWTYPE_INCOMINGTRANSFER).event_,
        senderNymID,
        accountID,
        notaryID.str(),
        "");

    if (false == workflowID->empty()) {
        const auto time = extract_conveyed_time(workflow);
        auto note = String::Factory();
        transfer.GetNote(note);
        update_activity(
            nymID,
            transfer.GetNymID(),
            Identifier::Factory(transfer),
            workflowID,
            StorageBox::INCOMINGTRANSFER,
            time);
        update_rpc(
            recipientNymID,
            senderNymID,
            accountID.str(),
            proto::ACCOUNTEVENT_INCOMINGTRANSFER,
            workflowID->str(),
            transfer.GetAmount(),
            api_.Factory().Amount(),
            time,
            note->Get());
    }

    return workflowID;
}

// NOTE: Since this is an INTERNAL transfer, then it was already CREATED,
// and thus we need to GET the existing workflow, and then add the new
// event to it.
// Whereas if this is an INCOMING transfer, then we need to CREATE its
// corresponding transfer workflow since it does not already exist.
auto Workflow::convey_internal_transfer(
    const identifier::Nym& nymID,
    const identifier::Server& notaryID,
    const OTTransaction& pending,
    const std::string& senderNymID,
    const Item& transfer) const -> OTIdentifier
{
    Lock global(lock_);
    const auto workflow = get_workflow(
        global,
        {proto::PAYMENTWORKFLOWTYPE_INTERNALTRANSFER},
        nymID.str(),
        transfer);

    if (false == bool(workflow)) {
        LogOutput(OT_METHOD)(__FUNCTION__)(
            ": Workflow for this transfer does not exist.")
            .Flush();

        return Identifier::Factory();
    }

    auto lock = get_workflow_lock(global, workflow->id());

    if (false == can_convey_transfer(*workflow)) {
        return Identifier::Factory();
    }

    const auto output = add_transfer_event(
        lock,
        nymID.str(),
        notaryID.str(),
        "",
        *workflow,
        proto::PAYMENTWORKFLOWSTATE_CONVEYED,
        proto::PAYMENTEVENTTYPE_CONVEY,
        versions_.at(proto::PAYMENTWORKFLOWTYPE_INTERNALTRANSFER).event_,
        pending,
        transfer.GetDestinationAcctID(),
        true);

    if (output) {

        return Identifier::Factory(workflow->id());
    } else {

        return Identifier::Factory();
    }
}

auto Workflow::ConveyTransfer(
    const identifier::Nym& nymID,
    const identifier::Server& notaryID,
    const OTTransaction& pending) const -> OTIdentifier
{
    const auto transfer = extract_transfer_from_pending(pending);

    if (false == bool(transfer)) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Invalid transaction").Flush();

        return Identifier::Factory();
    }

    const auto senderNymID = transfer->GetNymID().str();
    contact_.NymToContact(transfer->GetNymID());
    const auto recipientNymID = pending.GetNymID().str();

    if (pending.GetNymID() != nymID) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Invalid recipient").Flush();

        return Identifier::Factory();
    }

    const bool isInternal = (0 == senderNymID.compare(recipientNymID));

    if (isInternal) {

        return convey_internal_transfer(
            nymID, notaryID, pending, senderNymID, *transfer);
    } else {

        return convey_incoming_transfer(

            nymID, notaryID, pending, senderNymID, recipientNymID, *transfer);
    }
}

auto Workflow::create_cheque(
    const Lock& lock,
    const std::string& nymID,
    const opentxs::Cheque& cheque,
    const proto::PaymentWorkflowType workflowType,
    const proto::PaymentWorkflowState workflowState,
    const VersionNumber workflowVersion,
    const VersionNumber sourceVersion,
    const VersionNumber eventVersion,
    const std::string& party,
    const Identifier& account,
    const Message* message) const
    -> std::pair<OTIdentifier, proto::PaymentWorkflow>
{
    OT_ASSERT(verify_lock(lock))

    std::pair<OTIdentifier, proto::PaymentWorkflow> output{
        Identifier::Factory(), {}};
    auto& [workflowID, workflow] = output;
    const auto chequeID = Identifier::Factory(cheque);
    const std::string serialized = String::Factory(cheque)->Get();
    workflowID = Identifier::Random();
    workflow.set_version(workflowVersion);
    workflow.set_id(workflowID->str());
    workflow.set_type(workflowType);
    workflow.set_state(workflowState);
    auto& source = *(workflow.add_source());
    source.set_version(sourceVersion);
    source.set_id(chequeID->str());
    source.set_revision(1);
    source.set_item(serialized);

    // add party if it was passed in and is not already present
    if ((false == party.empty()) && (0 == workflow.party_size())) {
        workflow.add_party(party);
    }

    auto& event = *workflow.add_event();
    event.set_version(eventVersion);

    if (nullptr != message) {
        event.set_type(proto::PAYMENTEVENTTYPE_CONVEY);
        event.add_item(String::Factory(*message)->Get());
        event.set_time(message->m_lTime);
        event.set_method(proto::TRANSPORTMETHOD_OT);
        event.set_transport(message->m_strNotaryID->Get());
    } else {
        event.set_time(Clock::to_time_t(Clock::now()));

        if (proto::PAYMENTWORKFLOWSTATE_UNSENT == workflowState) {
            event.set_type(proto::PAYMENTEVENTTYPE_CREATE);
            event.set_method(proto::TRANSPORTMETHOD_NONE);
        } else if (proto::PAYMENTWORKFLOWSTATE_CONVEYED == workflowState) {
            event.set_type(proto::PAYMENTEVENTTYPE_CONVEY);
            event.set_method(proto::TRANSPORTMETHOD_OOB);
        } else {
            OT_FAIL
        }
    }

    if (false == party.empty()) { event.set_nym(party); }

    event.set_success(true);
    workflow.add_unit(cheque.GetInstrumentDefinitionID().str());

    // add account if it was passed in and is not already present
    if ((false == account.empty()) && (0 == workflow.account_size())) {
        workflow.add_account(account.str());
    }

    if ((false == account.empty()) && (workflow.notary().empty())) {
        workflow.set_notary(
            api_.Storage().AccountServer(Identifier::Factory(account))->str());
    }

    if (workflow.notary().empty() && (nullptr != message)) {
        workflow.set_notary(message->m_strNotaryID->Get());
    }

    return save_workflow(std::move(output), nymID, account, workflow);
}

auto Workflow::create_transfer(
    const Lock& global,
    const std::string& nymID,
    const Item& transfer,
    const proto::PaymentWorkflowType workflowType,
    const proto::PaymentWorkflowState workflowState,
    const VersionNumber workflowVersion,
    const VersionNumber sourceVersion,
    const VersionNumber eventVersion,
    const std::string& party,
    const Identifier& account,
    const std::string& notaryID,
    const std::string& destinationAccountID) const
    -> std::pair<OTIdentifier, proto::PaymentWorkflow>
{
    OT_ASSERT(verify_lock(global))
    OT_ASSERT(false == nymID.empty());
    OT_ASSERT(false == account.empty());
    OT_ASSERT(false == notaryID.empty());

    std::pair<OTIdentifier, proto::PaymentWorkflow> output{
        Identifier::Factory(), {}};
    auto& [workflowID, workflow] = output;
    const auto transferID = Identifier::Factory(transfer);
    LogVerbose(OT_METHOD)(__FUNCTION__)(": Transfer ID: ")(transferID).Flush();
    const std::string serialized = String::Factory(transfer)->Get();
    const auto existing = get_workflow(global, {workflowType}, nymID, transfer);

    if (existing) {
        LogOutput(OT_METHOD)(__FUNCTION__)(
            ": Workflow for this transfer already exists.")
            .Flush();
        workflowID = Identifier::Factory(existing->id());

        return output;
    }

    workflowID = Identifier::Random();
    workflow.set_version(workflowVersion);
    workflow.set_id(workflowID->str());
    workflow.set_type(workflowType);
    workflow.set_state(workflowState);
    auto& source = *(workflow.add_source());
    source.set_version(sourceVersion);
    source.set_id(transferID->str());
    source.set_revision(1);
    source.set_item(serialized);
    workflow.set_notary(notaryID);

    // add party if it was passed in and is not already present
    if ((false == party.empty()) && (0 == workflow.party_size())) {
        workflow.add_party(party);
    }

    auto& event = *workflow.add_event();
    event.set_version(eventVersion);
    event.set_time(Clock::to_time_t(Clock::now()));

    if (proto::PAYMENTWORKFLOWSTATE_INITIATED == workflowState) {
        event.set_type(proto::PAYMENTEVENTTYPE_CREATE);
        event.set_method(proto::TRANSPORTMETHOD_OT);
    } else if (proto::PAYMENTWORKFLOWSTATE_CONVEYED == workflowState) {
        event.set_type(proto::PAYMENTEVENTTYPE_CONVEY);
        event.set_method(proto::TRANSPORTMETHOD_OT);
    } else {
        OT_FAIL
    }

    event.set_transport(notaryID);

    if (false == party.empty()) { event.set_nym(party); }

    event.set_success(true);
    workflow.add_unit(
        api_.Storage().AccountContract(Identifier::Factory(account))->str());

    // add account if it is not already present
    if (0 == workflow.account_size()) {
        workflow.add_account(account.str());

        if (false == destinationAccountID.empty()) {
            workflow.add_account(destinationAccountID);
        }
    }

    return save_workflow(std::move(output), nymID, account, workflow);
}

// Creates outgoing and internal transfer workflows.
auto Workflow::CreateTransfer(const Item& transfer, const Message& request)
    const -> OTIdentifier
{
    if (false == isTransfer(transfer)) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Invalid item type on object")
            .Flush();

        return Identifier::Factory();
    }

    const String& senderNymID = request.m_strNymID;
    const auto& accountID = transfer.GetRealAccountID();
    const bool isInternal =
        isInternalTransfer(accountID, transfer.GetDestinationAcctID());
    Lock global(lock_);
    const auto existing = get_workflow(
        global,
        {isInternal ? proto::PAYMENTWORKFLOWTYPE_INTERNALTRANSFER
                    : proto::PAYMENTWORKFLOWTYPE_OUTGOINGTRANSFER},
        senderNymID.Get(),
        transfer);

    if (existing) {
        LogOutput(OT_METHOD)(__FUNCTION__)(
            ": Workflow for this transfer already exist.")
            .Flush();

        return Identifier::Factory(existing->id());
    }

    const auto [workflowID, workflow] = create_transfer(
        global,
        senderNymID.Get(),
        transfer,
        (isInternal ? proto::PAYMENTWORKFLOWTYPE_INTERNALTRANSFER
                    : proto::PAYMENTWORKFLOWTYPE_OUTGOINGTRANSFER),
        proto::PAYMENTWORKFLOWSTATE_INITIATED,
        (isInternal ? versions_.at(proto::PAYMENTWORKFLOWTYPE_INTERNALTRANSFER)
                          .workflow_
                    : versions_.at(proto::PAYMENTWORKFLOWTYPE_OUTGOINGTRANSFER)
                          .workflow_),
        (isInternal
             ? versions_.at(proto::PAYMENTWORKFLOWTYPE_INTERNALTRANSFER).source_
             : versions_.at(proto::PAYMENTWORKFLOWTYPE_OUTGOINGTRANSFER)
                   .source_),
        (isInternal
             ? versions_.at(proto::PAYMENTWORKFLOWTYPE_INTERNALTRANSFER).event_
             : versions_.at(proto::PAYMENTWORKFLOWTYPE_OUTGOINGTRANSFER)
                   .event_),
        "",
        accountID,
        request.m_strNotaryID->Get(),
        (isInternal ? transfer.GetDestinationAcctID().str() : ""));

    if (false == workflowID->empty()) {
        const auto time = extract_conveyed_time(workflow);
        auto note = String::Factory();
        transfer.GetNote(note);
        update_rpc(
            senderNymID.Get(),
            "",
            accountID.str(),
            proto::ACCOUNTEVENT_OUTGOINGTRANSFER,
            workflowID->str(),
            transfer.GetAmount(),
            api_.Factory().Amount(),
            time,
            note->Get());
    }

    return workflowID;
}

auto Workflow::DepositCheque(
    const identifier::Nym& receiver,
    const Identifier& accountID,
    const opentxs::Cheque& cheque,
    const Message& request,
    const Message* reply) const -> bool
{
    if (false == isCheque(cheque)) { return false; }

    const auto nymID = receiver.str();
    Lock global(lock_);
    const auto workflow = get_workflow(
        global, {proto::PAYMENTWORKFLOWTYPE_INCOMINGCHEQUE}, nymID, cheque);

    if (false == bool(workflow)) {
        LogOutput(OT_METHOD)(__FUNCTION__)(
            ": Workflow for this cheque does not exist.")
            .Flush();

        return false;
    }

    auto lock = get_workflow_lock(global, workflow->id());

    if (false == can_deposit_cheque(*workflow)) { return false; }

    const auto output = add_cheque_event(
        lock,
        nymID,
        cheque.GetSenderNymID().str(),
        *workflow,
        proto::PAYMENTWORKFLOWSTATE_COMPLETED,
        proto::PAYMENTEVENTTYPE_ACCEPT,
        versions_.at(proto::PAYMENTWORKFLOWTYPE_INCOMINGCHEQUE).event_,
        request,
        reply,
        accountID);

    if (output && cheque_deposit_success(reply)) {
        update_rpc(
            receiver.str(),
            cheque.GetSenderNymID().str(),
            accountID.str(),
            proto::ACCOUNTEVENT_INCOMINGCHEQUE,
            workflow->id(),
            cheque.GetAmount(),
            api_.Factory().Amount(),
            Clock::from_time_t(reply->m_lTime),
            cheque.GetMemo().Get());
    }

    return output;
}

auto Workflow::ExpireCheque(
    const identifier::Nym& nym,
    const opentxs::Cheque& cheque) const -> bool
{
    if (false == isCheque(cheque)) { return false; }

    const auto nymID = nym.str();
    Lock global(lock_);
    const auto workflow = get_workflow(
        global,
        {proto::PAYMENTWORKFLOWTYPE_OUTGOINGCHEQUE,
         proto::PAYMENTWORKFLOWTYPE_INCOMINGCHEQUE},
        nymID,
        cheque);

    if (false == bool(workflow)) {
        LogOutput(OT_METHOD)(__FUNCTION__)(
            ": Workflow for this cheque does not exist.")
            .Flush();

        return false;
    }

    auto lock = get_workflow_lock(global, workflow->id());

    if (false == can_expire_cheque(cheque, *workflow)) { return false; }

    workflow->set_state(proto::PAYMENTWORKFLOWSTATE_EXPIRED);

    return save_workflow(nymID, cheque.GetSenderAcctID(), *workflow);
}

auto Workflow::ExportCheque(const opentxs::Cheque& cheque) const -> bool
{
    if (false == isCheque(cheque)) { return false; }

    const auto nymID = cheque.GetSenderNymID().str();
    Lock global(lock_);
    const auto workflow = get_workflow(global, {}, nymID, cheque);

    if (false == bool(workflow)) {
        LogOutput(OT_METHOD)(__FUNCTION__)(
            ": Workflow for this cheque does not exist.")
            .Flush();

        return false;
    }

    auto lock = get_workflow_lock(global, workflow->id());

    if (false == can_convey_cheque(*workflow)) { return false; }

    workflow->set_state(proto::PAYMENTWORKFLOWSTATE_CONVEYED);
    auto& event = *(workflow->add_event());
    event.set_version(
        versions_.at(proto::PAYMENTWORKFLOWTYPE_OUTGOINGCHEQUE).event_);
    event.set_type(proto::PAYMENTEVENTTYPE_CONVEY);
    event.set_time(Clock::to_time_t(Clock::now()));
    event.set_method(proto::TRANSPORTMETHOD_OOB);
    event.set_success(true);

    return save_workflow(nymID, cheque.GetSenderAcctID(), *workflow);
}

auto Workflow::extract_conveyed_time(const proto::PaymentWorkflow& workflow)
    -> Time
{
    for (const auto& event : workflow.event()) {
        if (proto::PAYMENTEVENTTYPE_CONVEY == event.type()) {
            if (event.success()) { return Clock::from_time_t(event.time()); }
        }
    }

    return {};
}

auto Workflow::extract_transfer_from_pending(const OTTransaction& receipt) const
    -> std::unique_ptr<Item>
{
    if (transactionType::pending != receipt.GetType()) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Incorrect receipt type: ")(
            receipt.GetTypeString())
            .Flush();

        return nullptr;
    }

    auto serializedTransfer = String::Factory();
    receipt.GetReferenceString(serializedTransfer);

    if (serializedTransfer->empty()) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Missing serialized transfer item")
            .Flush();

        return nullptr;
    }

    auto transfer = api_.Factory().Item(serializedTransfer);

    if (false == bool(transfer)) {
        LogOutput(OT_METHOD)(__FUNCTION__)(
            ": Unable to instantiate transfer item")
            .Flush();

        return nullptr;
    }

    if (itemType::transfer != transfer->GetType()) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Invalid transfer item type.")
            .Flush();

        return nullptr;
    }

    return transfer;
}

auto Workflow::extract_transfer_from_receipt(
    const OTTransaction& receipt,
    Identifier& depositorNymID) const -> std::unique_ptr<Item>
{
    if (transactionType::transferReceipt != receipt.GetType()) {
        if (transactionType::pending == receipt.GetType()) {

            return extract_transfer_from_pending(receipt);
        } else {
            LogOutput(OT_METHOD)(__FUNCTION__)(": Incorrect receipt type: ")(
                receipt.GetTypeString())
                .Flush();

            return nullptr;
        }
    }

    auto serializedAcceptPending = String::Factory();
    receipt.GetReferenceString(serializedAcceptPending);

    if (serializedAcceptPending->empty()) {
        LogOutput(OT_METHOD)(__FUNCTION__)(
            ": Missing serialized accept pending item")
            .Flush();

        return nullptr;
    }

    const auto acceptPending = api_.Factory().Item(serializedAcceptPending);

    if (false == bool(acceptPending)) {
        LogOutput(OT_METHOD)(__FUNCTION__)(
            ": Unable to instantiate accept pending item")
            .Flush();

        return nullptr;
    }

    if (itemType::acceptPending != acceptPending->GetType()) {
        LogOutput(OT_METHOD)(__FUNCTION__)(
            ": Invalid accept pending item type.")
            .Flush();

        return nullptr;
    }

    depositorNymID.Assign(acceptPending->GetNymID());
    auto serializedPending = String::Factory();
    acceptPending->GetAttachment(serializedPending);

    if (serializedPending->empty()) {
        LogOutput(OT_METHOD)(__FUNCTION__)(
            ": Missing serialized pending transaction")
            .Flush();

        return nullptr;
    }

    auto pending = api_.Factory().Transaction(
        receipt.GetNymID(),
        receipt.GetRealAccountID(),
        receipt.GetRealNotaryID());

    if (false == bool(pending)) {
        LogOutput(OT_METHOD)(__FUNCTION__)(
            ": Unable to instantiate pending transaction")
            .Flush();

        return nullptr;
    }

    const bool loaded = pending->LoadContractFromString(serializedPending);

    if (false == loaded) {
        LogOutput(OT_METHOD)(__FUNCTION__)(
            ": Unable to deserialize pending transaction")
            .Flush();

        return nullptr;
    }

    if (transactionType::pending != pending->GetType()) {
        LogOutput(OT_METHOD)(__FUNCTION__)(
            ": Invalid pending transaction type.")
            .Flush();

        return nullptr;
    }

    auto serializedTransfer = String::Factory();
    pending->GetReferenceString(serializedTransfer);

    if (serializedTransfer->empty()) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Missing serialized transfer item")
            .Flush();

        return nullptr;
    }

    auto transfer = api_.Factory().Item(serializedTransfer);

    if (false == bool(transfer)) {
        LogOutput(OT_METHOD)(__FUNCTION__)(
            ": Unable to instantiate transfer item")
            .Flush();

        return nullptr;
    }

    if (itemType::transfer != transfer->GetType()) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Invalid transfer item type.")
            .Flush();

        return nullptr;
    }

    return transfer;
}

auto Workflow::FinishCheque(
    const opentxs::Cheque& cheque,
    const Message& request,
    const Message* reply) const -> bool
{
    if (false == isCheque(cheque)) { return false; }

    const auto nymID = cheque.GetSenderNymID().str();
    Lock global(lock_);
    const auto workflow = get_workflow(
        global, {proto::PAYMENTWORKFLOWTYPE_OUTGOINGCHEQUE}, nymID, cheque);

    if (false == bool(workflow)) {
        LogOutput(OT_METHOD)(__FUNCTION__)(
            ": Workflow for this cheque does not exist.")
            .Flush();

        return false;
    }

    auto lock = get_workflow_lock(global, workflow->id());

    if (false == can_finish_cheque(*workflow)) { return false; }

    static const auto accountID = api_.Factory().Identifier();

    return add_cheque_event(
        lock,
        nymID,
        "",
        *workflow,
        proto::PAYMENTWORKFLOWSTATE_COMPLETED,
        proto::PAYMENTEVENTTYPE_COMPLETE,
        versions_.at(proto::PAYMENTWORKFLOWTYPE_OUTGOINGCHEQUE).event_,
        request,
        reply,
        accountID);
}

template <typename T>
auto Workflow::get_workflow(
    const Lock& global,
    const std::set<proto::PaymentWorkflowType>& types,
    const std::string& nymID,
    const T& source) const -> std::shared_ptr<proto::PaymentWorkflow>
{
    OT_ASSERT(verify_lock(global));

    const auto itemID = Identifier::Factory(source)->str();
    LogVerbose(OT_METHOD)(__FUNCTION__)(": Item ID: ")(itemID).Flush();

    return get_workflow_by_source(types, nymID, itemID);
}

auto Workflow::get_workflow_by_id(
    const std::string& nymID,
    const std::string& workflowID) const
    -> std::shared_ptr<proto::PaymentWorkflow>
{
    std::shared_ptr<proto::PaymentWorkflow> output{nullptr};
    const auto loaded = api_.Storage().Load(nymID, workflowID, output);

    if (false == loaded) {
        LogDetail(OT_METHOD)(__FUNCTION__)(": Workflow ")(workflowID)(
            " for nym ")(nymID)(" can not be loaded")
            .Flush();

        return output;
    }

    return output;
}

auto Workflow::get_workflow_by_id(
    const std::set<proto::PaymentWorkflowType>& types,
    const std::string& nymID,
    const std::string& workflowID) const
    -> std::shared_ptr<proto::PaymentWorkflow>
{
    auto output = get_workflow_by_id(nymID, workflowID);

    if (0 == types.count(output->type())) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Incorrect type (")(
            output->type())(") on workflow ")(workflowID)(" for nym ")(nymID)
            .Flush();

        return {nullptr};
    }

    return output;
}

auto Workflow::get_workflow_by_source(
    const std::set<proto::PaymentWorkflowType>& types,
    const std::string& nymID,
    const std::string& sourceID) const
    -> std::shared_ptr<proto::PaymentWorkflow>
{
    const auto workflowID =
        api_.Storage().PaymentWorkflowLookup(nymID, sourceID);

    if (workflowID.empty()) { return {}; }

    return get_workflow_by_id(types, nymID, workflowID);
}

auto Workflow::get_workflow_lock(Lock& global, const std::string& id) const
    -> eLock
{
    OT_ASSERT(verify_lock(global));

    auto output = eLock(workflow_locks_[id]);
    global.unlock();

    return output;
}

auto Workflow::ImportCheque(
    const identifier::Nym& nymID,
    const opentxs::Cheque& cheque) const -> OTIdentifier
{
    if (false == isCheque(cheque)) { return Identifier::Factory(); }

    if (false == validate_recipient(nymID, cheque)) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Nym ")(nymID)(
            " can not deposit this cheque.")
            .Flush();

        return Identifier::Factory();
    }

    Lock global(lock_);
    const auto existing = get_workflow(
        global,
        {proto::PAYMENTWORKFLOWTYPE_INCOMINGCHEQUE},
        nymID.str(),
        cheque);

    if (existing) {
        LogOutput(OT_METHOD)(__FUNCTION__)(
            ": Workflow for this cheque already exist.")
            .Flush();

        return Identifier::Factory(existing->id());
    }

    const std::string party = cheque.GetSenderNymID().str();
    static const auto accountID = api_.Factory().Identifier();
    const auto [workflowID, workflow] = create_cheque(
        global,
        nymID.str(),
        cheque,
        proto::PAYMENTWORKFLOWTYPE_INCOMINGCHEQUE,
        proto::PAYMENTWORKFLOWSTATE_CONVEYED,
        versions_.at(proto::PAYMENTWORKFLOWTYPE_INCOMINGCHEQUE).workflow_,
        versions_.at(proto::PAYMENTWORKFLOWTYPE_INCOMINGCHEQUE).source_,
        versions_.at(proto::PAYMENTWORKFLOWTYPE_INCOMINGCHEQUE).event_,
        party,
        accountID);

    if (false == workflowID->empty()) {
        const auto time = extract_conveyed_time(workflow);
        update_activity(
            nymID,
            cheque.GetSenderNymID(),
            Identifier::Factory(cheque),
            workflowID,
            StorageBox::INCOMINGCHEQUE,
            time);
        update_rpc(
            nymID.str(),
            cheque.GetSenderNymID().str(),
            "",
            proto::ACCOUNTEVENT_INCOMINGCHEQUE,
            workflowID->str(),
            api_.Factory().Amount(),
            cheque.GetAmount(),
            time,
            cheque.GetMemo().Get());
    }

    return workflowID;
}

auto Workflow::isCheque(const opentxs::Cheque& cheque) -> bool
{
    if (cheque.HasRemitter()) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Provided instrument is a voucher")
            .Flush();

        return false;
    }

    if (cheque.GetAmount() <= Amount::Factory(0)) {
        LogOutput(OT_METHOD)(__FUNCTION__)(
            ": Provided instrument is an invoice")
            .Flush();

        return false;
    }

    if (cheque.GetAmount() == Amount::Factory(0)) {
        LogOutput(OT_METHOD)(__FUNCTION__)(
            ": Provided instrument is a cancellation")
            .Flush();

        return false;
    }

    return true;
}

auto Workflow::isInternalTransfer(
    const Identifier& sourceAccount,
    const Identifier& destinationAccount) const -> bool
{
    const auto ownerNymID = api_.Storage().AccountOwner(sourceAccount);

    OT_ASSERT(false == ownerNymID->empty());

    const auto recipientNymID = api_.Storage().AccountOwner(destinationAccount);

    if (recipientNymID->empty()) { return false; }

    return ownerNymID == recipientNymID;
}

auto Workflow::isTransfer(const Item& item) -> bool
{
    return itemType::transfer == item.GetType();
}

auto Workflow::List(
    const identifier::Nym& nymID,
    const proto::PaymentWorkflowType type,
    const proto::PaymentWorkflowState state) const -> std::set<OTIdentifier>
{
    const auto input =
        api_.Storage().PaymentWorkflowsByState(nymID.str(), type, state);
    std::set<OTIdentifier> output{};
    std::transform(
        input.begin(),
        input.end(),
        std::inserter(output, output.end()),
        [](const std::string& id) -> OTIdentifier {
            return Identifier::Factory(id);
        });

    return output;
}

auto Workflow::LoadCheque(
    const identifier::Nym& nymID,
    const Identifier& chequeID) const -> Workflow::Cheque
{
    auto workflow = get_workflow_by_source(
        {proto::PAYMENTWORKFLOWTYPE_OUTGOINGCHEQUE,
         proto::PAYMENTWORKFLOWTYPE_INCOMINGCHEQUE},
        nymID.str(),
        chequeID.str());

    if (false == bool(workflow)) {
        LogOutput(OT_METHOD)(__FUNCTION__)(
            ": Workflow for this cheque does not exist.")
            .Flush();

        return {};
    }

    return InstantiateCheque(api_, *workflow);
}

auto Workflow::LoadChequeByWorkflow(
    const identifier::Nym& nymID,
    const Identifier& workflowID) const -> Workflow::Cheque
{
    auto workflow = get_workflow_by_id(
        {proto::PAYMENTWORKFLOWTYPE_OUTGOINGCHEQUE,
         proto::PAYMENTWORKFLOWTYPE_INCOMINGCHEQUE},
        nymID.str(),
        workflowID.str());

    if (false == bool(workflow)) {
        LogOutput(OT_METHOD)(__FUNCTION__)(
            ": Workflow for this cheque does not exist.")
            .Flush();

        return {};
    }

    return InstantiateCheque(api_, *workflow);
}

auto Workflow::LoadTransfer(
    const identifier::Nym& nymID,
    const Identifier& transferID) const -> Workflow::Transfer
{
    auto workflow = get_workflow_by_source(
        {proto::PAYMENTWORKFLOWTYPE_OUTGOINGTRANSFER,
         proto::PAYMENTWORKFLOWTYPE_INCOMINGTRANSFER,
         proto::PAYMENTWORKFLOWTYPE_INTERNALTRANSFER},
        nymID.str(),
        transferID.str());

    if (false == bool(workflow)) {
        LogOutput(OT_METHOD)(__FUNCTION__)(
            ": Workflow for this transfer does not exist.")
            .Flush();

        return {};
    }

    return InstantiateTransfer(api_, *workflow);
}

auto Workflow::LoadTransferByWorkflow(
    const identifier::Nym& nymID,
    const Identifier& workflowID) const -> Workflow::Transfer
{
    auto workflow = get_workflow_by_id(
        {proto::PAYMENTWORKFLOWTYPE_OUTGOINGTRANSFER,
         proto::PAYMENTWORKFLOWTYPE_INCOMINGTRANSFER,
         proto::PAYMENTWORKFLOWTYPE_INTERNALTRANSFER},
        nymID.str(),
        workflowID.str());

    if (false == bool(workflow)) {
        LogOutput(OT_METHOD)(__FUNCTION__)(
            ": Workflow for this transfer does not exist.")
            .Flush();

        return {};
    }

    return InstantiateTransfer(api_, *workflow);
}

auto Workflow::LoadWorkflow(
    const identifier::Nym& nymID,
    const Identifier& workflowID) const
    -> std::shared_ptr<proto::PaymentWorkflow>
{
    return get_workflow_by_id(nymID.str(), workflowID.str());
}

#if OT_CASH
auto Workflow::ReceiveCash(
    const identifier::Nym& receiver,
    const blind::Purse& purse,
    const Message& message) const -> OTIdentifier
{
    Lock global(lock_);
    const std::string serialized = String::Factory(message)->Get();
    const std::string party = message.m_strNymID->Get();
    auto workflowID = Identifier::Random();
    proto::PaymentWorkflow workflow{};
    workflow.set_version(
        versions_.at(proto::PAYMENTWORKFLOWTYPE_INCOMINGCASH).workflow_);
    workflow.set_id(workflowID->str());
    workflow.set_type(proto::PAYMENTWORKFLOWTYPE_INCOMINGCASH);
    workflow.set_state(proto::PAYMENTWORKFLOWSTATE_CONVEYED);
    auto& source = *(workflow.add_source());
    source.set_version(
        versions_.at(proto::PAYMENTWORKFLOWTYPE_INCOMINGCASH).source_);
    source.set_id(workflowID->str());
    source.set_revision(1);
    source.set_item(proto::ToString(purse.Serialize()));
    workflow.set_notary(purse.Notary().str());
    auto& event = *workflow.add_event();
    event.set_version(
        versions_.at(proto::PAYMENTWORKFLOWTYPE_INCOMINGCASH).event_);
    event.set_time(message.m_lTime);
    event.set_type(proto::PAYMENTEVENTTYPE_CONVEY);
    event.set_method(proto::TRANSPORTMETHOD_OT);
    event.set_transport(message.m_strNotaryID->Get());
    event.add_item(serialized);
    event.set_nym(party);
    event.set_success(true);
    workflow.add_unit(purse.Unit().str());
    workflow.add_party(party);
    const auto saved = save_workflow(receiver.str(), workflow);

    if (false == saved) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Failed to save workflow").Flush();

        return Identifier::Factory();
    }

    return workflowID;
}
#endif

auto Workflow::ReceiveCheque(
    const identifier::Nym& nymID,
    const opentxs::Cheque& cheque,
    const Message& message) const -> OTIdentifier
{
    if (false == isCheque(cheque)) { return Identifier::Factory(); }

    if (false == validate_recipient(nymID, cheque)) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Nym ")(nymID)(
            " can not deposit this cheque.")
            .Flush();

        return Identifier::Factory();
    }

    Lock global(lock_);
    const auto existing = get_workflow(
        global,
        {proto::PAYMENTWORKFLOWTYPE_INCOMINGCHEQUE},
        nymID.str(),
        cheque);

    if (existing) {
        LogOutput(OT_METHOD)(__FUNCTION__)(
            ": Workflow for this cheque already exist.")
            .Flush();

        return Identifier::Factory(existing->id());
    }

    const std::string party = cheque.GetSenderNymID().str();
    static const auto accountID = api_.Factory().Identifier();
    const auto [workflowID, workflow] = create_cheque(
        global,
        nymID.str(),
        cheque,
        proto::PAYMENTWORKFLOWTYPE_INCOMINGCHEQUE,
        proto::PAYMENTWORKFLOWSTATE_CONVEYED,
        versions_.at(proto::PAYMENTWORKFLOWTYPE_INCOMINGCHEQUE).workflow_,
        versions_.at(proto::PAYMENTWORKFLOWTYPE_INCOMINGCHEQUE).source_,
        versions_.at(proto::PAYMENTWORKFLOWTYPE_INCOMINGCHEQUE).event_,
        party,
        accountID,
        &message);

    if (false == workflowID->empty()) {
        const auto time = extract_conveyed_time(workflow);
        update_activity(
            nymID,
            cheque.GetSenderNymID(),
            Identifier::Factory(cheque),
            workflowID,
            StorageBox::INCOMINGCHEQUE,
            time);
        update_rpc(
            nymID.str(),
            cheque.GetSenderNymID().str(),
            "",
            proto::ACCOUNTEVENT_INCOMINGCHEQUE,
            workflowID->str(),
            api_.Factory().Amount(),
            cheque.GetAmount(),
            time,
            cheque.GetMemo().Get());
    }

    return workflowID;
}

auto Workflow::save_workflow(
    const std::string& nymID,
    const proto::PaymentWorkflow& workflow) const -> bool
{
    static const auto id = api_.Factory().Identifier();

    return save_workflow(nymID, id, workflow);
}

auto Workflow::save_workflow(
    const std::string& nymID,
    const Identifier& accountID,
    const proto::PaymentWorkflow& workflow) const -> bool
{
    const bool valid = proto::Validate(workflow, VERBOSE);

    OT_ASSERT(valid)

    const auto saved = api_.Storage().Store(nymID, workflow);

    OT_ASSERT(saved)

    if (false == accountID.empty()) {
        auto work =
            api_.ZeroMQ().TaggedMessage(WorkType::WorkflowAccountUpdate);
        work->AddFrame(accountID);
        account_publisher_->Send(work);
    }

    return valid && saved;
}

auto Workflow::save_workflow(
    OTIdentifier&& output,
    const std::string& nymID,
    const Identifier& accountID,
    const proto::PaymentWorkflow& workflow) const -> OTIdentifier
{
    if (save_workflow(nymID, accountID, workflow)) { return std::move(output); }

    return Identifier::Factory();
}

auto Workflow::save_workflow(
    std::pair<OTIdentifier, proto::PaymentWorkflow>&& output,
    const std::string& nymID,
    const Identifier& accountID,
    const proto::PaymentWorkflow& workflow) const
    -> std::pair<OTIdentifier, proto::PaymentWorkflow>
{
    if (save_workflow(nymID, accountID, workflow)) { return std::move(output); }

    return {Identifier::Factory(), {}};
}

#if OT_CASH
auto Workflow::SendCash(
    const identifier::Nym& sender,
    const identifier::Nym& recipient,
    const Identifier& workflowID,
    const Message& request,
    const Message* reply) const -> bool
{
    Lock global(lock_);
    const auto pWorkflow = get_workflow_by_id(sender.str(), workflowID.str());

    if (false == bool(pWorkflow)) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Workflow ")(workflowID)(
            " does not exist.")
            .Flush();

        return false;
    }

    auto& workflow = *pWorkflow;
    auto lock = get_workflow_lock(global, workflowID.str());

    if (false == can_convey_cash(workflow)) { return false; }

    const bool haveReply = (nullptr != reply);

    if (haveReply) { workflow.set_state(proto::PAYMENTWORKFLOWSTATE_CONVEYED); }

    auto& event = *(workflow.add_event());
    event.set_version(
        versions_.at(proto::PAYMENTWORKFLOWTYPE_OUTGOINGCASH).event_);
    event.set_type(proto::PAYMENTEVENTTYPE_CONVEY);
    event.add_item(String::Factory(request)->Get());
    event.set_method(proto::TRANSPORTMETHOD_OT);
    event.set_transport(request.m_strNotaryID->Get());
    event.set_nym(request.m_strNymID2->Get());

    if (haveReply) {
        event.add_item(String::Factory(*reply)->Get());
        event.set_time(reply->m_lTime);
        event.set_success(reply->m_bSuccess);
    } else {
        event.set_time(request.m_lTime);
        event.set_success(false);
    }

    if (0 == workflow.party_size()) { workflow.add_party(recipient.str()); }

    return save_workflow(sender.str(), workflow);
}
#endif

auto Workflow::SendCheque(
    const opentxs::Cheque& cheque,
    const Message& request,
    const Message* reply) const -> bool
{
    if (false == isCheque(cheque)) { return false; }

    const auto nymID = cheque.GetSenderNymID().str();
    Lock global(lock_);
    const auto workflow = get_workflow(
        global, {proto::PAYMENTWORKFLOWTYPE_OUTGOINGCHEQUE}, nymID, cheque);

    if (false == bool(workflow)) {
        LogOutput(OT_METHOD)(__FUNCTION__)(
            ": Workflow for this cheque does not exist.")
            .Flush();

        return false;
    }

    auto lock = get_workflow_lock(global, workflow->id());

    if (false == can_convey_cheque(*workflow)) { return false; }

    static const auto accountID = api_.Factory().Identifier();

    return add_cheque_event(
        lock,
        nymID,
        request.m_strNymID2->Get(),
        *workflow,
        proto::PAYMENTWORKFLOWSTATE_CONVEYED,
        proto::PAYMENTEVENTTYPE_CONVEY,
        versions_.at(proto::PAYMENTWORKFLOWTYPE_OUTGOINGCHEQUE).event_,
        request,
        reply,
        accountID);
}

auto Workflow::update_activity(
    const identifier::Nym& localNymID,
    const identifier::Nym& remoteNymID,
    const Identifier& sourceID,
    const Identifier& workflowID,
    const StorageBox type,
    Time time) const -> bool
{
    const auto contactID = contact_.ContactID(remoteNymID);

    if (contactID->empty()) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Contact for nym ")(remoteNymID)(
            " does not exist")
            .Flush();

        return false;
    }

    const bool added = activity_.AddPaymentEvent(
        localNymID, contactID, type, sourceID, workflowID, time);

    if (added) {
        LogDetail(OT_METHOD)(__FUNCTION__)(
            ": Success adding payment event to thread ")(contactID->str())
            .Flush();

        return true;
    } else {
        LogOutput(OT_METHOD)(__FUNCTION__)(
            ": Failed to add payment event to thread ")(contactID->str())
            .Flush();

        return false;
    }
}

void Workflow::update_rpc(
    const std::string& localNymID,
    const std::string& remoteNymID,
    const std::string& accountID,
    const proto::AccountEventType type,
    const std::string& workflowID,
    const Amount& amount,
    const Amount& pending,
    const Time time,
    const std::string& memo) const
{
    proto::RPCPush push{};
    push.set_version(RPC_PUSH_VERSION);
    push.set_type(proto::RPCPUSH_ACCOUNT);
    push.set_id(localNymID);
    auto& event = *push.mutable_accountevent();
    event.set_version(RPC_ACCOUNT_EVENT_VERSION);
    event.set_id(accountID);
    event.set_type(type);

    if (false == remoteNymID.empty()) {
        event.set_contact(
            contact_.NymToContact(identifier::Nym::Factory(remoteNymID))
                ->str());
    }

    event.set_workflow(workflowID);
//    event.set_amount(amount);
//    event.set_pendingamount(pending);
    event.set_amount_mp(amount.str());
    event.set_pendingamount_mp(pending.str());
    event.set_timestamp(Clock::to_time_t(time));
    event.set_memo(memo);

    OT_ASSERT(proto::Validate(push, VERBOSE));

    auto message = zmq::Message::Factory();
    message->PrependEmptyFrame();
    message->AddFrame(localNymID);
    message->AddFrame(push);
    const auto instance = api_.Instance();
    message->AddFrame(Data::Factory(&instance, sizeof(instance)));
    rpc_publisher_->Send(message);
}

auto Workflow::validate_recipient(
    const identifier::Nym& nymID,
    const opentxs::Cheque& cheque) -> bool
{
    if (nymID.empty()) { return true; }

    return (nymID == cheque.GetRecipientNymID());
}

auto Workflow::WorkflowsByAccount(
    const identifier::Nym& nymID,
    const Identifier& accountID) const -> std::vector<OTIdentifier>
{
    std::vector<OTIdentifier> output{};
    const auto workflows =
        api_.Storage().PaymentWorkflowsByAccount(nymID.str(), accountID.str());
    std::transform(
        workflows.begin(),
        workflows.end(),
        std::inserter(output, output.end()),
        [](const std::string& id) -> OTIdentifier {
            return Identifier::Factory(id);
        });

    return output;
}

auto Workflow::WriteCheque(const opentxs::Cheque& cheque) const -> OTIdentifier
{
    if (false == isCheque(cheque)) {
        LogOutput(OT_METHOD)(__FUNCTION__)(
            ": Invalid item type on cheque object")
            .Flush();

        return Identifier::Factory();
    }

    const auto nymID = cheque.GetSenderNymID().str();
    Lock global(lock_);
    const auto existing = get_workflow(
        global, {proto::PAYMENTWORKFLOWTYPE_OUTGOINGCHEQUE}, nymID, cheque);

    if (existing) {
        LogOutput(OT_METHOD)(__FUNCTION__)(
            ": Workflow for this cheque already exist.")
            .Flush();

        return Identifier::Factory(existing->id());
    }

    if (cheque.HasRecipient()) {
        const auto& recipient = cheque.GetRecipientNymID();
        const auto contactID = contact_.ContactID(recipient);

        if (contactID->empty()) {
            LogOutput(OT_METHOD)(__FUNCTION__)(
                ": No contact exists for recipient nym ")(recipient)
                .Flush();

            return Identifier::Factory();
        }
    }

    const std::string party =
        cheque.HasRecipient() ? cheque.GetRecipientNymID().str() : "";
    const auto [workflowID, workflow] = create_cheque(
        global,
        nymID,
        cheque,
        proto::PAYMENTWORKFLOWTYPE_OUTGOINGCHEQUE,
        proto::PAYMENTWORKFLOWSTATE_UNSENT,
        versions_.at(proto::PAYMENTWORKFLOWTYPE_OUTGOINGCHEQUE).workflow_,
        versions_.at(proto::PAYMENTWORKFLOWTYPE_OUTGOINGCHEQUE).source_,
        versions_.at(proto::PAYMENTWORKFLOWTYPE_OUTGOINGCHEQUE).event_,
        party,
        cheque.GetSenderAcctID());
    global.unlock();
    const bool haveWorkflow = (false == workflowID->empty());
    const auto time{Clock::from_time_t(workflow.event(0).time())};

    if (haveWorkflow && cheque.HasRecipient()) {
        update_activity(
            cheque.GetSenderNymID(),
            cheque.GetRecipientNymID(),
            Identifier::Factory(cheque),
            workflowID,
            StorageBox::OUTGOINGCHEQUE,
            time);
    }

    if (false == workflowID->empty()) {
        update_rpc(
            nymID,
            cheque.GetRecipientNymID().str(),
            cheque.SourceAccountID().str(),
            proto::ACCOUNTEVENT_OUTGOINGCHEQUE,
            workflowID->str(),
            api_.Factory().Amount(),
            cheque.GetAmount() * api_.Factory().Amount(-1),
            time,
            cheque.GetMemo().Get());
    }

    return workflowID;
}
}  // namespace implementation
}  // namespace opentxs::api::client
