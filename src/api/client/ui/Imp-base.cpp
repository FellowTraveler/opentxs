// Copyright (c) 2010-2021 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include "0_stdafx.hpp"                // IWYU pragma: associated
#include "1_Internal.hpp"              // IWYU pragma: associated
#include "api/client/ui/Imp-base.hpp"  // IWYU pragma: associated

#include <map>
#include <memory>
#include <tuple>

#include "internal/api/client/Client.hpp"
#include "internal/core/Core.hpp"
#include "internal/ui/UI.hpp"
#include "opentxs/Types.hpp"
#include "opentxs/blockchain/BlockchainType.hpp"
#include "opentxs/core/Identifier.hpp"
#include "opentxs/core/Log.hpp"
#include "opentxs/core/identifier/Nym.hpp"
#include "opentxs/ui/Blockchains.hpp"
#include "ui/accountactivity/AccountActivity.hpp"
#include "ui/accountlist/AccountList.hpp"
#include "ui/accountsummary/AccountSummary.hpp"
#include "ui/activitysummary/ActivitySummary.hpp"
#include "ui/activitythread/ActivityThread.hpp"
#include "ui/blockchainselection/BlockchainSelection.hpp"
#include "ui/contact/Contact.hpp"
#include "ui/contactlist/ContactList.hpp"
#include "ui/messagablelist/MessagableList.hpp"
#include "ui/payablelist/PayableList.hpp"
#include "ui/profile/Profile.hpp"
#include "ui/unitlist/UnitList.hpp"

//#define OT_METHOD "opentxs::api::client::UI"

namespace opentxs::api::client
{
UI::Imp::Imp(
    const api::client::internal::Manager& api,
    const api::client::internal::Blockchain& blockchain,
    const Flag& running) noexcept
    : api_(api)
    , blockchain_(blockchain)
    , running_(running)
    , accounts_()
    , account_lists_()
    , account_summaries_()
    , activity_summaries_()
    , contacts_()
    , contact_lists_()
    , messagable_lists_()
    , payable_lists_()
    , activity_threads_()
    , profiles_()
    , unit_lists_()
    , blockchain_selection_()
    , update_manager_(api_)
{
    // WARNING: do not access api_.Wallet() during construction
}

auto UI::Imp::account_activity(
    const Lock& lock,
    const identifier::Nym& nymID,
    const Identifier& accountID,
    const SimpleCallback& cb) const noexcept -> AccountActivityMap::mapped_type&
{
    auto key = AccountActivityKey{nymID, accountID};
    auto it = accounts_.find(key);
#if OT_BLOCKCHAIN
    const auto chain = is_blockchain_account(accountID);
#endif  // OT_BLOCKCHAIN

    if (accounts_.end() == it) {
        it = accounts_
                 .emplace(
                     std::piecewise_construct,
                     std::forward_as_tuple(std::move(key)),
                     std::forward_as_tuple(
#if OT_BLOCKCHAIN
                         (chain.has_value()
                              ? opentxs::factory::BlockchainAccountActivityModel
                              : opentxs::factory::AccountActivityModel)
#else  // OT_BLOCKCHAIN
                         (opentxs::factory::AccountActivityModel)
#endif  // OT_BLOCKCHAIN
                             (api_, nymID, accountID, cb)))
                 .first;

        OT_ASSERT(it->second);
    }

    return it->second;
}

auto UI::Imp::AccountActivity(
    const identifier::Nym& nymID,
    const Identifier& accountID,
    const SimpleCallback cb) const noexcept
    -> const opentxs::ui::AccountActivity&
{
    Lock lock(lock_);

    return *account_activity(lock, nymID, accountID, cb);
}

auto UI::Imp::account_list(
    const Lock& lock,
    const identifier::Nym& nymID,
    const SimpleCallback& cb) const noexcept -> AccountListMap::mapped_type&
{
    auto key = AccountListKey{nymID};
    auto it = account_lists_.find(key);

    if (account_lists_.end() == it) {
        it = account_lists_
                 .emplace(
                     std::piecewise_construct,
                     std::forward_as_tuple(std::move(key)),
                     std::forward_as_tuple(
                         opentxs::factory::AccountListModel(api_, nymID, cb)))
                 .first;

        OT_ASSERT(it->second);
    }

    return it->second;
}

auto UI::Imp::AccountList(const identifier::Nym& nymID, const SimpleCallback cb)
    const noexcept -> const opentxs::ui::AccountList&
{
    Lock lock(lock_);

    return *account_list(lock, nymID, cb);
}

auto UI::Imp::account_summary(
    const Lock& lock,
    const identifier::Nym& nymID,
    const contact::ContactItemType currency,
    const SimpleCallback& cb) const noexcept -> AccountSummaryMap::mapped_type&
{
    auto key = AccountSummaryKey{nymID, currency};
    auto it = account_summaries_.find(key);

    if (account_summaries_.end() == it) {
        it =
            account_summaries_
                .emplace(
                    std::piecewise_construct,
                    std::forward_as_tuple(std::move(key)),
                    std::forward_as_tuple(opentxs::factory::AccountSummaryModel(
                        api_, nymID, currency, cb)))
                .first;

        OT_ASSERT(it->second);
    }

    return it->second;
}

auto UI::Imp::AccountSummary(
    const identifier::Nym& nymID,
    const contact::ContactItemType currency,
    const SimpleCallback cb) const noexcept
    -> const opentxs::ui::AccountSummary&
{
    Lock lock(lock_);

    return *account_summary(lock, nymID, currency, cb);
}

auto UI::Imp::ActivateUICallback(const Identifier& widget) const noexcept
    -> void
{
    update_manager_.ActivateUICallback(widget);
}

auto UI::Imp::activity_summary(
    const Lock& lock,
    const identifier::Nym& nymID,
    const SimpleCallback& cb) const noexcept -> ActivitySummaryMap::mapped_type&
{
    auto key = ActivitySummaryKey{nymID};
    auto it = activity_summaries_.find(key);

    if (activity_summaries_.end() == it) {
        it = activity_summaries_
                 .emplace(
                     std::piecewise_construct,
                     std::forward_as_tuple(std::move(key)),
                     std::forward_as_tuple(
                         opentxs::factory::ActivitySummaryModel(
                             api_, running_, nymID, cb)))
                 .first;

        OT_ASSERT(it->second);
    }

    return it->second;
}

auto UI::Imp::ActivitySummary(
    const identifier::Nym& nymID,
    const SimpleCallback cb) const noexcept
    -> const opentxs::ui::ActivitySummary&
{
    Lock lock(lock_);

    return *activity_summary(lock, nymID, cb);
}

auto UI::Imp::activity_thread(
    const Lock& lock,
    const identifier::Nym& nymID,
    const Identifier& threadID,
    const SimpleCallback& cb) const noexcept -> ActivityThreadMap::mapped_type&
{
    auto key = ActivityThreadKey{nymID, threadID};
    auto it = activity_threads_.find(key);

    if (activity_threads_.end() == it) {
        it =
            activity_threads_
                .emplace(
                    std::piecewise_construct,
                    std::forward_as_tuple(std::move(key)),
                    std::forward_as_tuple(opentxs::factory::ActivityThreadModel(
                        api_, nymID, threadID, cb)))
                .first;

        OT_ASSERT(it->second);
    }

    return it->second;
}

auto UI::Imp::ActivityThread(
    const identifier::Nym& nymID,
    const Identifier& threadID,
    const SimpleCallback cb) const noexcept
    -> const opentxs::ui::ActivityThread&
{
    Lock lock(lock_);

    return *activity_thread(lock, nymID, threadID, cb);
}

auto UI::Imp::BlockchainIssuerID(const opentxs::blockchain::Type chain)
    const noexcept -> const identifier::Nym&
{
    return opentxs::blockchain::IssuerID(api_, chain);
}

auto UI::Imp::BlockchainNotaryID(const opentxs::blockchain::Type chain)
    const noexcept -> const identifier::Server&
{
    return opentxs::blockchain::NotaryID(api_, chain);
}

auto UI::Imp::blockchain_selection(
    const Lock& lock,
    const opentxs::ui::Blockchains key,
    const SimpleCallback cb) const noexcept
    -> BlockchainSelectionMap::mapped_type&
{
    auto it = blockchain_selection_.find(key);

    if (blockchain_selection_.end() == it) {
        it = blockchain_selection_
                 .emplace(
                     std::piecewise_construct,
                     std::forward_as_tuple(key),
                     std::forward_as_tuple(
                         opentxs::factory::BlockchainSelectionModel(
                             api_, blockchain_, key, cb)))
                 .first;

        OT_ASSERT(it->second);
    }

    return it->second;
}

auto UI::Imp::BlockchainSelection(
    const opentxs::ui::Blockchains type,
    const SimpleCallback updateCB) const noexcept
    -> const opentxs::ui::BlockchainSelection&
{
    auto lock = Lock{lock_};

    return *blockchain_selection(lock, type, updateCB);
}

auto UI::Imp::BlockchainUnitID(const opentxs::blockchain::Type chain)
    const noexcept -> const identifier::UnitDefinition&
{
    return opentxs::blockchain::UnitID(api_, chain);
}

auto UI::Imp::ClearUICallbacks(const Identifier& widget) const noexcept -> void
{
    update_manager_.ClearUICallbacks(widget);
}

auto UI::Imp::contact(
    const Lock& lock,
    const Identifier& contactID,
    const SimpleCallback& cb) const noexcept -> ContactMap::mapped_type&
{
    auto key = ContactKey{contactID};
    auto it = contacts_.find(key);

    if (contacts_.end() == it) {
        it = contacts_
                 .emplace(
                     std::piecewise_construct,
                     std::forward_as_tuple(std::move(key)),
                     std::forward_as_tuple(
                         opentxs::factory::ContactModel(api_, contactID, cb)))
                 .first;

        OT_ASSERT(it->second);
    }

    return it->second;
}

auto UI::Imp::Contact(const Identifier& contactID, const SimpleCallback cb)
    const noexcept -> const opentxs::ui::Contact&
{
    Lock lock(lock_);

    return *contact(lock, contactID, cb);
}

auto UI::Imp::contact_list(
    const Lock& lock,
    const identifier::Nym& nymID,
    const SimpleCallback& cb) const noexcept -> ContactListMap::mapped_type&
{
    auto key = ContactListKey{nymID};
    auto it = contact_lists_.find(key);

    if (contact_lists_.end() == it) {
        it = contact_lists_
                 .emplace(
                     std::piecewise_construct,
                     std::forward_as_tuple(std::move(key)),
                     std::forward_as_tuple(
                         opentxs::factory::ContactListModel(api_, nymID, cb)))
                 .first;

        OT_ASSERT(it->second);
    }

    return it->second;
}

auto UI::Imp::ContactList(const identifier::Nym& nymID, const SimpleCallback cb)
    const noexcept -> const opentxs::ui::ContactList&
{
    Lock lock(lock_);

    return *contact_list(lock, nymID, cb);
}

auto UI::Imp::is_blockchain_account(const Identifier& id) const noexcept
    -> std::optional<opentxs::blockchain::Type>
{
    const auto [chain, owner] = blockchain_.LookupAccount(id);

    if (opentxs::blockchain::Type::Unknown == chain) { return std::nullopt; }

    return chain;
}

auto UI::Imp::messagable_list(
    const Lock& lock,
    const identifier::Nym& nymID,
    const SimpleCallback& cb) const noexcept -> MessagableListMap::mapped_type&
{
    auto key = MessagableListKey{nymID};
    auto it = messagable_lists_.find(key);

    if (messagable_lists_.end() == it) {
        it =
            messagable_lists_
                .emplace(
                    std::piecewise_construct,
                    std::forward_as_tuple(std::move(key)),
                    std::forward_as_tuple(
                        opentxs::factory::MessagableListModel(api_, nymID, cb)))
                .first;
    }

    return it->second;
}

auto UI::Imp::MessagableList(
    const identifier::Nym& nymID,
    const SimpleCallback cb) const noexcept
    -> const opentxs::ui::MessagableList&
{
    Lock lock(lock_);

    return *messagable_list(lock, nymID, cb);
}

auto UI::Imp::payable_list(
    const Lock& lock,
    const identifier::Nym& nymID,
    const contact::ContactItemType currency,
    const SimpleCallback& cb) const noexcept -> PayableListMap::mapped_type&
{
    auto key = PayableListKey{nymID, currency};
    auto it = payable_lists_.find(key);

    if (payable_lists_.end() == it) {
        it = payable_lists_
                 .emplace(
                     std::piecewise_construct,
                     std::forward_as_tuple(std::move(key)),
                     std::forward_as_tuple(opentxs::factory::PayableListModel(
                         api_, nymID, currency, cb)))
                 .first;
    }

    return it->second;
}

auto UI::Imp::PayableList(
    const identifier::Nym& nymID,
    contact::ContactItemType currency,
    const SimpleCallback cb) const noexcept -> const opentxs::ui::PayableList&
{
    Lock lock(lock_);

    return *payable_list(lock, nymID, currency, cb);
}

auto UI::Imp::profile(
    const Lock& lock,
    const identifier::Nym& nymID,
    const SimpleCallback& cb) const noexcept -> ProfileMap::mapped_type&
{
    auto key = ProfileKey{nymID};
    auto it = profiles_.find(key);

    if (profiles_.end() == it) {
        it = profiles_
                 .emplace(
                     std::piecewise_construct,
                     std::forward_as_tuple(std::move(key)),
                     std::forward_as_tuple(
                         opentxs::factory::ProfileModel(api_, nymID, cb)))
                 .first;

        OT_ASSERT(it->second);
    }

    return it->second;
}

auto UI::Imp::Profile(const identifier::Nym& nymID, const SimpleCallback cb)
    const noexcept -> const opentxs::ui::Profile&
{
    Lock lock(lock_);

    return *profile(lock, nymID, cb);
}

auto UI::Imp::RegisterUICallback(
    const Identifier& widget,
    const SimpleCallback& cb) const noexcept -> void
{
    update_manager_.RegisterUICallback(widget, cb);
}

auto UI::Imp::Shutdown() noexcept -> void
{
    ShutdownCallbacks();
    ShutdownModels();
}

auto UI::Imp::ShutdownCallbacks() noexcept -> void
{
    const auto clearCallbacks = [](auto& map) {
        for (auto& [key, widget] : map) {
            if (widget) { widget->ClearCallbacks(); }
        }
    };
    auto lock = Lock{lock_};
    clearCallbacks(blockchain_selection_);
    clearCallbacks(unit_lists_);
    clearCallbacks(profiles_);
    clearCallbacks(activity_threads_);
    clearCallbacks(payable_lists_);
    clearCallbacks(messagable_lists_);
    clearCallbacks(contact_lists_);
    clearCallbacks(contacts_);
    clearCallbacks(activity_summaries_);
    clearCallbacks(account_summaries_);
    clearCallbacks(account_lists_);
    clearCallbacks(accounts_);
}

auto UI::Imp::ShutdownModels() noexcept -> void
{
    blockchain_selection_.clear();
    unit_lists_.clear();
    profiles_.clear();
    activity_threads_.clear();
    payable_lists_.clear();
    messagable_lists_.clear();
    contact_lists_.clear();
    contacts_.clear();
    activity_summaries_.clear();
    account_summaries_.clear();
    account_lists_.clear();
    accounts_.clear();
}

auto UI::Imp::unit_list(
    const Lock& lock,
    const identifier::Nym& nymID,
    const SimpleCallback& cb) const noexcept -> UnitListMap::mapped_type&
{
    auto key = UnitListKey{nymID};
    auto it = unit_lists_.find(key);

    if (unit_lists_.end() == it) {
        it = unit_lists_
                 .emplace(
                     std::piecewise_construct,
                     std::forward_as_tuple(std::move(key)),
                     std::forward_as_tuple(
                         opentxs::factory::UnitListModel(api_, nymID, cb)))
                 .first;

        OT_ASSERT(it->second);
    }

    return it->second;
}

auto UI::Imp::UnitList(const identifier::Nym& nymID, const SimpleCallback cb)
    const noexcept -> const opentxs::ui::UnitList&
{
    Lock lock(lock_);

    return *unit_list(lock, nymID, cb);
}

UI::Imp::~Imp() { Shutdown(); }
}  // namespace opentxs::api::client
