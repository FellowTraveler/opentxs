// Copyright (c) 2010-2020 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include "0_stdafx.hpp"                              // IWYU pragma: associated
#include "1_Internal.hpp"                            // IWYU pragma: associated
#include "ui/accountactivity/ChequeBalanceItem.hpp"  // IWYU pragma: associated

#include <cstdint>
#include <memory>
#include <string>

#include "internal/api/client/Client.hpp"
#include "opentxs/Pimpl.hpp"
#include "opentxs/api/client/Workflow.hpp"
#include "opentxs/core/Cheque.hpp"
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

#define OT_METHOD "opentxs::ui::implementation::ChequeBalanceItem::"

namespace opentxs::ui::implementation
{
ChequeBalanceItem::ChequeBalanceItem(
    const AccountActivityInternalInterface& parent,
    const api::client::internal::Manager& api,
    const AccountActivityRowID& rowID,
    const AccountActivitySortKey& sortKey,
    CustomData& custom,
    const identifier::Nym& nymID,
    const Identifier& accountID) noexcept
    : BalanceItem(parent, api, rowID, sortKey, custom, nymID, accountID)
    , cheque_(nullptr)
{
    startup(
        extract_custom<proto::PaymentWorkflow>(custom, 0),
        extract_custom<proto::PaymentEvent>(custom, 1));
}

auto ChequeBalanceItem::effective_amount() const noexcept -> OTAmount
{
    sLock lock(shared_lock_);
    auto amount = api_.Factory().Amount();
    auto sign = api_.Factory().Amount();

    if (cheque_) { amount = cheque_->GetAmount(); }

    switch (type_) {
        case StorageBox::OUTGOINGCHEQUE: {
            sign = api_.Factory().Amount(-1);
        } break;
        case StorageBox::INCOMINGCHEQUE: {
            sign = api_.Factory().Amount(1);
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
        case StorageBox::DRAFT:
        case StorageBox::UNKNOWN:
        default: {
        }
    }

    return amount * sign;
}

auto ChequeBalanceItem::Memo() const noexcept -> std::string
{
    sLock lock(shared_lock_);

    if (cheque_) { return cheque_->GetMemo().Get(); }

    return {};
}

auto ChequeBalanceItem::reindex(
    const implementation::AccountActivitySortKey& key,
    implementation::CustomData& custom) noexcept -> bool
{
    auto output = BalanceItem::reindex(key, custom);
    output |= startup(
        extract_custom<proto::PaymentWorkflow>(custom, 0),
        extract_custom<proto::PaymentEvent>(custom, 1));

    return output;
}

auto ChequeBalanceItem::startup(
    const proto::PaymentWorkflow workflow,
    const proto::PaymentEvent event) noexcept -> bool
{
    eLock lock(shared_lock_);

    if (false == bool(cheque_)) {
        cheque_ =
            api::client::Workflow::InstantiateCheque(api_, workflow).second;
    }

    OT_ASSERT(cheque_)

    lock.unlock();
    auto name = std::string{};
    auto text = std::string{};
    auto number = std::to_string(cheque_->GetTransactionNum());
    auto otherNymID = identifier::Nym::Factory();

    switch (type_) {
        case StorageBox::INCOMINGCHEQUE: {
            otherNymID->Assign(cheque_->GetSenderNymID());

            if (otherNymID->empty()) { otherNymID = nym_id_; }

            switch (event.type()) {
                case proto::PAYMENTEVENTTYPE_CONVEY: {
                    text = "Received cheque #" + number + " from " +
                           get_contact_name(otherNymID);
                } break;
                case proto::PAYMENTEVENTTYPE_ERROR:
                case proto::PAYMENTEVENTTYPE_CREATE:
                case proto::PAYMENTEVENTTYPE_ACCEPT:
                case proto::PAYMENTEVENTTYPE_CANCEL:
                case proto::PAYMENTEVENTTYPE_COMPLETE:
                default: {
                    LogOutput(OT_METHOD)(__FUNCTION__)(
                        ": Invalid event state (")(event.type())(")")
                        .Flush();
                }
            }
        } break;
        case StorageBox::OUTGOINGCHEQUE: {
            otherNymID->Assign(cheque_->GetRecipientNymID());

            switch (event.type()) {
                case proto::PAYMENTEVENTTYPE_CREATE: {
                    text = "Wrote cheque #" + number;

                    if (false == otherNymID->empty()) {
                        text += " for " + get_contact_name(otherNymID);
                    }
                } break;
                case proto::PAYMENTEVENTTYPE_ACCEPT: {
                    text = "Cheque #" + number + " cleared";
                } break;
                case proto::PAYMENTEVENTTYPE_ERROR:
                case proto::PAYMENTEVENTTYPE_CONVEY:
                case proto::PAYMENTEVENTTYPE_CANCEL:
                case proto::PAYMENTEVENTTYPE_COMPLETE:
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
        case StorageBox::OUTGOINGTRANSFER:
        case StorageBox::INCOMINGTRANSFER:
        case StorageBox::INTERNALTRANSFER:
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

auto ChequeBalanceItem::UUID() const noexcept -> std::string
{
    if (cheque_) {

        return api::client::Workflow::UUID(
                   cheque_->GetNotaryID(), cheque_->GetTransactionNum())
            ->str();
    }

    return {};
}
}  // namespace opentxs::ui::implementation
