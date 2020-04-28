// Copyright (c) 2010-2020 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include "0_stdafx.hpp"             // IWYU pragma: associated
#include "1_Internal.hpp"           // IWYU pragma: associated
#include "api/client/Activity.hpp"  // IWYU pragma: associated

#include <algorithm>
#include <iterator>
#include <list>
#include <map>
#include <set>
#include <thread>
#include <type_traits>
#include <utility>
#include <vector>

#include "internal/api/Api.hpp"
#include "internal/api/client/Client.hpp"
#include "internal/api/client/Factory.hpp"
#include "opentxs/Pimpl.hpp"
#include "opentxs/Types.hpp"
#include "opentxs/api/Endpoints.hpp"
#include "opentxs/api/Factory.hpp"
#include "opentxs/api/Wallet.hpp"
#include "opentxs/api/client/Activity.hpp"
#include "opentxs/api/client/Contacts.hpp"
#include "opentxs/api/client/Workflow.hpp"
#include "opentxs/api/storage/Storage.hpp"
#if OT_BLOCKCHAIN
#include "opentxs/blockchain/block/bitcoin/Transaction.hpp"  // IWYU pragma: keep
#endif                                                       // OT_BLOCKCHAIN
#include "opentxs/contact/Contact.hpp"
#include "opentxs/core/Armored.hpp"
#include "opentxs/core/Cheque.hpp"
#include "opentxs/core/Data.hpp"
#include "opentxs/core/Item.hpp"
#include "opentxs/core/Log.hpp"
#include "opentxs/core/LogSource.hpp"
#include "opentxs/core/Message.hpp"
#include "opentxs/core/PasswordPrompt.hpp"
#include "opentxs/core/String.hpp"
#include "opentxs/core/contract/UnitDefinition.hpp"
#include "opentxs/core/contract/peer/PeerObject.hpp"
#include "opentxs/core/identifier/Nym.hpp"
#include "opentxs/core/identifier/UnitDefinition.hpp"
#include "opentxs/network/zeromq/Context.hpp"
#include "opentxs/network/zeromq/Message.hpp"
#include "opentxs/network/zeromq/socket/Publish.hpp"
#include "opentxs/protobuf/PaymentWorkflow.pb.h"
#include "opentxs/protobuf/PaymentWorkflowEnums.pb.h"
#include "opentxs/protobuf/StorageThread.pb.h"
#include "opentxs/protobuf/StorageThreadItem.pb.h"
#include "opentxs/util/WorkType.hpp"

#define OT_METHOD "opentxs::api::client::implementation::Activity::"

namespace opentxs::factory
{
auto Activity(
    const api::internal::Core& api,
    const api::client::Contacts& contact) noexcept
    -> std::unique_ptr<api::client::internal::Activity>
{
    using ReturnType = api::client::implementation::Activity;

    return std::make_unique<ReturnType>(api, contact);
}
}  // namespace opentxs::factory

namespace opentxs::api::client::implementation
{
Activity::Activity(
    const api::internal::Core& api,
    const client::Contacts& contact) noexcept
    : api_(api)
    , contact_(contact)
    , mail_cache_lock_()
    , mail_cache_()
    , publisher_lock_()
    , thread_publishers_()
#if OT_BLOCKCHAIN
    , blockchain_publishers_()
#endif  // OT_BLOCKCHAIN
{
    // WARNING: do not access api_.Wallet() during construction
}

void Activity::activity_preload_thread(
    OTPasswordPrompt reason,
    const OTIdentifier nym,
    const std::size_t count) const noexcept
{
    const std::string nymID = nym->str();
    auto threads = api_.Storage().ThreadList(nymID, false);

    for (const auto& it : threads) {
        const auto& threadID = it.first;
        thread_preload_thread(reason, nymID, threadID, 0, count);
    }
}

#if OT_BLOCKCHAIN
auto Activity::add_blockchain_transaction(
    const eLock& lock,
    const Blockchain& blockchain,
    const identifier::Nym& nym,
    const BlockchainTransaction& transaction) const noexcept -> bool
{
    const auto incoming =
        transaction.AssociatedRemoteContacts(blockchain, contact_, nym);
    const auto existing =
        api_.Storage().BlockchainThreadMap(nym, transaction.ID());
    auto added = std::vector<OTIdentifier>{};
    auto removed = std::vector<OTIdentifier>{};
    std::set_difference(
        std::begin(incoming),
        std::end(incoming),
        std::begin(existing),
        std::end(existing),
        std::back_inserter(added));
    std::set_difference(
        std::begin(existing),
        std::end(existing),
        std::begin(incoming),
        std::end(incoming),
        std::back_inserter(removed));
    auto output{true};
    const auto& txid = transaction.ID();
    const auto chains = transaction.Chains();

    for (const auto& thread : added) {
        if (thread->empty()) { continue; }

        const auto sThreadID = thread->str();

        if (verify_thread_exists(nym.str(), sThreadID)) {
            auto saved{true};
            std::for_each(
                std::begin(chains), std::end(chains), [&](const auto& chain) {
                    saved &= api_.Storage().Store(
                        nym, thread, chain, txid, transaction.Timestamp());
                });

            if (saved) { publish(nym, thread); }

            output &= saved;
        } else {
            output = false;
        }
    }

    for (const auto& thread : removed) {
        if (thread->empty()) { continue; }

        auto saved{true};
        const auto chains = transaction.Chains();
        std::for_each(
            std::begin(chains), std::end(chains), [&](const auto& chain) {
                saved &= api_.Storage().RemoveBlockchainThreadItem(
                    nym, thread, chain, txid);
            });

        if (saved) { publish(nym, thread); }

        output &= saved;
    }

    if (0 == incoming.size()) {
        api_.Storage().UnaffiliatedBlockchainTransaction(nym, txid);
    }

    std::for_each(std::begin(chains), std::end(chains), [&](const auto& chain) {
        auto out =
            api_.ZeroMQ().TaggedMessage(WorkType::BlockchainNewTransaction);
        out->AddFrame(txid);
        out->AddFrame(chain);
        get_blockchain(lock, nym).Send(out);
    });

    return output;
}

auto Activity::AddBlockchainTransaction(
    const Blockchain& api,
    const BlockchainTransaction& transaction) const noexcept -> bool
{
    eLock lock(shared_lock_);

    for (const auto& nym : transaction.AssociatedLocalNyms(api)) {
        if (false == add_blockchain_transaction(lock, api, nym, transaction)) {
            return false;
        }
    }

    return true;
}
#endif  // OT_BLOCKCHAIN

auto Activity::AddPaymentEvent(
    const identifier::Nym& nymID,
    const Identifier& threadID,
    const StorageBox type,
    const Identifier& itemID,
    const Identifier& workflowID,
    Time time) const noexcept -> bool
{
    eLock lock(shared_lock_);
    const std::string sNymID = nymID.str();
    const std::string sthreadID = threadID.str();

    if (false == verify_thread_exists(sNymID, sthreadID)) { return false; }

    const bool saved = api_.Storage().Store(
        sNymID,
        sthreadID,
        itemID.str(),
        Clock::to_time_t(time),
        {},
        {},
        type,
        workflowID.str());

    if (saved) { publish(nymID, threadID); }

    return saved;
}

auto Activity::Cheque(
    const identifier::Nym& nym,
    [[maybe_unused]] const std::string& id,
    const std::string& workflowID) const noexcept -> Activity::ChequeData
{
    auto output = ChequeData{nullptr, api_.Factory().UnitDefinition()};
    auto& [cheque, contract] = output;
    auto [type, state] =
        api_.Storage().PaymentWorkflowState(nym.str(), workflowID);
    [[maybe_unused]] const auto& notUsed = state;

    switch (type) {
        case proto::PAYMENTWORKFLOWTYPE_OUTGOINGCHEQUE:
        case proto::PAYMENTWORKFLOWTYPE_INCOMINGCHEQUE:
        case proto::PAYMENTWORKFLOWTYPE_OUTGOINGINVOICE:
        case proto::PAYMENTWORKFLOWTYPE_INCOMINGINVOICE: {
        } break;

        case proto::PAYMENTWORKFLOWTYPE_ERROR:
        case proto::PAYMENTWORKFLOWTYPE_OUTGOINGTRANSFER:
        case proto::PAYMENTWORKFLOWTYPE_INCOMINGTRANSFER:
        case proto::PAYMENTWORKFLOWTYPE_INTERNALTRANSFER:
        default: {
            LogOutput(OT_METHOD)(__FUNCTION__)(": Wrong workflow type.")
                .Flush();

            return output;
        }
    }

    std::shared_ptr<proto::PaymentWorkflow> workflow{nullptr};
    const auto loaded = api_.Storage().Load(nym.str(), workflowID, workflow);

    if (false == loaded) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Workflow ")(workflowID)(
            " for nym ")(nym)(" can not be loaded.")
            .Flush();

        return output;
    }

    OT_ASSERT(workflow)

    auto instantiated = client::Workflow::InstantiateCheque(api_, *workflow);
    cheque.reset(std::get<1>(instantiated).release());

    OT_ASSERT(cheque)

    const auto& unit = cheque->GetInstrumentDefinitionID();

    try {
        contract = api_.Wallet().UnitDefinition(unit);
    } catch (...) {
        LogOutput(OT_METHOD)(__FUNCTION__)(
            ": Unable to load unit definition contract.")
            .Flush();
    }

    return output;
}

auto Activity::Transfer(
    const identifier::Nym& nym,
    [[maybe_unused]] const std::string& id,
    const std::string& workflowID) const noexcept -> Activity::TransferData
{
    auto output = TransferData{nullptr, api_.Factory().UnitDefinition()};
    auto& [transfer, contract] = output;
    auto [type, state] =
        api_.Storage().PaymentWorkflowState(nym.str(), workflowID);

    switch (type) {
        case proto::PAYMENTWORKFLOWTYPE_OUTGOINGTRANSFER:
        case proto::PAYMENTWORKFLOWTYPE_INCOMINGTRANSFER:
        case proto::PAYMENTWORKFLOWTYPE_INTERNALTRANSFER: {
        } break;

        case proto::PAYMENTWORKFLOWTYPE_ERROR:
        case proto::PAYMENTWORKFLOWTYPE_OUTGOINGCHEQUE:
        case proto::PAYMENTWORKFLOWTYPE_INCOMINGCHEQUE:
        case proto::PAYMENTWORKFLOWTYPE_OUTGOINGINVOICE:
        case proto::PAYMENTWORKFLOWTYPE_INCOMINGINVOICE:
        default: {
            LogOutput(OT_METHOD)(__FUNCTION__)(": Wrong workflow type").Flush();

            return output;
        }
    }

    auto workflow = std::shared_ptr<proto::PaymentWorkflow>{nullptr};
    const auto loaded = api_.Storage().Load(nym.str(), workflowID, workflow);

    if (false == loaded) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Workflow ")(workflowID)(
            " for nym ")(nym)(" can not be loaded")
            .Flush();

        return output;
    }

    OT_ASSERT(workflow)

    auto instantiated = client::Workflow::InstantiateTransfer(api_, *workflow);
    transfer.reset(std::get<1>(instantiated).release());

    OT_ASSERT(transfer)

    if (0 == workflow->account_size()) {
        LogOutput(OT_METHOD)(__FUNCTION__)(
            ": Workflow does not list any accounts.")
            .Flush();

        return output;
    }

    const auto unit = api_.Storage().AccountContract(
        Identifier::Factory(workflow->account(0)));

    if (unit->empty()) {
        LogOutput(OT_METHOD)(__FUNCTION__)(
            ": Unable to calculate unit definition id.")
            .Flush();

        return output;
    }

    try {
        contract = api_.Wallet().UnitDefinition(unit);
    } catch (...) {
        LogOutput(OT_METHOD)(__FUNCTION__)(
            ": Unable to load unit definition contract.")
            .Flush();
    }

    return output;
}

auto Activity::get_publisher(const identifier::Nym& nymID) const noexcept
    -> const opentxs::network::zeromq::socket::Publish&
{
    std::string endpoint{};

    return get_publisher(nymID, endpoint);
}

#if OT_BLOCKCHAIN
auto Activity::get_blockchain(const eLock&, const identifier::Nym& nymID)
    const noexcept -> const opentxs::network::zeromq::socket::Publish&
{
    auto it = blockchain_publishers_.find(nymID);

    if (blockchain_publishers_.end() != it) { return it->second; }

    const auto endpoint = api_.Endpoints().BlockchainTransactions(nymID);
    const auto& [publisher, inserted] =
        blockchain_publishers_.emplace(nymID, start_publisher(endpoint));

    OT_ASSERT(inserted);

    return publisher->second.get();
}
#endif  // OT_BLOCKCHAIN

auto Activity::get_publisher(
    const identifier::Nym& nymID,
    std::string& endpoint) const noexcept
    -> const opentxs::network::zeromq::socket::Publish&
{
    endpoint = api_.Endpoints().ThreadUpdate(nymID.str());
    Lock lock(publisher_lock_);
    auto it = thread_publishers_.find(nymID);

    if (thread_publishers_.end() != it) { return it->second; }

    const auto& [publisher, inserted] =
        thread_publishers_.emplace(nymID, start_publisher(endpoint));

    OT_ASSERT(inserted);

    return publisher->second.get();
}

auto Activity::Mail(
    const identifier::Nym& nym,
    const Identifier& id,
    const StorageBox& box) const noexcept -> std::unique_ptr<Message>
{
    std::string raw, alias;
    const bool loaded =
        api_.Storage().Load(nym.str(), id.str(), box, raw, alias, true);

    std::unique_ptr<Message> output;

    if (false == loaded) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Failed to load message ")(id)
            .Flush();

        return output;
    }

    if (raw.empty()) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Empty message ")(id).Flush();

        return output;
    }

    output.reset(api_.Factory().Message().release());

    OT_ASSERT(output);

    if (false == output->LoadContractFromString(String::Factory(raw.c_str()))) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Failed to deserialize message ")(
            id)
            .Flush();

        output.reset();
    }

    return output;
}

auto Activity::Mail(
    const identifier::Nym& nym,
    const Message& mail,
    const StorageBox box,
    const PasswordPrompt& reason) const noexcept -> std::string
{
    const std::string nymID = nym.str();
    auto id = Identifier::Factory();
    mail.CalculateContractID(id);
    const std::string output = id->str();
    const auto data = String::Factory(mail);
    std::string participantNymID;
    const auto localName = String::Factory(nym);

    if (localName->Compare(mail.m_strNymID2)) {
        // This is an incoming message. The contact id is the sender's id.
        participantNymID = mail.m_strNymID->Get();
    } else {
        // This is an outgoing message. The contact id is the recipient's id.
        participantNymID = mail.m_strNymID2->Get();
    }

    const auto contact = nym_to_contact(participantNymID);

    OT_ASSERT(contact);

    eLock lock(shared_lock_);
    std::string alias = contact->Label();
    const auto& contactID = contact->ID();
    const auto& threadID = contactID.str();

    if (false == verify_thread_exists(nymID, threadID)) { return {}; }

    const bool saved = api_.Storage().Store(
        localName->Get(),
        threadID,
        output,
        mail.m_lTime,
        alias,
        data->Get(),
        box);

    if (saved) {
        std::thread preload(
            &Activity::preload,
            this,
            OTPasswordPrompt{reason},
            OTNymID{nym},
            OTIdentifier{id},
            box);
        preload.detach();
        publish(nym, contactID);

        return output;
    }

    return "";
}

auto Activity::Mail(const identifier::Nym& nym, const StorageBox box)
    const noexcept -> ObjectList
{
    return api_.Storage().NymBoxList(nym.str(), box);
}

auto Activity::MailRemove(
    const identifier::Nym& nym,
    const Identifier& id,
    const StorageBox box) const noexcept -> bool
{
    const std::string nymid = nym.str();
    const std::string mail = id.str();

    return api_.Storage().RemoveNymBoxItem(nymid, box, mail);
}

auto Activity::MailText(
    const identifier::Nym& nymID,
    const Identifier& id,
    const StorageBox& box,
    const PasswordPrompt& reason) const noexcept
    -> std::shared_ptr<const std::string>
{
    Lock lock(mail_cache_lock_);
    auto it = mail_cache_.find(id);
    lock.unlock();

    if (mail_cache_.end() != it) { return it->second; }

    preload(reason, nymID, id, box);
    lock.lock();
    it = mail_cache_.find(id);
    lock.unlock();

    if (mail_cache_.end() == it) { return {}; }

    return it->second;
}

auto Activity::MarkRead(
    const identifier::Nym& nymId,
    const Identifier& threadId,
    const Identifier& itemId) const noexcept -> bool
{
    const std::string nym = nymId.str();
    const std::string thread = threadId.str();
    const std::string item = itemId.str();

    return api_.Storage().SetReadState(nym, thread, item, false);
}

auto Activity::MarkUnread(
    const identifier::Nym& nymId,
    const Identifier& threadId,
    const Identifier& itemId) const noexcept -> bool
{
    const std::string nym = nymId.str();
    const std::string thread = threadId.str();
    const std::string item = itemId.str();

    return api_.Storage().SetReadState(nym, thread, item, true);
}

void Activity::MigrateLegacyThreads() const noexcept
{
    eLock lock(shared_lock_);
    std::set<std::string> contacts{};

    for (const auto& it : contact_.ContactList()) { contacts.insert(it.first); }

    const auto nymlist = api_.Storage().NymList();

    for (const auto& it1 : nymlist) {
        const auto& nymID = it1.first;
        const auto threadList = api_.Storage().ThreadList(nymID, false);

        for (const auto& it2 : threadList) {
            const auto& originalThreadID = it2.first;
            const bool isContactID = (1 == contacts.count(originalThreadID));

            if (isContactID) { continue; }

            auto contactID =
                contact_.ContactID(identifier::Nym::Factory(originalThreadID));

            if (false == contactID->empty()) {
                api_.Storage().RenameThread(
                    nymID, originalThreadID, contactID->str());
            } else {
                std::shared_ptr<proto::StorageThread> thread;
                api_.Storage().Load(nymID, originalThreadID, thread);

                OT_ASSERT(thread);

                const auto nymCount = thread->participant().size();

                if (1 == nymCount) {
                    auto paymentCode = api_.Factory().PaymentCode("");
                    auto newContact = contact_.NewContact(
                        "",
                        identifier::Nym::Factory(originalThreadID),
                        paymentCode);

                    OT_ASSERT(newContact);

                    api_.Storage().RenameThread(
                        nymID, originalThreadID, newContact->ID().str());
                } else {
                    // Multi-party chats were not implemented prior to the
                    // update to contact IDs, so there is no need to handle
                    // this case
                }
            }
        }
    }
}

auto Activity::nym_to_contact(const std::string& id) const noexcept
    -> std::shared_ptr<const Contact>
{
    const auto nymID = identifier::Nym::Factory(id);
    const auto contactID = contact_.NymToContact(nymID);

    return contact_.Contact(contactID);
}

auto Activity::PaymentText(
    const identifier::Nym& nym,
    const std::string& id,
    const std::string& workflowID) const noexcept
    -> std::shared_ptr<const std::string>
{
    std::shared_ptr<std::string> output;
    auto [type, state] =
        api_.Storage().PaymentWorkflowState(nym.str(), workflowID);
    [[maybe_unused]] const auto& notUsed = state;

    switch (type) {
        case proto::PAYMENTWORKFLOWTYPE_OUTGOINGCHEQUE: {
            output.reset(new std::string("Sent cheque"));
        } break;
        case proto::PAYMENTWORKFLOWTYPE_INCOMINGCHEQUE: {
            output.reset(new std::string("Received cheque"));
        } break;
        case proto::PAYMENTWORKFLOWTYPE_OUTGOINGTRANSFER: {
            output.reset(new std::string("Sent transfer"));
        } break;
        case proto::PAYMENTWORKFLOWTYPE_INCOMINGTRANSFER: {
            output.reset(new std::string("Received transfer"));
        } break;
        case proto::PAYMENTWORKFLOWTYPE_INTERNALTRANSFER: {
            output.reset(new std::string("Internal transfer"));
        } break;

        case proto::PAYMENTWORKFLOWTYPE_ERROR:
        case proto::PAYMENTWORKFLOWTYPE_OUTGOINGINVOICE:
        case proto::PAYMENTWORKFLOWTYPE_INCOMINGINVOICE:
        default: {

            return std::move(output);
        }
    }

    std::shared_ptr<proto::PaymentWorkflow> workflow{nullptr};
    const auto loaded = api_.Storage().Load(nym.str(), workflowID, workflow);

    if (false == loaded) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Workflow ")(workflowID)(
            " for nym ")(nym)(" can not be loaded.")
            .Flush();

        return std::move(output);
    }

    OT_ASSERT(workflow)

    switch (type) {
        case proto::PAYMENTWORKFLOWTYPE_OUTGOINGCHEQUE:
        case proto::PAYMENTWORKFLOWTYPE_INCOMINGCHEQUE:
        case proto::PAYMENTWORKFLOWTYPE_OUTGOINGINVOICE:
        case proto::PAYMENTWORKFLOWTYPE_INCOMINGINVOICE: {
            auto chequeData = Cheque(nym, id, workflowID);
            const auto& [cheque, contract] = chequeData;

            OT_ASSERT(cheque)

            if (0 < contract->Version()) {
                std::string amount{};
                const bool haveAmount = contract->FormatAmountLocale(
                    cheque->GetAmount(),
                    amount, ",", ".");

                if (haveAmount) {
                    const std::string text =
                        *output + std::string{" for "} + amount;
                    *output = text;
                }
            }
        } break;

        case proto::PAYMENTWORKFLOWTYPE_OUTGOINGTRANSFER:
        case proto::PAYMENTWORKFLOWTYPE_INCOMINGTRANSFER:
        case proto::PAYMENTWORKFLOWTYPE_INTERNALTRANSFER: {
            auto transferData = Transfer(nym, id, workflowID);
            const auto& [transfer, contract] = transferData;

            OT_ASSERT(transfer)

            if (0 < contract->Version()) {
                std::string amount{};
                const bool haveAmount = contract->FormatAmountLocale(
                    transfer->GetAmount(),
                    amount, ",", ".");

                if (haveAmount) {
                    const std::string text =
                        *output + std::string{" for "} + amount;
                    *output = text;
                }
            }
        } break;

        case proto::PAYMENTWORKFLOWTYPE_ERROR:
        default: {

            return nullptr;
        }
    }

    return std::move(output);
}

void Activity::preload(
    OTPasswordPrompt reason,
    const identifier::Nym& nymID,
    const Identifier& id,
    const StorageBox box) const noexcept
{
    const auto message = Mail(nymID, id, box);

    if (!message) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Unable to load message ")(id)
            .Flush();

        return;
    }

    auto nym = api_.Wallet().Nym(nymID);

    if (false == bool(nym)) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Unable to load recipent nym.")
            .Flush();

        return;
    }

    LogVerbose(OT_METHOD)(__FUNCTION__)(": Decrypting message ")(id).Flush();
    auto peerObject =
        api_.Factory().PeerObject(nym, message->m_ascPayload, reason);
    LogVerbose(OT_METHOD)(__FUNCTION__)(": Message ")(id)(" decrypted.")
        .Flush();

    if (!peerObject) {
        LogOutput(OT_METHOD)(__FUNCTION__)(
            ": Unable to instantiate peer object.")
            .Flush();

        return;
    }

    if (!peerObject->Message()) {
        LogOutput(OT_METHOD)(__FUNCTION__)(
            ": Peer object does not contain a message.")
            .Flush();

        return;
    }

    Lock lock(mail_cache_lock_);
    auto& output = mail_cache_[id];
    lock.unlock();
    output.reset(new std::string(*peerObject->Message()));
}

void Activity::PreloadActivity(
    const identifier::Nym& nymID,
    const std::size_t count,
    const PasswordPrompt& reason) const noexcept
{
    std::thread preload(
        &Activity::activity_preload_thread,
        this,
        OTPasswordPrompt{reason},
        Identifier::Factory(nymID),
        count);
    preload.detach();
}

void Activity::PreloadThread(
    const identifier::Nym& nymID,
    const Identifier& threadID,
    const std::size_t start,
    const std::size_t count,
    const PasswordPrompt& reason) const noexcept
{
    const std::string nym = nymID.str();
    const std::string thread = threadID.str();
    std::thread preload(
        &Activity::thread_preload_thread,
        this,
        OTPasswordPrompt{reason},
        nym,
        thread,
        start,
        count);
    preload.detach();
}

void Activity::publish(const identifier::Nym& nymID, const Identifier& threadID)
    const noexcept
{
    auto& socket = get_publisher(nymID);
    auto work = socket.Context().TaggedMessage(WorkType::ActivityThreadUpdated);
    work->AddFrame(threadID);
    socket.Send(work);
}

auto Activity::start_publisher(const std::string& endpoint) const noexcept
    -> OTZMQPublishSocket
{
    auto output = api_.ZeroMQ().PublishSocket();
    const auto started = output->Start(endpoint);

    OT_ASSERT(started);

    LogDetail(OT_METHOD)(__FUNCTION__)(": Publisher started on ")(endpoint)
        .Flush();

    return output;
}

auto Activity::Thread(const identifier::Nym& nymID, const Identifier& threadID)
    const noexcept -> std::shared_ptr<proto::StorageThread>
{
    sLock lock(shared_lock_);
    std::shared_ptr<proto::StorageThread> output;
    api_.Storage().Load(nymID.str(), threadID.str(), output);

    return output;
}

void Activity::thread_preload_thread(
    OTPasswordPrompt reason,
    const std::string nymID,
    const std::string threadID,
    const std::size_t start,
    const std::size_t count) const noexcept
{
    std::shared_ptr<proto::StorageThread> thread{};
    const bool loaded = api_.Storage().Load(nymID, threadID, thread);

    if (false == loaded) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Unable to load thread ")(
            threadID)(" for nym ")(nymID)
            .Flush();

        return;
    }

    const std::size_t size = thread->item_size();
    std::size_t cached{0};

    if (start > size) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Error: start larger than size "
                                           "(")(start)(" / ")(size)(").")
            .Flush();

        return;
    }

    for (auto i = (size - start); i > 0; --i) {
        if (cached >= count) { break; }

        const auto& item = thread->item(i - 1);
        const auto& box = static_cast<StorageBox>(item.box());

        switch (box) {
            case StorageBox::MAILINBOX:
            case StorageBox::MAILOUTBOX: {
                LogOutput(OT_METHOD)(__FUNCTION__)(": Preloading item ")(
                    item.id())(" in thread ")(threadID)
                    .Flush();
                MailText(
                    identifier::Nym::Factory(nymID),
                    Identifier::Factory(item.id()),
                    box,
                    reason);
                ++cached;
            } break;
            default: {
                continue;
            }
        }
    }
}

auto Activity::ThreadPublisher(const identifier::Nym& nym) const noexcept
    -> std::string
{
    std::string endpoint{};
    get_publisher(nym, endpoint);

    return endpoint;
}

auto Activity::Threads(const identifier::Nym& nym, const bool unreadOnly)
    const noexcept -> ObjectList
{
    const std::string nymID = nym.str();
    auto output = api_.Storage().ThreadList(nymID, unreadOnly);

    for (auto& it : output) {
        const auto& threadID = it.first;
        auto& label = it.second;

        if (label.empty()) {
            auto contact = contact_.Contact(Identifier::Factory(threadID));

            if (contact) {
                const auto& name = contact->Label();

                if (label != name) {
                    api_.Storage().SetThreadAlias(nymID, threadID, name);
                    label = name;
                }
            }
        }
    }

    return output;
}

auto Activity::UnreadCount(const identifier::Nym& nymId) const noexcept
    -> std::size_t
{
    const std::string nym = nymId.str();
    std::size_t output{0};

    const auto& threads = api_.Storage().ThreadList(nym, true);

    for (const auto& it : threads) {
        const auto& threadId = it.first;
        output += api_.Storage().UnreadCount(nym, threadId);
    }

    return output;
}

auto Activity::verify_thread_exists(
    const std::string& nym,
    const std::string& thread) const noexcept -> bool
{
    const auto list = api_.Storage().ThreadList(nym, false);

    for (const auto& it : list) {
        const auto& id = it.first;

        if (id == thread) { return true; }
    }

    return api_.Storage().CreateThread(nym, thread, {thread});
}
}  // namespace opentxs::api::client::implementation
