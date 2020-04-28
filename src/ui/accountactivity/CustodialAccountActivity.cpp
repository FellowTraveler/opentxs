// Copyright (c) 2010-2020 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include "0_stdafx.hpp"    // IWYU pragma: associated
#include "1_Internal.hpp"  // IWYU pragma: associated
#include "ui/accountactivity/CustodialAccountActivity.hpp"  // IWYU pragma: associated

#include <atomic>
#include <chrono>
#include <future>
#include <memory>
#include <set>
#include <string>
#include <vector>

#include "internal/api/client/Client.hpp"
#include "opentxs/Pimpl.hpp"
#include "opentxs/Shared.hpp"
#include "opentxs/Types.hpp"
#include "opentxs/api/Endpoints.hpp"  // IWYU pragma: keep
#include "opentxs/api/Factory.hpp"
#include "opentxs/api/Wallet.hpp"
#include "opentxs/api/client/Workflow.hpp"
#include "opentxs/api/storage/Storage.hpp"
#include "opentxs/core/Account.hpp"
#include "opentxs/core/Flag.hpp"
#include "opentxs/core/Identifier.hpp"
#include "opentxs/core/Log.hpp"
#include "opentxs/core/LogSource.hpp"
#include "opentxs/core/contract/ServerContract.hpp"
#include "opentxs/core/contract/UnitDefinition.hpp"
#include "opentxs/core/identifier/Server.hpp"
#include "opentxs/core/identifier/UnitDefinition.hpp"
#include "opentxs/network/zeromq/Frame.hpp"
#include "opentxs/network/zeromq/FrameSection.hpp"
#include "opentxs/protobuf/ContactEnums.pb.h"
#include "opentxs/protobuf/PaymentEvent.pb.h"
#include "opentxs/protobuf/PaymentWorkflow.pb.h"
#include "opentxs/protobuf/PaymentWorkflowEnums.pb.h"
#include "ui/base/Widget.hpp"

#define OT_METHOD "opentxs::ui::implementation::CustodialAccountActivity::"

namespace opentxs::factory
{
auto AccountActivityModel(
    const api::client::internal::Manager& api,
    const identifier::Nym& nymID,
    const Identifier& accountID,
    const SimpleCallback& cb) noexcept
    -> std::unique_ptr<ui::implementation::AccountActivity>
{
    using ReturnType = ui::implementation::CustodialAccountActivity;

    return std::make_unique<ReturnType>(api, nymID, accountID, cb);
}
}  // namespace opentxs::factory

namespace opentxs::ui::implementation
{
CustodialAccountActivity::CustodialAccountActivity(
    const api::client::internal::Manager& api,
    const identifier::Nym& nymID,
    const Identifier& accountID,
    const SimpleCallback& cb) noexcept
    : AccountActivity(api, nymID, accountID, AccountType::Custodial, cb)
    , alias_()
{
    init({
        api.Endpoints().AccountUpdate(),
        api.Endpoints().ContactUpdate(),
        api.Endpoints().ServerUpdate(),
        api.Endpoints().UnitUpdate(),
        api.Endpoints().WorkflowAccountUpdate(),
    });

    // If an Account exists, then the unit definition and notary contracts must
    // exist already.
    OT_ASSERT(0 < Contract().Version());
    OT_ASSERT(0 < Notary().Version());
}

auto CustodialAccountActivity::ContractID() const noexcept -> std::string
{
    sLock lock(shared_lock_);

    return contract_->ID()->str();
}

auto CustodialAccountActivity::DisplayBalance() const noexcept -> std::string
{
    sLock lock(shared_lock_);

    const auto amount = balance_.load();
    std::string output{};
    const auto formatted =
        contract_->FormatAmountLocale(amount, output, ",", ".");
    if (formatted) { return output; }

    return std::to_string(amount);
}

auto CustodialAccountActivity::DisplayUnit() const noexcept -> std::string
{
    sLock lock(shared_lock_);

    return contract_->TLA();
}

auto CustodialAccountActivity::extract_event(
    const proto::PaymentEventType eventType,
    const proto::PaymentWorkflow& workflow) noexcept -> EventRow
{
    bool success{false};
    bool found{false};
    EventRow output{};
    auto& [time, event_p] = output;

    for (const auto& event : workflow.event()) {
        const auto eventTime = Clock::from_time_t(event.time());

        if (eventType != event.type()) { continue; }

        if (eventTime > time) {
            if (success) {
                if (event.success()) {
                    time = eventTime;
                    event_p = &event;
                    found = true;
                }
            } else {
                time = eventTime;
                event_p = &event;
                success = event.success();
                found = true;
            }
        } else {
            if (false == success) {
                if (event.success()) {
                    // This is a weird case. It probably shouldn't happen
                    time = eventTime;
                    event_p = &event;
                    success = true;
                    found = true;
                }
            }
        }
    }

    if (false == found) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Workflow ")(workflow.id())(
            ", type ")(workflow.type())(", state ")(workflow.state())(
            " does not contain an event of type ")(eventType)
            .Flush();

        OT_FAIL;
    }

    return output;
}

auto CustodialAccountActivity::extract_rows(
    const proto::PaymentWorkflow& workflow) noexcept -> std::vector<RowKey>
{
    auto output = std::vector<RowKey>{};

    switch (workflow.type()) {
        case proto::PAYMENTWORKFLOWTYPE_OUTGOINGCHEQUE: {
            switch (workflow.state()) {
                case proto::PAYMENTWORKFLOWSTATE_UNSENT:
                case proto::PAYMENTWORKFLOWSTATE_CONVEYED:
                case proto::PAYMENTWORKFLOWSTATE_EXPIRED: {
                    output.emplace_back(
                        proto::PAYMENTEVENTTYPE_CREATE,
                        extract_event(
                            proto::PAYMENTEVENTTYPE_CREATE, workflow));
                } break;
                case proto::PAYMENTWORKFLOWSTATE_CANCELLED: {
                    output.emplace_back(
                        proto::PAYMENTEVENTTYPE_CREATE,
                        extract_event(
                            proto::PAYMENTEVENTTYPE_CREATE, workflow));
                    output.emplace_back(
                        proto::PAYMENTEVENTTYPE_CANCEL,
                        extract_event(
                            proto::PAYMENTEVENTTYPE_CANCEL, workflow));
                } break;
                case proto::PAYMENTWORKFLOWSTATE_ACCEPTED:
                case proto::PAYMENTWORKFLOWSTATE_COMPLETED: {
                    output.emplace_back(
                        proto::PAYMENTEVENTTYPE_CREATE,
                        extract_event(
                            proto::PAYMENTEVENTTYPE_CREATE, workflow));
                    output.emplace_back(
                        proto::PAYMENTEVENTTYPE_ACCEPT,
                        extract_event(
                            proto::PAYMENTEVENTTYPE_ACCEPT, workflow));
                } break;
                case proto::PAYMENTWORKFLOWSTATE_ERROR:
                case proto::PAYMENTWORKFLOWSTATE_INITIATED:
                default: {
                    LogOutput(OT_METHOD)(__FUNCTION__)(
                        ": Invalid workflow state (")(workflow.state())(")")
                        .Flush();
                }
            }
        } break;
        case proto::PAYMENTWORKFLOWTYPE_INCOMINGCHEQUE: {
            switch (workflow.state()) {
                case proto::PAYMENTWORKFLOWSTATE_CONVEYED:
                case proto::PAYMENTWORKFLOWSTATE_EXPIRED:
                case proto::PAYMENTWORKFLOWSTATE_COMPLETED: {
                    output.emplace_back(
                        proto::PAYMENTEVENTTYPE_CONVEY,
                        extract_event(
                            proto::PAYMENTEVENTTYPE_CONVEY, workflow));
                } break;
                case proto::PAYMENTWORKFLOWSTATE_ERROR:
                case proto::PAYMENTWORKFLOWSTATE_UNSENT:
                case proto::PAYMENTWORKFLOWSTATE_CANCELLED:
                case proto::PAYMENTWORKFLOWSTATE_ACCEPTED:
                case proto::PAYMENTWORKFLOWSTATE_INITIATED:
                default: {
                    LogOutput(OT_METHOD)(__FUNCTION__)(
                        ": Invalid workflow state (")(workflow.state())(")")
                        .Flush();
                }
            }
        } break;
        case proto::PAYMENTWORKFLOWTYPE_OUTGOINGTRANSFER: {
            switch (workflow.state()) {
                case proto::PAYMENTWORKFLOWSTATE_ACKNOWLEDGED:
                case proto::PAYMENTWORKFLOWSTATE_ACCEPTED: {
                    output.emplace_back(
                        proto::PAYMENTEVENTTYPE_ACKNOWLEDGE,
                        extract_event(
                            proto::PAYMENTEVENTTYPE_ACKNOWLEDGE, workflow));
                } break;
                case proto::PAYMENTWORKFLOWSTATE_COMPLETED: {
                    output.emplace_back(
                        proto::PAYMENTEVENTTYPE_ACKNOWLEDGE,
                        extract_event(
                            proto::PAYMENTEVENTTYPE_ACKNOWLEDGE, workflow));
                    output.emplace_back(
                        proto::PAYMENTEVENTTYPE_COMPLETE,
                        extract_event(
                            proto::PAYMENTEVENTTYPE_COMPLETE, workflow));
                } break;
                case proto::PAYMENTWORKFLOWSTATE_INITIATED:
                case proto::PAYMENTWORKFLOWSTATE_ABORTED: {
                } break;
                case proto::PAYMENTWORKFLOWSTATE_ERROR:
                case proto::PAYMENTWORKFLOWSTATE_UNSENT:
                case proto::PAYMENTWORKFLOWSTATE_CONVEYED:
                case proto::PAYMENTWORKFLOWSTATE_CANCELLED:
                case proto::PAYMENTWORKFLOWSTATE_EXPIRED:
                default: {
                    LogOutput(OT_METHOD)(__FUNCTION__)(
                        ": Invalid workflow state (")(workflow.state())(")")
                        .Flush();
                }
            }
        } break;
        case proto::PAYMENTWORKFLOWTYPE_INCOMINGTRANSFER: {
            switch (workflow.state()) {
                case proto::PAYMENTWORKFLOWSTATE_CONVEYED: {
                    output.emplace_back(
                        proto::PAYMENTEVENTTYPE_CONVEY,
                        extract_event(
                            proto::PAYMENTEVENTTYPE_CONVEY, workflow));
                } break;
                case proto::PAYMENTWORKFLOWSTATE_COMPLETED: {
                    output.emplace_back(
                        proto::PAYMENTEVENTTYPE_CONVEY,
                        extract_event(
                            proto::PAYMENTEVENTTYPE_CONVEY, workflow));
                    output.emplace_back(
                        proto::PAYMENTEVENTTYPE_ACCEPT,
                        extract_event(
                            proto::PAYMENTEVENTTYPE_ACCEPT, workflow));
                } break;
                case proto::PAYMENTWORKFLOWSTATE_ERROR:
                case proto::PAYMENTWORKFLOWSTATE_UNSENT:
                case proto::PAYMENTWORKFLOWSTATE_CANCELLED:
                case proto::PAYMENTWORKFLOWSTATE_ACCEPTED:
                case proto::PAYMENTWORKFLOWSTATE_EXPIRED:
                case proto::PAYMENTWORKFLOWSTATE_INITIATED:
                case proto::PAYMENTWORKFLOWSTATE_ABORTED:
                case proto::PAYMENTWORKFLOWSTATE_ACKNOWLEDGED:
                default: {
                    LogOutput(OT_METHOD)(__FUNCTION__)(
                        ": Invalid workflow state (")(workflow.state())(")")
                        .Flush();
                }
            }
        } break;
        case proto::PAYMENTWORKFLOWTYPE_INTERNALTRANSFER: {
            switch (workflow.state()) {
                case proto::PAYMENTWORKFLOWSTATE_ACKNOWLEDGED:
                case proto::PAYMENTWORKFLOWSTATE_CONVEYED:
                case proto::PAYMENTWORKFLOWSTATE_ACCEPTED: {
                    output.emplace_back(
                        proto::PAYMENTEVENTTYPE_ACKNOWLEDGE,
                        extract_event(
                            proto::PAYMENTEVENTTYPE_ACKNOWLEDGE, workflow));
                } break;
                case proto::PAYMENTWORKFLOWSTATE_COMPLETED: {
                    output.emplace_back(
                        proto::PAYMENTEVENTTYPE_ACKNOWLEDGE,
                        extract_event(
                            proto::PAYMENTEVENTTYPE_ACKNOWLEDGE, workflow));
                    output.emplace_back(
                        proto::PAYMENTEVENTTYPE_COMPLETE,
                        extract_event(
                            proto::PAYMENTEVENTTYPE_COMPLETE, workflow));
                } break;
                case proto::PAYMENTWORKFLOWSTATE_INITIATED:
                case proto::PAYMENTWORKFLOWSTATE_ABORTED: {
                } break;
                case proto::PAYMENTWORKFLOWSTATE_ERROR:
                case proto::PAYMENTWORKFLOWSTATE_UNSENT:
                case proto::PAYMENTWORKFLOWSTATE_CANCELLED:
                case proto::PAYMENTWORKFLOWSTATE_EXPIRED:
                default: {
                    LogOutput(OT_METHOD)(__FUNCTION__)(
                        ": Invalid workflow state (")(workflow.state())(")")
                        .Flush();
                }
            }
        } break;
        case proto::PAYMENTWORKFLOWTYPE_ERROR:
        case proto::PAYMENTWORKFLOWTYPE_OUTGOINGINVOICE:
        case proto::PAYMENTWORKFLOWTYPE_INCOMINGINVOICE:
        default: {
            LogOutput(OT_METHOD)(__FUNCTION__)(": Unsupported workflow type (")(
                workflow.type())(")")
                .Flush();
        }
    }

    return output;
}

auto CustodialAccountActivity::Name() const noexcept -> std::string
{
    sLock lock(shared_lock_);

    return alias_;
}

auto CustodialAccountActivity::NotaryID() const noexcept -> std::string
{
    sLock lock(shared_lock_);

    return notary_->ID()->str();
}

auto CustodialAccountActivity::NotaryName() const noexcept -> std::string
{
    sLock lock(shared_lock_);

    return notary_->EffectiveName();
}

auto CustodialAccountActivity::pipeline(const Message& in) noexcept -> void
{
    if (false == running_.get()) { return; }

    const auto body = in.Body();

    if (1 > body.size()) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Invalid message").Flush();

        OT_FAIL;
    }

    const auto work = body.at(0).as<Work>();

    switch (work) {
        case Work::notary: {
            process_notary(in);
        } break;
        case Work::unit: {
            process_unit(in);
        } break;
        case Work::contact: {
            process_contact(in);
        } break;
        case Work::account: {
            process_balance(in);
        } break;
        case Work::workflow: {
            process_workflow(in);
        } break;
        case Work::init: {
            startup();
            finish_startup();
        } break;
        case Work::statemachine: {
            do_work();
        } break;
        case Work::shutdown: {
            running_->Off();
            shutdown(shutdown_promise_);
        } break;
        default: {
            LogOutput(OT_METHOD)(__FUNCTION__)(": Unhandled type").Flush();

            OT_FAIL;
        }
    }
}

auto CustodialAccountActivity::process_balance(const Message& message) noexcept
    -> void
{
    wait_for_startup();
    const auto body = message.Body();

    OT_ASSERT(2 < body.size())

    auto accountID = Widget::api_.Factory().Identifier();
    accountID->Assign(body.at(1).Bytes());

    if (account_id_ != accountID) { return; }

    const auto balance = body.at(2).as<Amount>();
    const auto oldBalance = balance_.exchange(balance);
    const auto balanceChanged = (oldBalance != balance);
    const auto alias = [&] {
        auto account = Widget::api_.Wallet().Account(account_id_);

        OT_ASSERT(account);

        return account.get().Alias();
    }();
    const auto aliasChanged = [&] {
        eLock lock(shared_lock_);

        if (alias != alias_) {
            alias_ = alias;

            return true;
        }

        return false;
    }();

    if (balanceChanged || aliasChanged) {
        // TODO Qt widgets need to know that balance or alias properties have
        // changed
        UpdateNotify();
    }
}

auto CustodialAccountActivity::process_contact(const Message& message) noexcept
    -> void
{
    wait_for_startup();
    // Contact names may have changed, therefore all row texts must be
    // recalculated
    startup();
}

auto CustodialAccountActivity::process_notary(const Message& message) noexcept
    -> void
{
    wait_for_startup();
    const auto oldName = NotaryName();
    const auto newName = [&] {
        {
            eLock lock{shared_lock_};
            notary_ = Widget::api_.Wallet().Server(
                Widget::api_.Storage().AccountServer(account_id_));
        }

        return NotaryName();
    }();

    if (oldName != newName) {
        // TODO Qt widgets need to know that notary name property has changed
        UpdateNotify();
    }
}

auto CustodialAccountActivity::process_workflow(
    const Identifier& workflowID,
    std::set<AccountActivityRowID>& active) noexcept -> void
{
    const auto workflow =
        Widget::api_.Workflow().LoadWorkflow(primary_id_, workflowID);

    OT_ASSERT(workflow)

    const auto rows = extract_rows(*workflow);

    for (const auto& [type, row] : rows) {
        const auto& [time, event_p] = row;
        auto key = AccountActivityRowID{Identifier::Factory(workflowID), type};
        auto custom = CustomData{
            new proto::PaymentWorkflow(*workflow),
            new proto::PaymentEvent(*event_p)};
        add_item(key, time, custom);
        active.emplace(std::move(key));
    }
}

auto CustodialAccountActivity::process_workflow(const Message& message) noexcept
    -> void
{
    wait_for_startup();
    const auto body = message.Body();

    OT_ASSERT(1 < body.size());

    const auto accountID = [&] {
        auto output = Widget::api_.Factory().Identifier();
        output->Assign(body.at(1).Bytes());

        return output;
    }();

    OT_ASSERT(false == accountID->empty())

    if (account_id_ == accountID) { startup(); }
}

auto CustodialAccountActivity::process_unit(const Message& message) noexcept
    -> void
{
    wait_for_startup();
    // TODO currently it doesn't matter if the unit definition alias changes
    // since we don't use it
    eLock lock{shared_lock_};
    contract_ = Widget::api_.Wallet().UnitDefinition(
        Widget::api_.Storage().AccountContract(account_id_));
}

auto CustodialAccountActivity::startup() noexcept -> void
{
    const auto alias = [&] {
        auto account = Widget::api_.Wallet().Account(account_id_);

        OT_ASSERT(account);

        return account.get().Alias();
    }();
    const auto aliasChanged = [&] {
        eLock lock(shared_lock_);

        if (alias != alias_) {
            alias_ = alias;

            return true;
        }

        return false;
    }();

    const auto workflows =
        Widget::api_.Workflow().WorkflowsByAccount(primary_id_, account_id_);
    auto active = std::set<AccountActivityRowID>{};

    for (const auto& id : workflows) { process_workflow(id, active); }

    delete_inactive(active);

    if (aliasChanged) {
        // TODO Qt widgets need to know the alias property has changed
        UpdateNotify();
    }
}

auto CustodialAccountActivity::Unit() const noexcept -> proto::ContactItemType
{
    sLock lock(shared_lock_);

    return contract_->UnitOfAccount();
}

CustodialAccountActivity::~CustodialAccountActivity()
{
    wait_for_startup();
    stop_worker().get();
}
}  // namespace opentxs::ui::implementation
