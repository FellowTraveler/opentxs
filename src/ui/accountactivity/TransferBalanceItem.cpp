// Copyright (c) 2010-2020 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include "0_stdafx.hpp"    // IWYU pragma: associated
#include "1_Internal.hpp"  // IWYU pragma: associated
#include "ui/accountactivity/TransferBalanceItem.hpp"  // IWYU pragma: associated

#include <cstdint>
#include <memory>
#include <string>

#include "internal/api/client/Client.hpp"
#include "internal/ui/UI.hpp"
#include "opentxs/Pimpl.hpp"
#include "opentxs/api/client/Workflow.hpp"
#include "opentxs/core/Identifier.hpp"
#include "opentxs/core/Log.hpp"
#include "opentxs/core/LogSource.hpp"
#include "opentxs/core/String.hpp"
#include "opentxs/core/identifier/Nym.hpp"
#include "opentxs/core/identifier/Server.hpp"
#include "opentxs/protobuf/PaymentEvent.pb.h"
#include "opentxs/protobuf/PaymentWorkflow.pb.h"
#include "opentxs/protobuf/PaymentWorkflowEnums.pb.h"
#include "ui/accountactivity/BalanceItem.hpp"
#include "ui/base/Widget.hpp"

#define OT_METHOD "opentxs::ui::implementation::TransferBalanceItem::"

namespace opentxs::ui::implementation
{
TransferBalanceItem::TransferBalanceItem(
    const AccountActivityInternalInterface& parent,
    const api::client::internal::Manager& api,
    const AccountActivityRowID& rowID,
    const AccountActivitySortKey& sortKey,
    CustomData& custom,
    const identifier::Nym& nymID,
    const Identifier& accountID) noexcept
    : BalanceItem(parent, api, rowID, sortKey, custom, nymID, accountID)
    , transfer_()
{
    startup(
        extract_custom<proto::PaymentWorkflow>(custom, 0),
        extract_custom<proto::PaymentEvent>(custom, 1));
}

auto TransferBalanceItem::effective_amount() const noexcept -> OTAmount
{
    sLock lock(shared_lock_);
    auto amount = api_.Factory().Amount();
    auto sign = api_.Factory().Amount();

    if (transfer_) { amount = transfer_->GetAmount(); }

    switch (type_) {
        case StorageBox::OUTGOINGTRANSFER: {
            sign = api_.Factory().Amount(-1);
        } break;
        case StorageBox::INCOMINGTRANSFER: {
            sign = api_.Factory().Amount(1);
        } break;
        case StorageBox::INTERNALTRANSFER: {
            const auto in =
                parent_.AccountID() == transfer_->GetDestinationAcctID().str();

            if (in) {
                sign = api_.Factory().Amount(1);
            } else {
                sign = api_.Factory().Amount(-1);
            }
        } break;
        case StorageBox::SENTPEERREQUEST:
        case StorageBox::INCOMINGPEERREQUEST:
        case StorageBox::SENTPEERREPLY:
        case StorageBox::INCOMINGPEERREPLY:
        case StorageBox::FINISHEDPEERREQUEST:
        case StorageBox::FINISHEDPEERREPLY:
        case StorageBox::PROCESSEDPEERREQUEST:
        case StorageBox::PROCESSEDPEERREPLY:
        case StorageBox::MAILINBOX:
        case StorageBox::MAILOUTBOX:
        case StorageBox::BLOCKCHAIN:
        case StorageBox::INCOMINGCHEQUE:
        case StorageBox::OUTGOINGCHEQUE:
        case StorageBox::DRAFT:
        case StorageBox::UNKNOWN:
        default: {
        }
    }

    return amount * sign;
}

auto TransferBalanceItem::Memo() const noexcept -> std::string
{
    sLock lock(shared_lock_);

    if (transfer_) {
        auto note = String::Factory();
        transfer_->GetNote(note);

        return note->Get();
    }

    return {};
}

auto TransferBalanceItem::reindex(
    const implementation::AccountActivitySortKey& key,
    implementation::CustomData& custom) noexcept -> bool
{
    auto output = BalanceItem::reindex(key, custom);
    output |= startup(
        extract_custom<proto::PaymentWorkflow>(custom, 0),
        extract_custom<proto::PaymentEvent>(custom, 1));

    return output;
}

auto TransferBalanceItem::startup(
    const proto::PaymentWorkflow workflow,
    const proto::PaymentEvent event) noexcept -> bool
{
    eLock lock(shared_lock_);

    if (false == bool(transfer_)) {
        transfer_ =
            api::client::Workflow::InstantiateTransfer(api_, workflow).second;
    }

    OT_ASSERT(transfer_)

    lock.unlock();
    auto text = std::string{};
    const auto number = std::to_string(transfer_->GetTransactionNum());

    switch (type_) {
        case StorageBox::OUTGOINGTRANSFER: {
            switch (event.type()) {
                case proto::PAYMENTEVENTTYPE_ACKNOWLEDGE: {
                    text = "Sent transfer #" + number + " to ";

                    if (0 < workflow.party_size()) {
                        text += get_contact_name(
                            identifier::Nym::Factory(workflow.party(0)));
                    } else {
                        text += "account " +
                                transfer_->GetDestinationAcctID().str();
                    }
                } break;
                case proto::PAYMENTEVENTTYPE_COMPLETE: {
                    text = "Transfer #" + number + " cleared.";
                } break;
                default: {
                    LogOutput(OT_METHOD)(__FUNCTION__)(
                        ": Invalid event state (")(event.type())(")")
                        .Flush();
                }
            }
        } break;
        case StorageBox::INCOMINGTRANSFER: {
            switch (event.type()) {
                case proto::PAYMENTEVENTTYPE_CONVEY: {
                    text = "Received transfer #" + number + " from ";

                    if (0 < workflow.party_size()) {
                        text += get_contact_name(
                            identifier::Nym::Factory(workflow.party(0)));
                    } else {
                        text += "account " +
                                transfer_->GetPurportedAccountID().str();
                    }
                } break;
                case proto::PAYMENTEVENTTYPE_COMPLETE: {
                    text = "Transfer #" + number + " cleared.";
                } break;
                default: {
                    LogOutput(OT_METHOD)(__FUNCTION__)(
                        ": Invalid event state (")(event.type())(")")
                        .Flush();
                }
            }
        } break;
        case StorageBox::INTERNALTRANSFER: {
            const auto in =
                parent_.AccountID() == transfer_->GetDestinationAcctID().str();

            switch (event.type()) {
                case proto::PAYMENTEVENTTYPE_ACKNOWLEDGE: {
                    if (in) {
                        text = "Received internal transfer #" + number +
                               " from account " +
                               transfer_->GetPurportedAccountID().str();
                    } else {
                        text = "Sent internal transfer #" + number +
                               " to account " +
                               transfer_->GetDestinationAcctID().str();
                    }
                } break;
                case proto::PAYMENTEVENTTYPE_COMPLETE: {
                    text = "Transfer #" + number + " cleared.";
                } break;
                default: {
                    LogOutput(OT_METHOD)(__FUNCTION__)(
                        ": Invalid event state (")(event.type())(")")
                        .Flush();
                }
            }
        } break;
        case StorageBox::SENTPEERREQUEST:
        case StorageBox::INCOMINGPEERREQUEST:
        case StorageBox::SENTPEERREPLY:
        case StorageBox::INCOMINGPEERREPLY:
        case StorageBox::FINISHEDPEERREQUEST:
        case StorageBox::FINISHEDPEERREPLY:
        case StorageBox::PROCESSEDPEERREQUEST:
        case StorageBox::PROCESSEDPEERREPLY:
        case StorageBox::MAILINBOX:
        case StorageBox::MAILOUTBOX:
        case StorageBox::BLOCKCHAIN:
        case StorageBox::INCOMINGCHEQUE:
        case StorageBox::OUTGOINGCHEQUE:
        case StorageBox::DRAFT:
        case StorageBox::UNKNOWN:
        default: {
            LogOutput(OT_METHOD)(__FUNCTION__)(": Invalid item type (")(
                static_cast<std::uint8_t>(type_))(")")
                .Flush();
        }
    }

    auto output{false};
    lock.lock();

    if (text_ != text) {
        text_ = text;
        output = true;
    }

    lock.unlock();

    return output;
}

auto TransferBalanceItem::UUID() const noexcept -> std::string
{
    if (transfer_) {

        return api::client::Workflow::UUID(
                   transfer_->GetPurportedNotaryID(),
                   transfer_->GetTransactionNum())
            ->str();
    }

    return {};
}
}  // namespace opentxs::ui::implementation
