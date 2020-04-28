// Copyright (c) 2010-2020 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include "0_stdafx.hpp"                          // IWYU pragma: associated
#include "1_Internal.hpp"                        // IWYU pragma: associated
#include "ui/activitythread/ActivityThread.hpp"  // IWYU pragma: associated

#include <chrono>
#include <memory>
#include <ostream>
#include <set>
#include <thread>
#include <tuple>
#include <vector>

#include "internal/api/client/Client.hpp"
#include "opentxs/Forward.hpp"
#include "opentxs/Pimpl.hpp"
#include "opentxs/Proto.hpp"
#include "opentxs/Types.hpp"
#include "opentxs/Version.hpp"
#include "opentxs/api/Factory.hpp"
#include "opentxs/api/Wallet.hpp"
#include "opentxs/api/client/Activity.hpp"
#include "opentxs/api/client/Contacts.hpp"
#include "opentxs/api/storage/Storage.hpp"
#include "opentxs/blockchain/BlockchainType.hpp"
#include "opentxs/contact/Contact.hpp"
#include "opentxs/core/Data.hpp"
#include "opentxs/core/Identifier.hpp"
#include "opentxs/core/Log.hpp"
#include "opentxs/core/LogSource.hpp"
#include "opentxs/core/contract/UnitDefinition.hpp"
#include "opentxs/core/identifier/Nym.hpp"
#include "opentxs/core/identifier/UnitDefinition.hpp"
#include "opentxs/network/zeromq/Frame.hpp"
#include "opentxs/network/zeromq/FrameSection.hpp"
#include "opentxs/protobuf/ContactEnums.pb.h"
#include "opentxs/protobuf/StorageThread.pb.h"
#include "opentxs/protobuf/StorageThreadItem.pb.h"
#include "ui/base/List.hpp"

template class std::
    tuple<opentxs::OTIdentifier, opentxs::StorageBox, opentxs::OTIdentifier>;

#define OT_METHOD "opentxs::ui::implementation::ActivityThread::"

namespace zmq = opentxs::network::zeromq;

namespace opentxs::factory
{
auto ActivityThreadModel(
    const api::client::internal::Manager& api,
    const identifier::Nym& nymID,
    const Identifier& threadID,
    const SimpleCallback& cb) noexcept
    -> std::unique_ptr<ui::implementation::ActivityThread>
{
    using ReturnType = ui::implementation::ActivityThread;

    return std::make_unique<ReturnType>(api, nymID, threadID, cb);
}

#if OT_QT
auto ActivityThreadQtModel(ui::implementation::ActivityThread& parent) noexcept
    -> std::unique_ptr<ui::ActivityThreadQt>
{
    using ReturnType = ui::ActivityThreadQt;

    return std::make_unique<ReturnType>(parent);
}
#endif  // OT_QT
}  // namespace opentxs::factory

#if OT_QT
namespace opentxs::ui
{
QT_PROXY_MODEL_WRAPPER(ActivityThreadQt, implementation::ActivityThread)

QString ActivityThreadQt::displayName() const noexcept
{
    return parent_.DisplayName().c_str();
}
QString ActivityThreadQt::getDraft() const noexcept
{
    return parent_.GetDraft().c_str();
}
QString ActivityThreadQt::participants() const noexcept
{
    return parent_.Participants().c_str();
}
bool ActivityThreadQt::pay(
    const QString& amount,
    const QString& sourceAccount,
    const QString& memo) const noexcept
{
    return parent_.Pay(
        amount.toStdString(),
        Identifier::Factory(sourceAccount.toStdString()),
        memo.toStdString(),
        PaymentType::Cheque);
}
QString ActivityThreadQt::paymentCode(const int currency) const noexcept
{
    return parent_.PaymentCode(static_cast<proto::ContactItemType>(currency))
        .c_str();
}
bool ActivityThreadQt::sendDraft() const noexcept
{
    return parent_.SendDraft();
}
bool ActivityThreadQt::setDraft(const QString& draft) const noexcept
{
    return parent_.SetDraft(draft.toStdString());
}
QString ActivityThreadQt::threadID() const noexcept
{
    return parent_.ThreadID().c_str();
}
}  // namespace opentxs::ui
#endif

namespace opentxs::ui::implementation
{
ActivityThread::ActivityThread(
    const api::client::internal::Manager& api,
    const identifier::Nym& nymID,
    const Identifier& threadID,
    const SimpleCallback& cb) noexcept
    : ActivityThreadList(
        api,
        nymID,
        cb,
        false
#if OT_QT
        ,
        Roles{{ActivityThreadQt::PolarityRole, "polarity"},
              {ActivityThreadQt::TypeRole, "type"},
        },
        6
#endif
    )
    , StateMachine(std::bind(&ActivityThread::process_drafts, this))
    , listeners_{{api_.Activity().ThreadPublisher(nymID),
        new MessageProcessor<ActivityThread>(&ActivityThread::process_thread)},}
    , threadID_(Identifier::Factory(threadID))
    , participants_()
    , participants_promise_()
    , participants_future_(participants_promise_.get_future())
    , contact_lock_()
    , draft_()
    , draft_tasks_()
    , contact_(nullptr)
    , contact_thread_(nullptr)
{
    init();
    setup_listeners(listeners_);
    startup_.reset(new std::thread(&ActivityThread::startup, this));

    OT_ASSERT(startup_)

    contact_thread_.reset(new std::thread(&ActivityThread::init_contact, this));

    OT_ASSERT(contact_thread_)
}

void ActivityThread::can_message() const noexcept
{
    participants_future_.get();

    for (const auto& id : participants_) {
        api_.OTX().CanMessage(primary_id_, id, true);
    }
}

auto ActivityThread::comma(const std::set<std::string>& list) const noexcept
    -> std::string
{
    std::ostringstream stream;

    for (const auto& item : list) {
        stream << item;
        stream << ", ";
    }

    std::string output = stream.str();

    if (0 < output.size()) { output.erase(output.size() - 2, 2); }

    return output;
}

auto ActivityThread::construct_row(
    const ActivityThreadRowID& id,
    const ActivityThreadSortKey& index,
    CustomData& custom) const noexcept -> RowPointer
{
    const auto& box = std::get<1>(id);

    switch (box) {
        case StorageBox::MAILINBOX:
        case StorageBox::MAILOUTBOX: {
            return factory::MailItem(
                *this, api_, primary_id_, id, index, custom);
        }
        case StorageBox::DRAFT: {
            return factory::MailItem(
                *this,
                api_,

                primary_id_,
                id,
                index,
                custom,
                false,
                true);
        }
        case StorageBox::INCOMINGCHEQUE:
        case StorageBox::OUTGOINGCHEQUE: {
            return factory::PaymentItem(
                *this, api_, primary_id_, id, index, custom);
        }
        case StorageBox::PENDING_SEND: {
            return factory::PendingSend(
                *this, api_, primary_id_, id, index, custom);
        }
#if OT_BLOCKCHAIN
        case StorageBox::BLOCKCHAIN: {
            return factory::BlockchainActivityThreadItem(
                *this, api_, primary_id_, id, index, custom);
        }
#endif  // OT_BLOCKCHAIN
        case StorageBox::SENTPEERREQUEST:
        case StorageBox::INCOMINGPEERREQUEST:
        case StorageBox::SENTPEERREPLY:
        case StorageBox::INCOMINGPEERREPLY:
        case StorageBox::FINISHEDPEERREQUEST:
        case StorageBox::FINISHEDPEERREPLY:
        case StorageBox::PROCESSEDPEERREQUEST:
        case StorageBox::PROCESSEDPEERREPLY:
        case StorageBox::UNKNOWN:
        default: {
            OT_FAIL
        }
    }
}

auto ActivityThread::DisplayName() const noexcept -> std::string
{
    participants_future_.get();
    std::set<std::string> names{};

    for (const auto& contactID : participants_) {
        auto name = api_.Contacts().ContactName(contactID);

        if (name.empty()) {
            names.emplace(contactID->str());
        } else {
            names.emplace(name);
        }
    }

    return comma(names);
}

auto ActivityThread::GetDraft() const noexcept -> std::string
{
    Lock lock(decision_lock_);

    return draft_;
}

void ActivityThread::init_contact() noexcept
{
    participants_future_.get();

    if (1 != participants_.size()) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Wrong number of participants (")(
            participants_.size())(").")
            .Flush();

        return;
    }

    auto contact = api_.Contacts().Contact(*participants_.cbegin());
    Lock lock(contact_lock_);
    contact_ = contact;
    lock.unlock();
    UpdateNotify();
}

void ActivityThread::load_thread(const proto::StorageThread& thread) noexcept
{
    for (const auto& id : thread.participant()) {
        participants_.emplace(Identifier::Factory(id));
    }

    participants_promise_.set_value();
    LogDetail(OT_METHOD)(__FUNCTION__)(": Loading ")(thread.item().size())(
        " items.")
        .Flush();

    for (const auto& item : thread.item()) { process_item(item); }

    finish_startup();
}

void ActivityThread::new_thread() noexcept
{
    participants_.emplace(threadID_);
    participants_promise_.set_value();
    UpdateNotify();
    finish_startup();
}

auto ActivityThread::Participants() const noexcept -> std::string
{
    participants_future_.get();
    std::set<std::string> ids{};

    for (const auto& id : participants_) { ids.emplace(id->str()); }

    return comma(ids);
}

auto ActivityThread::Pay(
    const std::string& amount,
    const Identifier& sourceAccount,
    const std::string& memo,
    const PaymentType type) const noexcept -> bool
{
    const auto& unitID = api_.Storage().AccountContract(sourceAccount);

    if (unitID->empty()) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Invalid account: (")(
            sourceAccount)(")")
            .Flush();

        return false;
    }

    try {
        const auto contract = api_.Wallet().UnitDefinition(unitID);
        auto value = api_.Factory().Amount();
        const auto converted =
            contract->StringToAmountLocale(value, amount, "", "");

        if (false == converted) {
            LogOutput(OT_METHOD)(__FUNCTION__)(": Error parsing amount (")(
                amount)(")")
                .Flush();

            return false;
        }

        return Pay(value, sourceAccount, memo, type);
    } catch (...) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Missing unit definition (")(
            unitID)(")")
            .Flush();

        return false;
    }
}

auto ActivityThread::Pay(
    const Amount& amount,
    const Identifier& sourceAccount,
    const std::string& memo,
    const PaymentType type) const noexcept -> bool
{
    if (amount < 0) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Invalid amount: (")(amount.str())(")")
            .Flush();

        return false;
    }

    switch (type) {
        case PaymentType::Cheque: {
            return send_cheque(amount, sourceAccount, memo);
        }
        default: {
            LogOutput(OT_METHOD)(__FUNCTION__)(": Unsupported payment type: (")(
                static_cast<int>(type))(")")
                .Flush();

            return false;
        }
    }
}

auto ActivityThread::PaymentCode(
    const proto::ContactItemType currency) const noexcept -> std::string
{
    Lock lock(contact_lock_);

    if (contact_) { return contact_->PaymentCode(currency); }

    return {};
}

auto ActivityThread::process_drafts() noexcept -> bool
{
    Lock draftLock(decision_lock_);
    LogVerbose(OT_METHOD)(__FUNCTION__)(": Checking ")(draft_tasks_.size())(
        " pending sends.")
        .Flush();
    std::vector<ActivityThreadRowID> deleted{};

    for (auto i = draft_tasks_.begin(); i != draft_tasks_.end();) {
        auto& [rowID, backgroundTask] = *i;
        auto& future = std::get<1>(backgroundTask);
        const auto status = future.wait_for(std::chrono::microseconds(1));

        if (std::future_status::ready == status) {
            // TODO maybe keep failed sends
            deleted.emplace_back(rowID);
            i = draft_tasks_.erase(i);
        } else {
            ++i;
        }
    }

    rLock widgetLock(recursive_lock_);

    for (const auto& id : deleted) { delete_item(widgetLock, id); }

    if (0 < deleted.size()) { UpdateNotify(); }

    if (0 < draft_tasks_.size()) { return true; }

    return false;
}

auto ActivityThread::process_item(const proto::StorageThreadItem& item) noexcept
    -> ActivityThreadRowID
{
    const auto id = ActivityThreadRowID{
        api_.Factory().Identifier(item.id()),
        static_cast<StorageBox>(item.box()),
        api_.Factory().Identifier(item.account())};
    auto& [itemID, box, account] = id;
    const auto key =
        ActivityThreadSortKey{std::chrono::seconds(item.time()), item.index()};
    auto custom = CustomData{new std::string};

    switch (box) {
        case StorageBox::BLOCKCHAIN: {
            auto txid = api_.Factory().Data(item.txid(), StringStyle::Raw);
            const auto chain = static_cast<blockchain::Type>(item.chain());
            custom.emplace_back(new blockchain::Type{chain});
            custom.emplace_back(new OTData{std::move(txid)});
        } break;
        default: {
        }
    }

    add_item(id, key, custom);

    return id;
}

void ActivityThread::process_thread(const Message& message) noexcept
{
    wait_for_startup();

    const auto body = message.Body();

    OT_ASSERT(1 < body.size());

    const auto threadID = [&] {
        auto output = api_.Factory().Identifier();
        output->Assign(body.at(1).Bytes());

        return output;
    }();

    OT_ASSERT(false == threadID->empty())

    if (threadID_ != threadID) { return; }

    const auto thread = api_.Activity().Thread(primary_id_, threadID_);

    OT_ASSERT(thread)

    std::set<ActivityThreadRowID> active{};

    for (const auto& item : thread->item()) {
        const auto itemID = process_item(item);
        active.emplace(itemID);
    }

    Lock draftLock(decision_lock_);

    for (const auto& [id, task] : draft_tasks_) { active.emplace(id); }

    delete_inactive(active);
}

auto ActivityThread::send_cheque(
    const Amount& amount,
    const Identifier& sourceAccount,
    const std::string& memo) const noexcept -> bool
{
    participants_future_.get();

    if (false == validate_account(sourceAccount)) { return false; }

    if (participants_.empty()) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": No recipients.").Flush();

        return false;
    }

    if (1 < participants_.size()) {
        LogOutput(OT_METHOD)(__FUNCTION__)(
            ": Sending to multiple recipient not yet supported.")
            .Flush();

        return false;
    }

    auto displayAmount = std::string{};

    try {
        const auto contract = api_.Wallet().UnitDefinition(
            api_.Storage().AccountContract(sourceAccount));
        contract->FormatAmountLocale(amount, displayAmount, ",", ".");
    } catch (...) {
        LogOutput(OT_METHOD)(__FUNCTION__)(
            ": Failed to load unit definition contract")
            .Flush();

        return false;
    }

    auto task = make_blank<DraftTask>::value(api_);
    auto& [id, otx] = task;
    otx = api_.OTX().SendCheque(
        primary_id_, sourceAccount, *participants_.begin(), amount, memo);
    const auto taskID = std::get<0>(otx);

    if (0 == taskID) {
        LogOutput(OT_METHOD)(__FUNCTION__)(
            ": Failed to queue payment for sending.")
            .Flush();

        return false;
    }

    Lock draftLock(decision_lock_);
    id = ActivityThreadRowID{
        Identifier::Random(), StorageBox::PENDING_SEND, Identifier::Factory()};
    const ActivityThreadSortKey key{Clock::now(), 0};
    CustomData custom{
        new std::string{"Sending cheque"},
        new Amount{amount},
        new std::string{displayAmount},
        new std::string{memo}};
    const_cast<ActivityThread&>(*this).add_item(id, key, custom);
    draft_tasks_.emplace_back(std::move(task));
    trigger(draftLock);

    return true;
}

auto ActivityThread::SendDraft() const noexcept -> bool
{
    Lock draftLock(decision_lock_);

    if (draft_.empty()) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": No draft message to send.")
            .Flush();

        return false;
    }

    if (participants_.empty()) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": No recipients.").Flush();

        return false;
    }

    if (1 < participants_.size()) {
        LogOutput(OT_METHOD)(__FUNCTION__)(
            ": Sending to multiple recipient not yet supported.")
            .Flush();

        return false;
    }

    auto task = make_blank<DraftTask>::value(api_);
    auto& [id, otx] = task;
    otx =
        api_.OTX().MessageContact(primary_id_, *participants_.begin(), draft_);
    const auto taskID = std::get<0>(otx);

    if (0 == taskID) {
        LogOutput(OT_METHOD)(__FUNCTION__)(
            ": Failed to queue message for sending.")
            .Flush();

        return false;
    }

    id = ActivityThreadRowID{
        Identifier::Random(), StorageBox::DRAFT, Identifier::Factory()};
    const ActivityThreadSortKey key{Clock::now(), 0};
    CustomData custom{new std::string(draft_)};
    const_cast<ActivityThread&>(*this).add_item(id, key, custom);
    draft_tasks_.emplace_back(std::move(task));
    draft_.clear();
    trigger(draftLock);

    return true;
}

auto ActivityThread::SetDraft(const std::string& draft) const noexcept -> bool
{
    can_message();

    if (draft.empty()) { return false; }

    Lock lock(decision_lock_);
    draft_ = draft;

    return true;
}

void ActivityThread::startup() noexcept
{
    const auto thread = api_.Activity().Thread(primary_id_, threadID_);

    if (thread) {
        load_thread(*thread);
    } else {
        new_thread();
    }
}

auto ActivityThread::ThreadID() const noexcept -> std::string
{
    rLock lock{recursive_lock_};

    return threadID_->str();
}

auto ActivityThread::validate_account(
    const Identifier& sourceAccount) const noexcept -> bool
{
    const auto owner = api_.Storage().AccountOwner(sourceAccount);

    if (owner->empty()) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Invalid account id: (")(
            sourceAccount)(")")
            .Flush();

        return false;
    }

    if (primary_id_ != owner) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Account ")(sourceAccount)(
            " is not owned by nym ")(primary_id_)
            .Flush();

        return false;
    }

    return true;
}

ActivityThread::~ActivityThread()
{
    Stop();

    for (auto& it : listeners_) { delete it.second; }

    if (contact_thread_ && contact_thread_->joinable()) {
        contact_thread_->join();
        contact_thread_.reset();
    }
}
}  // namespace opentxs::ui::implementation
