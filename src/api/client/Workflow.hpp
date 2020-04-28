// Copyright (c) 2010-2020 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

// IWYU pragma: private
// IWYU pragma: friend ".*src/api/client/Workflow.cpp"

#pragma once

#include <chrono>
#include <map>
#include <memory>
#include <set>
#include <shared_mutex>
#include <string>
#include <utility>
#include <vector>

#include "opentxs/Proto.hpp"
#include "opentxs/Types.hpp"
#include "opentxs/Version.hpp"
#include "opentxs/api/client/Workflow.hpp"
#include "opentxs/core/Cheque.hpp"
#include "opentxs/core/Identifier.hpp"
#include "opentxs/core/Item.hpp"
#include "opentxs/core/Lockable.hpp"
#include "opentxs/core/Message.hpp"
#include "opentxs/core/OTTransaction.hpp"
#include "opentxs/network/zeromq/socket/Publish.hpp"
#include "opentxs/network/zeromq/socket/Push.hpp"
#include "opentxs/protobuf/PaymentWorkflowEnums.pb.h"
#include "opentxs/protobuf/RPCEnums.pb.h"

namespace opentxs
{
namespace api
{
namespace client
{
class Activity;
class Contacts;
}  // namespace client

namespace internal
{
struct Core;
}  // namespace internal
}  // namespace api

namespace blind
{
class Purse;
}  // namespace blind

namespace identifier
{
class Nym;
class Server;
}  // namespace identifier

namespace proto
{
class PaymentWorkflow;
}  // namespace proto
}  // namespace opentxs

namespace opentxs::api::client::implementation
{
class Workflow final : public opentxs::api::client::Workflow, Lockable
{
public:
    auto AbortTransfer(
        const identifier::Nym& nymID,
        const Item& transfer,
        const Message& reply) const -> bool final;
    auto AcceptTransfer(
        const identifier::Nym& nymID,
        const identifier::Server& notaryID,
        const OTTransaction& pending,
        const Message& reply) const -> bool final;
    auto AcknowledgeTransfer(
        const identifier::Nym& nymID,
        const Item& transfer,
        const Message& reply) const -> bool final;
#if OT_CASH
    auto AllocateCash(const identifier::Nym& id, const blind::Purse& purse)
        const -> OTIdentifier final;
#endif
    auto CancelCheque(
        const opentxs::Cheque& cheque,
        const Message& request,
        const Message* reply) const -> bool final;
    auto ClearCheque(
        const identifier::Nym& recipientNymID,
        const OTTransaction& receipt) const -> bool final;
    auto ClearTransfer(
        const identifier::Nym& nymID,
        const identifier::Server& notaryID,
        const OTTransaction& receipt) const -> bool final;
    auto CompleteTransfer(
        const identifier::Nym& nymID,
        const identifier::Server& notaryID,
        const OTTransaction& receipt,
        const Message& reply) const -> bool final;
    auto ConveyTransfer(
        const identifier::Nym& nymID,
        const identifier::Server& notaryID,
        const OTTransaction& pending) const -> OTIdentifier final;
    auto CreateTransfer(const Item& transfer, const Message& request) const
        -> OTIdentifier final;
    auto DepositCheque(
        const identifier::Nym& nymID,
        const Identifier& accountID,
        const opentxs::Cheque& cheque,
        const Message& request,
        const Message* reply) const -> bool final;
    auto ExpireCheque(
        const identifier::Nym& nymID,
        const opentxs::Cheque& cheque) const -> bool final;
    auto ExportCheque(const opentxs::Cheque& cheque) const -> bool final;
    auto FinishCheque(
        const opentxs::Cheque& cheque,
        const Message& request,
        const Message* reply) const -> bool final;
    auto ImportCheque(
        const identifier::Nym& nymID,
        const opentxs::Cheque& cheque) const -> OTIdentifier final;
    auto List(
        const identifier::Nym& nymID,
        const proto::PaymentWorkflowType type,
        const proto::PaymentWorkflowState state) const
        -> std::set<OTIdentifier> final;
    auto LoadCheque(const identifier::Nym& nymID, const Identifier& chequeID)
        const -> Cheque final;
    auto LoadChequeByWorkflow(
        const identifier::Nym& nymID,
        const Identifier& workflowID) const -> Cheque final;
    auto LoadTransfer(
        const identifier::Nym& nymID,
        const Identifier& transferID) const -> Transfer final;
    auto LoadTransferByWorkflow(
        const identifier::Nym& nymID,
        const Identifier& workflowID) const -> Transfer final;
    auto LoadWorkflow(
        const identifier::Nym& nymID,
        const Identifier& workflowID) const
        -> std::shared_ptr<proto::PaymentWorkflow> final;
#if OT_CASH
    auto ReceiveCash(
        const identifier::Nym& receiver,
        const blind::Purse& purse,
        const Message& message) const -> OTIdentifier final;
#endif
    auto ReceiveCheque(
        const identifier::Nym& nymID,
        const opentxs::Cheque& cheque,
        const Message& message) const -> OTIdentifier final;
#if OT_CASH
    auto SendCash(
        const identifier::Nym& sender,
        const identifier::Nym& recipient,
        const Identifier& workflowID,
        const Message& request,
        const Message* reply) const -> bool final;
#endif
    auto SendCheque(
        const opentxs::Cheque& cheque,
        const Message& request,
        const Message* reply) const -> bool final;
    auto WorkflowsByAccount(
        const identifier::Nym& nymID,
        const Identifier& accountID) const -> std::vector<OTIdentifier> final;
    auto WriteCheque(const opentxs::Cheque& cheque) const -> OTIdentifier final;

    Workflow(
        const api::internal::Core& api,
        const Activity& activity,
        const Contacts& contact);

    ~Workflow() final = default;

private:
    struct ProtobufVersions {
        VersionNumber event_;
        VersionNumber source_;
        VersionNumber workflow_;
    };

    using VersionMap = std::map<proto::PaymentWorkflowType, ProtobufVersions>;

    static const VersionMap versions_;

    const api::internal::Core& api_;
    const Activity& activity_;
    const Contacts& contact_;
    const OTZMQPublishSocket account_publisher_;
    const OTZMQPushSocket rpc_publisher_;
    mutable std::map<std::string, std::shared_mutex> workflow_locks_;

    static auto can_abort_transfer(const proto::PaymentWorkflow& workflow)
        -> bool;
    static auto can_accept_cheque(const proto::PaymentWorkflow& workflow)
        -> bool;
    static auto can_accept_transfer(const proto::PaymentWorkflow& workflow)
        -> bool;
    static auto can_acknowledge_transfer(const proto::PaymentWorkflow& workflow)
        -> bool;
    static auto can_cancel_cheque(const proto::PaymentWorkflow& workflow)
        -> bool;
    static auto can_clear_transfer(const proto::PaymentWorkflow& workflow)
        -> bool;
    static auto can_complete_transfer(const proto::PaymentWorkflow& workflow)
        -> bool;
#if OT_CASH
    static auto can_convey_cash(const proto::PaymentWorkflow& workflow) -> bool;
#endif
    static auto can_convey_cheque(const proto::PaymentWorkflow& workflow)
        -> bool;
    static auto can_convey_transfer(const proto::PaymentWorkflow& workflow)
        -> bool;
    static auto can_deposit_cheque(const proto::PaymentWorkflow& workflow)
        -> bool;
    static auto can_expire_cheque(
        const opentxs::Cheque& cheque,
        const proto::PaymentWorkflow& workflow) -> bool;
    static auto can_finish_cheque(const proto::PaymentWorkflow& workflow)
        -> bool;
    static auto cheque_deposit_success(const Message* message) -> bool;
    static auto extract_conveyed_time(const proto::PaymentWorkflow& workflow)
        -> Time;
    static auto isCheque(const opentxs::Cheque& cheque) -> bool;
    static auto isTransfer(const Item& item) -> bool;
    static auto validate_recipient(
        const identifier::Nym& nymID,
        const opentxs::Cheque& cheque) -> bool;

    auto add_cheque_event(
        const eLock& lock,
        const std::string& nymID,
        const std::string& eventNym,
        proto::PaymentWorkflow& workflow,
        const proto::PaymentWorkflowState newState,
        const proto::PaymentEventType newEventType,
        const VersionNumber version,
        const Message& request,
        const Message* reply,
        const Identifier& account) const -> bool;
    auto add_cheque_event(
        const eLock& lock,
        const std::string& nymID,
        const Identifier& accountID,
        proto::PaymentWorkflow& workflow,
        const proto::PaymentWorkflowState newState,
        const proto::PaymentEventType newEventType,
        const VersionNumber version,
        const identifier::Nym& recipientNymID,
        const OTTransaction& receipt,
        const Time time = Clock::now()) const -> bool;
    auto add_transfer_event(
        const eLock& lock,
        const std::string& nymID,
        const std::string& eventNym,
        proto::PaymentWorkflow& workflow,
        const proto::PaymentWorkflowState newState,
        const proto::PaymentEventType newEventType,
        const VersionNumber version,
        const Message& message,
        const Identifier& account,
        const bool success) const -> bool;
    auto add_transfer_event(
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
        const bool success) const -> bool;
    auto convey_incoming_transfer(
        const identifier::Nym& nymID,
        const identifier::Server& notaryID,
        const OTTransaction& pending,
        const std::string& senderNymID,
        const std::string& recipientNymID,
        const Item& transfer) const -> OTIdentifier;
    auto convey_internal_transfer(
        const identifier::Nym& nymID,
        const identifier::Server& notaryID,
        const OTTransaction& pending,
        const std::string& senderNymID,
        const Item& transfer) const -> OTIdentifier;
    auto create_cheque(
        const Lock& global,
        const std::string& nymID,
        const opentxs::Cheque& cheque,
        const proto::PaymentWorkflowType workflowType,
        const proto::PaymentWorkflowState workflowState,
        const VersionNumber workflowVersion,
        const VersionNumber sourceVersion,
        const VersionNumber eventVersion,
        const std::string& party,
        const Identifier& account,
        const Message* message = nullptr) const
        -> std::pair<OTIdentifier, proto::PaymentWorkflow>;
    auto create_transfer(
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
        -> std::pair<OTIdentifier, proto::PaymentWorkflow>;
    auto extract_transfer_from_pending(const OTTransaction& receipt) const
        -> std::unique_ptr<Item>;
    auto extract_transfer_from_receipt(
        const OTTransaction& receipt,
        Identifier& depositorNymID) const -> std::unique_ptr<Item>;
    template <typename T>
    auto get_workflow(
        const Lock& global,
        const std::set<proto::PaymentWorkflowType>& types,
        const std::string& nymID,
        const T& source) const -> std::shared_ptr<proto::PaymentWorkflow>;
    auto get_workflow_by_id(
        const std::set<proto::PaymentWorkflowType>& types,
        const std::string& nymID,
        const std::string& workflowID) const
        -> std::shared_ptr<proto::PaymentWorkflow>;
    auto get_workflow_by_id(
        const std::string& nymID,
        const std::string& workflowID) const
        -> std::shared_ptr<proto::PaymentWorkflow>;
    auto get_workflow_by_source(
        const std::set<proto::PaymentWorkflowType>& types,
        const std::string& nymID,
        const std::string& sourceID) const
        -> std::shared_ptr<proto::PaymentWorkflow>;
    // Unlocks global after successfully locking the workflow-specific mutex
    auto get_workflow_lock(Lock& global, const std::string& id) const -> eLock;
    auto isInternalTransfer(
        const Identifier& sourceAccount,
        const Identifier& destinationAccount) const -> bool;
    auto save_workflow(
        const std::string& nymID,
        const proto::PaymentWorkflow& workflow) const -> bool;
    auto save_workflow(
        const std::string& nymID,
        const Identifier& accountID,
        const proto::PaymentWorkflow& workflow) const -> bool;
    auto save_workflow(
        OTIdentifier&& workflowID,
        const std::string& nymID,
        const Identifier& accountID,
        const proto::PaymentWorkflow& workflow) const -> OTIdentifier;
    auto save_workflow(
        std::pair<OTIdentifier, proto::PaymentWorkflow>&& workflowID,
        const std::string& nymID,
        const Identifier& accountID,
        const proto::PaymentWorkflow& workflow) const
        -> std::pair<OTIdentifier, proto::PaymentWorkflow>;
    auto update_activity(
        const identifier::Nym& localNymID,
        const identifier::Nym& remoteNymID,
        const Identifier& sourceID,
        const Identifier& workflowID,
        const StorageBox type,
        Time time) const -> bool;
    void update_rpc(
        const std::string& localNymID,
        const std::string& remoteNymID,
        const std::string& accountID,
        const proto::AccountEventType type,
        const std::string& workflowID,
        const Amount& amount,
        const Amount& pending,
        const Time time,
        const std::string& memo) const;

    Workflow() = delete;
    Workflow(const Workflow&) = delete;
    Workflow(Workflow&&) = delete;
    auto operator=(const Workflow&) -> Workflow& = delete;
    auto operator=(Workflow&&) -> Workflow& = delete;
};
}  // namespace opentxs::api::client::implementation
