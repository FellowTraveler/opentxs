// Copyright (c) 2010-2020 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#pragma once

#if OT_WITH_QML
#include <QtQml/QQmlEngine>
#endif
#include <chrono>
#include <cstdint>
#include <memory>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

#include "opentxs/Types.hpp"
#include "opentxs/Version.hpp"
#include "opentxs/api/Core.hpp"
#include "opentxs/api/Factory.hpp"
#include "opentxs/blockchain/BlockchainType.hpp"
#include "opentxs/blockchain/Types.hpp"
#include "opentxs/core/Identifier.hpp"
#include "opentxs/core/identifier/Nym.hpp"
#include "opentxs/protobuf/ContactEnums.pb.h"
#include "opentxs/protobuf/PaymentWorkflowEnums.pb.h"
#include "opentxs/ui/AccountActivity.hpp"
#include "opentxs/ui/AccountList.hpp"
#include "opentxs/ui/AccountListItem.hpp"
#include "opentxs/ui/AccountSummary.hpp"
#include "opentxs/ui/AccountSummaryItem.hpp"
#include "opentxs/ui/ActivitySummary.hpp"
#include "opentxs/ui/ActivitySummaryItem.hpp"
#include "opentxs/ui/ActivityThread.hpp"
#include "opentxs/ui/ActivityThreadItem.hpp"
#include "opentxs/ui/BalanceItem.hpp"
#if OT_BLOCKCHAIN
#include "opentxs/ui/BlockchainSelection.hpp"
#include "opentxs/ui/BlockchainSelectionItem.hpp"
#endif  // OT_BLOCKCHAIN
#include "opentxs/ui/Contact.hpp"
#include "opentxs/ui/ContactItem.hpp"
#include "opentxs/ui/ContactList.hpp"
#include "opentxs/ui/ContactListItem.hpp"
#include "opentxs/ui/ContactSection.hpp"
#include "opentxs/ui/ContactSubsection.hpp"
#include "opentxs/ui/IssuerItem.hpp"
#include "opentxs/ui/List.hpp"
#include "opentxs/ui/ListRow.hpp"
#include "opentxs/ui/MessagableList.hpp"
#include "opentxs/ui/PayableList.hpp"
#include "opentxs/ui/PayableListItem.hpp"
#include "opentxs/ui/Profile.hpp"
#include "opentxs/ui/ProfileItem.hpp"
#include "opentxs/ui/ProfileSection.hpp"
#include "opentxs/ui/ProfileSubsection.hpp"
#if OT_BLOCKCHAIN
#include "opentxs/ui/Types.hpp"
#endif  // OT_BLOCKCHAIN
#include "opentxs/ui/UnitList.hpp"
#include "opentxs/ui/UnitListItem.hpp"
#include "opentxs/ui/Widget.hpp"
#include "util/Blank.hpp"

namespace opentxs
{
namespace api
{
namespace client
{
namespace internal
{
struct Blockchain;
struct Manager;
}  // namespace internal
}  // namespace client
}  // namespace api

namespace contract
{
class Server;
class Unit;
}  // namespace contract

namespace identifier
{
class Server;
class UnitDefinition;
}  // namespace identifier

namespace network
{
namespace zeromq
{
namespace socket
{
class Publish;
}  // namespace socket
}  // namespace zeromq
}  // namespace network

namespace ui
{
namespace implementation
{
class AccountActivity;
class AccountList;
class AccountSummary;
class ActivitySummary;
class ActivityThread;
class BlockchainSelection;
class Contact;
class ContactList;
class MessagableList;
class PayableList;
class Profile;
class UnitList;
}  // namespace implementation
}  // namespace ui

class Flag;
}  // namespace opentxs

namespace opentxs::ui::internal
{
namespace blank
{
struct AccountListItem;
struct AccountSummaryItem;
struct ActivitySummaryItem;
struct ActivityThreadItem;
struct BalanceItem;
struct BlockchainSelectionItem;
struct ContactItem;
struct ContactListItem;
struct ContactSection;
struct ContactSubsection;
struct IssuerItem;
struct PayableListItem;
struct ProfileItem;
struct ProfileSection;
struct ProfileSubsection;
struct UnitListItem;
}  // namespace blank

struct AccountActivity;
struct AccountList;
struct AccountListItem;
struct AccountSummary;
struct AccountSummaryItem;
struct ActivitySummary;
struct ActivitySummaryItem;
struct ActivityThread;
struct ActivityThreadItem;
struct BalanceItem;
struct BlockchainSelection;
struct BlockchainSelectionItem;
struct Contact;
struct ContactItem;
struct ContactList;
struct ContactListItem;
struct ContactSection;
struct ContactSubsection;
struct IssuerItem;
struct MessagableList;
struct PayableList;
struct PayableListItem;
struct Profile;
struct ProfileItem;
struct ProfileSection;
struct ProfileSubsection;
struct UnitList;
struct UnitListItem;
}  // namespace opentxs::ui::internal

#if OT_BLOCKCHAIN
namespace opentxs::ui
{
auto AccountID(const api::Core& api, const blockchain::Type chain) noexcept
    -> const Identifier&;
auto AccountName(const blockchain::Type chain) noexcept -> std::string;
auto Chain(const api::Core& api, const Identifier& account) noexcept
    -> blockchain::Type;
auto NotaryID(const api::Core& api, const blockchain::Type chain) noexcept
    -> const identifier::Server&;
auto UnitID(const api::Core& api, const blockchain::Type chain) noexcept
    -> const identifier::UnitDefinition&;
}  // namespace opentxs::ui
#endif  // OT_BLOCKCHAIN

namespace opentxs::ui::implementation
{
using CustomData = std::vector<void*>;

// Account activity
using AccountActivityPrimaryID = OTNymID;
using AccountActivityExternalInterface = ui::AccountActivity;
using AccountActivityInternalInterface = ui::internal::AccountActivity;
/** WorkflowID, state */
using AccountActivityRowID = std::pair<OTIdentifier, proto::PaymentEventType>;
using AccountActivityRowInterface = ui::BalanceItem;
using AccountActivityRowInternal = ui::internal::BalanceItem;
using AccountActivityRowBlank = ui::internal::blank::BalanceItem;
using AccountActivitySortKey = Time;

// Account list
using AccountListPrimaryID = OTNymID;
using AccountListExternalInterface = ui::AccountList;
using AccountListInternalInterface = ui::internal::AccountList;
using AccountListRowID = OTIdentifier;
using AccountListRowInterface = ui::AccountListItem;
using AccountListRowInternal = ui::internal::AccountListItem;
using AccountListRowBlank = ui::internal::blank::AccountListItem;
// type, notary ID
using AccountListSortKey = std::pair<proto::ContactItemType, std::string>;

// Account summary
using AccountSummaryPrimaryID = OTNymID;
using AccountSummaryExternalInterface = ui::AccountSummary;
using AccountSummaryInternalInterface = ui::internal::AccountSummary;
using AccountSummaryRowID = OTNymID;
using AccountSummaryRowInterface = ui::IssuerItem;
using AccountSummaryRowInternal = ui::internal::IssuerItem;
using AccountSummaryRowBlank = ui::internal::blank::IssuerItem;
using AccountSummarySortKey = std::pair<bool, std::string>;

using IssuerItemPrimaryID = OTNymID;
using IssuerItemExternalInterface = AccountSummaryRowInterface;
using IssuerItemInternalInterface = ui::internal::IssuerItem;
using IssuerItemRowID = std::pair<OTIdentifier, proto::ContactItemType>;
using IssuerItemRowInterface = ui::AccountSummaryItem;
using IssuerItemRowInternal = ui::internal::AccountSummaryItem;
using IssuerItemRowBlank = ui::internal::blank::AccountSummaryItem;
using IssuerItemSortKey = std::string;

// Activity summary
using ActivitySummaryPrimaryID = OTNymID;
using ActivitySummaryExternalInterface = ui::ActivitySummary;
using ActivitySummaryInternalInterface = ui::internal::ActivitySummary;
using ActivitySummaryRowID = OTIdentifier;
using ActivitySummaryRowInterface = ui::ActivitySummaryItem;
using ActivitySummaryRowInternal = ui::internal::ActivitySummaryItem;
using ActivitySummaryRowBlank = ui::internal::blank::ActivitySummaryItem;
using ActivitySummarySortKey = std::pair<Time, std::string>;

// Activity thread
using ActivityThreadPrimaryID = OTNymID;
using ActivityThreadExternalInterface = ui::ActivityThread;
using ActivityThreadInternalInterface = ui::internal::ActivityThread;
/** item id, box, accountID, taskID */
using ActivityThreadRowID = std::tuple<OTIdentifier, StorageBox, OTIdentifier>;
using ActivityThreadRowInterface = ui::ActivityThreadItem;
using ActivityThreadRowInternal = ui::internal::ActivityThreadItem;
using ActivityThreadRowBlank = ui::internal::blank::ActivityThreadItem;
/** timestamp, index */
using ActivityThreadSortKey = std::pair<Time, std::uint64_t>;

#if OT_BLOCKCHAIN
using BlockchainSelectionPrimaryID = OTIdentifier;
using BlockchainSelectionExternalInterface = ui::BlockchainSelection;
using BlockchainSelectionInternalInterface = ui::internal::BlockchainSelection;
using BlockchainSelectionRowID = blockchain::Type;
using BlockchainSelectionRowInterface = ui::BlockchainSelectionItem;
using BlockchainSelectionRowInternal = ui::internal::BlockchainSelectionItem;
using BlockchainSelectionRowBlank =
    ui::internal::blank::BlockchainSelectionItem;
using BlockchainSelectionSortKey = std::pair<bool, std::string>;
#endif  // OT_BLOCKCHAIN

// Contact
using ContactPrimaryID = OTIdentifier;
using ContactExternalInterface = ui::Contact;
using ContactInternalInterface = ui::internal::Contact;
using ContactRowID = proto::ContactSectionName;
using ContactRowInterface = ui::ContactSection;
using ContactRowInternal = ui::internal::ContactSection;
using ContactRowBlank = ui::internal::blank::ContactSection;
using ContactSortKey = int;

using ContactSectionPrimaryID = ContactPrimaryID;
using ContactSectionExternalInterface = ContactRowInterface;
using ContactSectionInternalInterface = ui::internal::ContactSection;
using ContactSectionRowID =
    std::pair<proto::ContactSectionName, proto::ContactItemType>;
using ContactSectionRowInterface = ui::ContactSubsection;
using ContactSectionRowInternal = ui::internal::ContactSubsection;
using ContactSectionRowBlank = ui::internal::blank::ContactSubsection;
using ContactSectionSortKey = int;

using ContactSubsectionPrimaryID = ContactSectionPrimaryID;
using ContactSubsectionExternalInterface = ContactSectionRowInterface;
using ContactSubsectionInternalInterface = ui::internal::ContactSubsection;
using ContactSubsectionRowID = OTIdentifier;
using ContactSubsectionRowInterface = ui::ContactItem;
using ContactSubsectionRowInternal = ui::internal::ContactItem;
using ContactSubsectionRowBlank = ui::internal::blank::ContactItem;
using ContactSubsectionSortKey = int;

// Contact list
using ContactListPrimaryID = OTNymID;
using ContactListExternalInterface = ui::ContactList;
using ContactListInternalInterface = ui::internal::ContactList;
using ContactListRowID = OTIdentifier;
using ContactListRowInterface = ui::ContactListItem;
using ContactListRowInternal = ui::internal::ContactListItem;
using ContactListRowBlank = ui::internal::blank::ContactListItem;
using ContactListSortKey = std::string;

// Messagable list
using MessagableListPrimaryID = OTNymID;
using MessagableExternalInterface = ui::MessagableList;
using MessagableInternalInterface = ui::internal::MessagableList;
using MessagableListRowID = ContactListRowID;
using MessagableListRowInterface = ContactListRowInterface;
using MessagableListRowInternal = ContactListRowInternal;
using MessagableListRowBlank = ContactListRowBlank;
using MessagableListSortKey = std::string;

// Payable list
using PayablePrimaryID = OTNymID;
using PayableExternalInterface = ui::PayableList;
using PayableInternalInterface = ui::internal::PayableList;
using PayableListRowID = ContactListRowID;
using PayableListRowInterface = ui::PayableListItem;
using PayableListRowInternal = ui::internal::PayableListItem;
using PayableListRowBlank = ui::internal::blank::PayableListItem;
using PayableListSortKey = std::string;

// Profile
using ProfilePrimaryID = OTNymID;
using ProfileExternalInterface = ui::Profile;
using ProfileInternalInterface = ui::internal::Profile;
using ProfileRowID = proto::ContactSectionName;
using ProfileRowInterface = ui::ProfileSection;
using ProfileRowInternal = ui::internal::ProfileSection;
using ProfileRowBlank = ui::internal::blank::ProfileSection;
using ProfileSortKey = int;

using ProfileSectionPrimaryID = ProfilePrimaryID;
using ProfileSectionExternalInterface = ProfileRowInterface;
using ProfileSectionInternalInterface = ui::internal::ProfileSection;
using ProfileSectionRowID =
    std::pair<proto::ContactSectionName, proto::ContactItemType>;
using ProfileSectionRowInterface = ui::ProfileSubsection;
using ProfileSectionRowInternal = ui::internal::ProfileSubsection;
using ProfileSectionRowBlank = ui::internal::blank::ProfileSubsection;
using ProfileSectionSortKey = int;

using ProfileSubsectionPrimaryID = ProfileSectionPrimaryID;
using ProfileSubsectionExternalInterface = ProfileSectionRowInterface;
using ProfileSubsectionInternalInterface = ui::internal::ProfileSubsection;
using ProfileSubsectionRowID = OTIdentifier;
using ProfileSubsectionRowInterface = ui::ProfileItem;
using ProfileSubsectionRowInternal = ui::internal::ProfileItem;
using ProfileSubsectionRowBlank = ui::internal::blank::ProfileItem;
using ProfileSubsectionSortKey = int;

// Unit list
using UnitListPrimaryID = OTNymID;
using UnitListExternalInterface = ui::UnitList;
using UnitListInternalInterface = ui::internal::UnitList;
using UnitListRowID = proto::ContactItemType;
using UnitListRowInterface = ui::UnitListItem;
using UnitListRowInternal = ui::internal::UnitListItem;
using UnitListRowBlank = ui::internal::blank::UnitListItem;
using UnitListSortKey = std::string;
}  // namespace opentxs::ui::implementation

namespace opentxs
{
template <typename T>
struct make_blank;

template <>
struct make_blank<ui::implementation::AccountActivityRowID> {
    static auto value(const api::Core& api)
        -> ui::implementation::AccountActivityRowID
    {
        return {api.Factory().Identifier(), proto::PAYMENTEVENTTYPE_ERROR};
    }
};

template <>
struct make_blank<ui::implementation::IssuerItemRowID> {
    static auto value(const api::Core& api)
        -> ui::implementation::IssuerItemRowID
    {
        return {api.Factory().Identifier(), proto::CITEMTYPE_ERROR};
    }
};

template <>
struct make_blank<ui::implementation::ActivityThreadRowID> {
    static auto value(const api::Core& api)
        -> ui::implementation::ActivityThreadRowID
    {
        return {api.Factory().Identifier(), {}, api.Factory().Identifier()};
    }
};
}  // namespace opentxs

namespace opentxs::ui::internal
{
struct List : virtual public ui::List {
#if OT_QT
    virtual auto emit_begin_insert_rows(
        const QModelIndex& parent,
        int first,
        int last) const noexcept -> void = 0;
    virtual auto emit_begin_move_rows(
        const QModelIndex& source,
        int start,
        int end,
        const QModelIndex& destination,
        int to) const noexcept -> void = 0;
    virtual auto emit_begin_remove_rows(
        const QModelIndex& parent,
        int first,
        int last) const noexcept -> void = 0;
    virtual auto emit_data_changed(
        const QModelIndex& topLeft,
        const QModelIndex& bottomRight) noexcept -> void = 0;
    virtual auto emit_end_insert_rows() const noexcept -> void = 0;
    virtual auto emit_end_move_rows() const noexcept -> void = 0;
    virtual auto emit_end_remove_rows() const noexcept -> void = 0;
    virtual auto me() const noexcept -> QModelIndex = 0;
    virtual auto register_child(const void* child) const noexcept -> void = 0;
    virtual auto unregister_child(const void* child) const noexcept -> void = 0;
#endif

    ~List() override = default;
};

struct Row : virtual public ui::ListRow {
#if OT_QT
    virtual int qt_column_count() const noexcept { return 0; }
    virtual QVariant qt_data(
        [[maybe_unused]] const int column,
        [[maybe_unused]] const int role) const noexcept
    {
        return {};
    }
    virtual QModelIndex qt_index(
        [[maybe_unused]] const int row,
        [[maybe_unused]] const int column) const noexcept
    {
        return {};
    }
    virtual QModelIndex qt_parent() const noexcept = 0;
    virtual int qt_row_count() const noexcept { return 0; }
#endif  // OT_QT

    ~Row() override = default;
};
struct AccountActivity : virtual public List,
                         virtual public ui::AccountActivity {
    virtual auto last(const implementation::AccountActivityRowID& id)
        const noexcept -> bool = 0;
    // WARNING potential race condition. Child rows must never call this
    // except when directed by parent object
    virtual auto Contract() const noexcept -> const contract::Unit& = 0;
    // WARNING potential race condition. Child rows must never call this
    // except when directed by parent object
    virtual auto Notary() const noexcept -> const contract::Server& = 0;

    ~AccountActivity() override = default;
};
struct AccountList : virtual public List, virtual public ui::AccountList {
    virtual auto last(const implementation::AccountListRowID& id) const noexcept
        -> bool = 0;

    ~AccountList() override = default;
};
struct AccountListItem : virtual public Row,
                         virtual public ui::AccountListItem {
    virtual auto reindex(
        const implementation::AccountListSortKey& key,
        implementation::CustomData& custom) noexcept -> bool = 0;

    ~AccountListItem() override = default;
};
struct AccountSummary : virtual public List, virtual public ui::AccountSummary {
    virtual auto Currency() const -> proto::ContactItemType = 0;
#if OT_QT
    virtual int FindRow(
        const implementation::AccountSummaryRowID& id) const noexcept = 0;
#endif
    virtual auto last(const implementation::AccountSummaryRowID& id)
        const noexcept -> bool = 0;
    virtual auto NymID() const -> const identifier::Nym& = 0;

    ~AccountSummary() override = default;
};
struct AccountSummaryItem : virtual public Row,
                            virtual public ui::AccountSummaryItem {
    virtual auto reindex(
        const implementation::IssuerItemSortKey& key,
        implementation::CustomData& custom) noexcept -> bool = 0;

    ~AccountSummaryItem() override = default;
};
struct ActivitySummary : virtual public List,
                         virtual public ui::ActivitySummary {
    virtual auto last(const implementation::ActivitySummaryRowID& id)
        const noexcept -> bool = 0;

    ~ActivitySummary() override = default;
};
struct ActivitySummaryItem : virtual public Row,
                             virtual public ui::ActivitySummaryItem {
    virtual auto reindex(
        const implementation::ActivitySummarySortKey& key,
        implementation::CustomData& custom) noexcept -> bool = 0;

    ~ActivitySummaryItem() override = default;
};
struct ActivityThread : virtual public List {
    virtual auto last(const implementation::ActivityThreadRowID& id)
        const noexcept -> bool = 0;
    // custom
    virtual auto ThreadID() const -> std::string = 0;

    ~ActivityThread() override = default;
};
struct ActivityThreadItem : virtual public Row,
                            virtual public ui::ActivityThreadItem {
    virtual auto reindex(
        const implementation::ActivityThreadSortKey& key,
        implementation::CustomData& custom) noexcept -> bool = 0;

    ~ActivityThreadItem() override = default;
};
struct BalanceItem : virtual public Row, virtual public ui::BalanceItem {
    virtual auto reindex(
        const implementation::AccountActivitySortKey& key,
        implementation::CustomData& custom) noexcept -> bool = 0;

    ~BalanceItem() override = default;
};
#if OT_BLOCKCHAIN
struct BlockchainSelection : virtual public List,
                             virtual public ui::BlockchainSelection {
    virtual auto last(const implementation::BlockchainSelectionRowID& id)
        const noexcept -> bool = 0;

    ~BlockchainSelection() override = default;
};
struct BlockchainSelectionItem : virtual public Row,
                                 virtual public ui::BlockchainSelectionItem {
    virtual auto reindex(
        const implementation::BlockchainSelectionSortKey& key,
        implementation::CustomData& custom) noexcept -> bool = 0;

    ~BlockchainSelectionItem() override = default;
};
#endif  // OT_BLOCKCHAIN
struct Contact : virtual public List, virtual public ui::Contact {
#if OT_QT
    virtual int FindRow(
        const implementation::ContactRowID& id) const noexcept = 0;
#endif
    virtual auto last(const implementation::ContactRowID& id) const noexcept
        -> bool = 0;

    ~Contact() override = default;
};
struct ContactItem : virtual public Row, virtual public ui::ContactItem {
    virtual auto reindex(
        const implementation::ContactSubsectionSortKey& key,
        implementation::CustomData& custom) noexcept -> bool = 0;

    ~ContactItem() override = default;
};
struct ContactList : virtual public List {
    virtual auto last(const implementation::ContactListRowID& id) const noexcept
        -> bool = 0;
    // custom
    virtual auto ID() const -> const Identifier& = 0;

    ~ContactList() override = default;
};
struct ContactListItem : virtual public Row,
                         virtual public ui::ContactListItem {
    virtual auto reindex(
        const implementation::ContactListSortKey& key,
        implementation::CustomData& custom) noexcept -> bool = 0;

    ~ContactListItem() override = default;
};
struct ContactSection : virtual public List,
                        virtual public Row,
                        virtual public ui::ContactSection {
    virtual auto ContactID() const noexcept -> std::string = 0;
#if OT_QT
    virtual int FindRow(
        const implementation::ContactSectionRowID& id) const noexcept = 0;
#endif
    virtual auto last(const implementation::ContactSectionRowID& id)
        const noexcept -> bool = 0;

    virtual auto reindex(
        const implementation::ContactSortKey& key,
        implementation::CustomData& custom) noexcept -> bool = 0;

    ~ContactSection() override = default;
};
struct ContactSubsection : virtual public List,
                           virtual public Row,
                           virtual public ui::ContactSubsection {
    virtual auto reindex(
        const implementation::ContactSectionSortKey& key,
        implementation::CustomData& custom) noexcept -> bool = 0;
    // List
    virtual auto last(const implementation::ContactSubsectionRowID& id)
        const noexcept -> bool = 0;

    ~ContactSubsection() override = default;
};
struct IssuerItem : virtual public List,
                    virtual public Row,
                    virtual public ui::IssuerItem {
    // Row
    virtual auto reindex(
        const implementation::AccountSummarySortKey& key,
        implementation::CustomData& custom) noexcept -> bool = 0;
    // List
    virtual auto last(const implementation::IssuerItemRowID& id) const noexcept
        -> bool = 0;

    ~IssuerItem() override = default;
};
struct MessagableList : virtual public ContactList {
    ~MessagableList() override = default;
};
struct PayableList : virtual public ContactList {
    ~PayableList() override = default;
};
struct PayableListItem : virtual public Row,
                         virtual public ui::PayableListItem {
    virtual auto reindex(
        const implementation::PayableListSortKey& key,
        implementation::CustomData& custom) noexcept -> bool = 0;

    ~PayableListItem() override = default;
};
struct Profile : virtual public List, virtual public ui::Profile {
#if OT_QT
    virtual int FindRow(
        const implementation::ProfileRowID& id) const noexcept = 0;
#endif
    virtual auto last(const implementation::ProfileRowID& id) const noexcept
        -> bool = 0;
    virtual auto NymID() const -> const identifier::Nym& = 0;

    ~Profile() override = default;
};
struct ProfileSection : virtual public List,
                        virtual public Row,
                        virtual public ui::ProfileSection {
#if OT_QT
    virtual int FindRow(
        const implementation::ProfileSectionRowID& id) const noexcept = 0;
#endif
    virtual auto last(const implementation::ProfileSectionRowID& id)
        const noexcept -> bool = 0;
    virtual auto NymID() const noexcept -> const identifier::Nym& = 0;

    virtual auto reindex(
        const implementation::ProfileSortKey& key,
        implementation::CustomData& custom) noexcept -> bool = 0;

    ~ProfileSection() override = default;
};
struct ProfileSubsection : virtual public List,
                           virtual public Row,
                           virtual public ui::ProfileSubsection {
    // Row
    virtual auto reindex(
        const implementation::ProfileSectionSortKey& key,
        implementation::CustomData& custom) noexcept -> bool = 0;
    // List
    virtual auto last(const implementation::ProfileSubsectionRowID& id)
        const noexcept -> bool = 0;
    // custom
    virtual auto NymID() const noexcept -> const identifier::Nym& = 0;
    virtual auto Section() const noexcept -> proto::ContactSectionName = 0;

    ~ProfileSubsection() override = default;
};
struct ProfileItem : virtual public Row, virtual public ui::ProfileItem {
    virtual auto reindex(
        const implementation::ProfileSubsectionSortKey& key,
        implementation::CustomData& custom) noexcept -> bool = 0;

    ~ProfileItem() override = default;
};
struct UnitList : virtual public List, virtual public ui::UnitList {
    virtual auto last(const implementation::UnitListRowID& id) const noexcept
        -> bool = 0;

    ~UnitList() override = default;
};
struct UnitListItem : virtual public Row, virtual public ui::UnitListItem {
    virtual auto reindex(
        const implementation::UnitListSortKey& key,
        implementation::CustomData& custom) noexcept -> bool = 0;

    ~UnitListItem() override = default;
};

#if OT_QT
#if OT_WITH_QML
#define QT_PROXY_MODEL_WRAPPER_EXTRA(WrapperType, InterfaceType)               \
    WrapperType::WrapperType(InterfaceType& parent) noexcept                   \
        : parent_(parent)                                                      \
    {                                                                          \
        QQmlEngine::setObjectOwnership(this, QQmlEngine::CppOwnership);        \
        parent_.SetCallback([this]() -> void { notify(); });                   \
        setSourceModel(&parent_);                                              \
        init();                                                                \
    }                                                                          \
                                                                               \
    void WrapperType::notify() const noexcept { emit updated(); }
#define QT_PROXY_MODEL_WRAPPER(WrapperType, InterfaceType)                     \
    WrapperType::WrapperType(InterfaceType& parent) noexcept                   \
        : parent_(parent)                                                      \
    {                                                                          \
        QQmlEngine::setObjectOwnership(this, QQmlEngine::CppOwnership);        \
        parent_.SetCallback([this]() -> void { notify(); });                   \
        setSourceModel(&parent_);                                              \
    }                                                                          \
                                                                               \
    void WrapperType::notify() const noexcept { emit updated(); }
#else  // OT_WITH_QML
#define QT_PROXY_MODEL_WRAPPER_EXTRA(WrapperType, InterfaceType)               \
    WrapperType::WrapperType(InterfaceType& parent) noexcept                   \
        : parent_(parent)                                                      \
    {                                                                          \
        parent_.SetCallback([this]() -> void { notify(); });                   \
        setSourceModel(&parent_);                                              \
        init();                                                                \
    }                                                                          \
                                                                               \
    void WrapperType::notify() const noexcept { emit updated(); }
#define QT_PROXY_MODEL_WRAPPER(WrapperType, InterfaceType)                     \
    WrapperType::WrapperType(InterfaceType& parent) noexcept                   \
        : parent_(parent)                                                      \
    {                                                                          \
        parent_.SetCallback([this]() -> void { notify(); });                   \
        setSourceModel(&parent_);                                              \
    }                                                                          \
                                                                               \
    void WrapperType::notify() const noexcept { emit updated(); }
#endif  // OT_WITH_QML
#endif  // OT_QT

namespace blank
{
struct Widget : virtual public ui::Widget {
    void SetCallback(SimpleCallback) const noexcept final {}
    auto WidgetID() const noexcept -> OTIdentifier override
    {
        return Identifier::Factory();
    }
};
struct Row : virtual public ui::internal::Row, public Widget {
    auto Last() const noexcept -> bool final { return true; }
#if OT_QT
    QModelIndex qt_parent() const noexcept final { return {}; }
#endif  // OT_QT
    auto Valid() const noexcept -> bool final { return false; }
};
template <typename ListType, typename RowType, typename RowIDType>
struct List : virtual public ListType, public Row {
    auto First() const noexcept -> RowType final { return RowType{nullptr}; }
    auto last(const RowIDType&) const noexcept -> bool final { return false; }
#if OT_QT
    auto emit_begin_insert_rows(const QModelIndex& parent, int first, int last)
        const noexcept -> void final
    {
    }
    auto emit_begin_move_rows(
        const QModelIndex& source,
        int start,
        int end,
        const QModelIndex& destination,
        int to) const noexcept -> void final
    {
    }
    auto emit_begin_remove_rows(const QModelIndex& parent, int first, int last)
        const noexcept -> void final
    {
    }
    auto emit_data_changed(const QModelIndex&, const QModelIndex&) noexcept
        -> void final
    {
    }
    auto emit_end_insert_rows() const noexcept -> void final {}
    auto emit_end_move_rows() const noexcept -> void final {}
    auto emit_end_remove_rows() const noexcept -> void final {}
    auto me() const noexcept -> QModelIndex final { return {}; }
    auto register_child(const void* child) const noexcept -> void final {}
    auto unregister_child(const void* child) const noexcept -> void final {}
#endif
    auto Next() const noexcept -> RowType final { return RowType{nullptr}; }
    auto WidgetID() const noexcept -> OTIdentifier final
    {
        return blank::Widget::WidgetID();
    }
};
struct AccountListItem final : virtual public Row,
                               virtual public internal::AccountListItem {
    auto AccountID() const noexcept -> std::string final { return {}; }
    auto Balance() const noexcept -> Amount final { return {}; }
    auto ContractID() const noexcept -> std::string final { return {}; }
    auto DisplayBalance() const noexcept -> std::string final { return {}; }
    auto DisplayUnit() const noexcept -> std::string final { return {}; }
    auto Name() const noexcept -> std::string final { return {}; }
    auto NotaryID() const noexcept -> std::string final { return {}; }
    auto NotaryName() const noexcept -> std::string final { return {}; }
    auto Type() const noexcept -> AccountType final { return {}; }
    auto Unit() const noexcept -> proto::ContactItemType final { return {}; }

    auto reindex(
        const implementation::AccountListSortKey&,
        implementation::CustomData&) noexcept -> bool final
    {
        return false;
    }
};
struct AccountSummaryItem final : public Row,
                                  public internal::AccountSummaryItem {
    auto AccountID() const noexcept -> std::string final { return {}; }
    auto Balance() const noexcept -> Amount final { return {}; }
    auto DisplayBalance() const noexcept -> std::string final { return {}; }
    auto Name() const noexcept -> std::string final { return {}; }

    auto reindex(
        const implementation::IssuerItemSortKey&,
        implementation::CustomData&) noexcept -> bool final
    {
        return false;
    }
};
struct ActivitySummaryItem final
    : virtual public Row,
      virtual public internal::ActivitySummaryItem {
    auto DisplayName() const noexcept -> std::string final { return {}; }
    auto ImageURI() const noexcept -> std::string final { return {}; }
    auto Text() const noexcept -> std::string final { return {}; }
    auto ThreadID() const noexcept -> std::string final { return {}; }
    auto Timestamp() const noexcept -> Time final { return {}; }
    auto Type() const noexcept -> StorageBox final
    {
        return StorageBox::UNKNOWN;
    }

    auto reindex(
        const implementation::ActivitySummarySortKey&,
        implementation::CustomData&) noexcept -> bool final
    {
        return false;
    }
};
struct ActivityThreadItem final : public Row,
                                  public internal::ActivityThreadItem {
    auto Amount() const noexcept -> OTAmount final { return 0; }
    auto Deposit() const noexcept -> bool final { return false; }
    auto DisplayAmount() const noexcept -> std::string final { return {}; }
    auto Loading() const noexcept -> bool final { return false; }
    auto MarkRead() const noexcept -> bool final { return false; }
    auto Memo() const noexcept -> std::string final { return {}; }
    auto Pending() const noexcept -> bool final { return false; }
    auto Text() const noexcept -> std::string final { return {}; }
    auto Timestamp() const noexcept -> Time final { return {}; }
    auto Type() const noexcept -> StorageBox final
    {
        return StorageBox::UNKNOWN;
    }

    auto reindex(
        const implementation::ActivityThreadSortKey&,
        implementation::CustomData&) noexcept -> bool final
    {
        return false;
    }
};
struct BalanceItem final : public Row, public internal::BalanceItem {
    auto Amount() const noexcept -> OTAmount final { return {}; }
    auto Contacts() const noexcept -> std::vector<std::string> final
    {
        return {};
    }
    auto DisplayAmount() const noexcept -> std::string final { return {}; }
    auto Memo() const noexcept -> std::string final { return {}; }
    auto Workflow() const noexcept -> std::string final { return {}; }
    auto Text() const noexcept -> std::string final { return {}; }
    auto Timestamp() const noexcept -> Time final { return {}; }
    auto Type() const noexcept -> StorageBox final
    {
        return StorageBox::UNKNOWN;
    }
    auto UUID() const noexcept -> std::string final { return {}; }

    auto reindex(
        const implementation::AccountActivitySortKey&,
        implementation::CustomData&) noexcept -> bool final
    {
        return false;
    }
};
#if OT_BLOCKCHAIN
struct BlockchainSelectionItem final
    : virtual public Row,
      virtual public internal::BlockchainSelectionItem {

    auto Name() const noexcept -> std::string final { return {}; }
    auto IsEnabled() const noexcept -> bool final { return {}; }
    auto IsTestnet() const noexcept -> bool final { return {}; }
    auto Type() const noexcept -> blockchain::Type final { return {}; }

    auto reindex(
        const implementation::BlockchainSelectionSortKey&,
        implementation::CustomData&) noexcept -> bool final
    {
        return false;
    }
};
#endif  // OT_BLOCKCHAIN
struct ContactItem final : public Row, public internal::ContactItem {
    auto ClaimID() const noexcept -> std::string final { return {}; }
    auto IsActive() const noexcept -> bool final { return false; }
    auto IsPrimary() const noexcept -> bool final { return false; }
    auto Value() const noexcept -> std::string final { return {}; }

    auto reindex(
        const implementation::ContactSectionSortKey&,
        implementation::CustomData&) noexcept -> bool final
    {
        return false;
    }
};
struct ContactListItem : virtual public Row,
                         virtual public internal::ContactListItem {
    auto ContactID() const noexcept -> std::string final { return {}; }
    auto DisplayName() const noexcept -> std::string final { return {}; }
    auto ImageURI() const noexcept -> std::string final { return {}; }
    auto Section() const noexcept -> std::string final { return {}; }

    auto reindex(
        const implementation::ContactListSortKey&,
        implementation::CustomData&) noexcept -> bool override
    {
        return false;
    }
};
struct ContactSection final : public List<
                                  internal::ContactSection,
                                  OTUIContactSubsection,
                                  implementation::ContactSectionRowID> {
    auto ContactID() const noexcept -> std::string final { return {}; }
#if OT_QT
    int FindRow(
        const implementation::ContactSectionRowID& id) const noexcept final
    {
        return -1;
    }
#endif
    auto Name(const std::string& lang) const noexcept -> std::string final
    {
        return {};
    }
    auto Type() const noexcept -> proto::ContactSectionName final { return {}; }

    auto reindex(
        const implementation::ContactSortKey&,
        implementation::CustomData&) noexcept -> bool final
    {
        return false;
    }
};
struct ContactSubsection final : public List<
                                     internal::ContactSubsection,
                                     OTUIContactItem,
                                     implementation::ContactSubsectionRowID> {
    auto Name(const std::string& lang) const noexcept -> std::string final
    {
        return {};
    }
    auto Type() const noexcept -> proto::ContactItemType final { return {}; }

    auto reindex(
        const implementation::ContactSectionSortKey&,
        implementation::CustomData&) noexcept -> bool final
    {
        return false;
    }
};
struct IssuerItem final : public List<
                              internal::IssuerItem,
                              OTUIAccountSummaryItem,
                              implementation::IssuerItemRowID> {
    auto ConnectionState() const noexcept -> bool final { return {}; }
    auto Debug() const noexcept -> std::string final { return {}; }
    auto Name() const noexcept -> std::string final { return {}; }
    auto Trusted() const noexcept -> bool final { return {}; }

    auto reindex(
        const implementation::AccountSummarySortKey&,
        implementation::CustomData&) noexcept -> bool final
    {
        return false;
    }
};
struct PayableListItem final : virtual public ContactListItem,
                               virtual public internal::PayableListItem {
    auto PaymentCode() const noexcept -> std::string final { return {}; }

    auto reindex(
        const implementation::PayableListSortKey&,
        implementation::CustomData&) noexcept -> bool final
    {
        return false;
    }
};
struct ProfileItem : virtual public Row, virtual public internal::ProfileItem {
    auto ClaimID() const noexcept -> std::string final { return {}; }
    auto Delete() const noexcept -> bool final { return false; }
    auto IsActive() const noexcept -> bool final { return false; }
    auto IsPrimary() const noexcept -> bool final { return false; }
    auto Value() const noexcept -> std::string final { return {}; }
    auto SetActive(const bool& active) const noexcept -> bool final
    {
        return false;
    }
    auto SetPrimary(const bool& primary) const noexcept -> bool final
    {
        return false;
    }
    auto SetValue(const std::string& value) const noexcept -> bool final
    {
        return false;
    }

    auto reindex(
        const implementation::ProfileSubsectionSortKey&,
        implementation::CustomData&) noexcept -> bool final
    {
        return false;
    }
};
struct ProfileSection : public List<
                            internal::ProfileSection,
                            OTUIProfileSubsection,
                            implementation::ProfileSectionRowID> {
    auto AddClaim(
        const proto::ContactItemType type,
        const std::string& value,
        const bool primary,
        const bool active) const noexcept -> bool final
    {
        return false;
    }
    auto Delete(const int, const std::string&) const noexcept -> bool final
    {
        return false;
    }
#if OT_QT
    int FindRow(
        const implementation::ProfileSectionRowID& id) const noexcept final
    {
        return -1;
    }
#endif
    auto Items(const std::string&) const noexcept -> ItemTypeList final
    {
        return {};
    }
    auto Name(const std::string& lang) const noexcept -> std::string final
    {
        return {};
    }
    auto NymID() const noexcept -> const identifier::Nym& final
    {
        return nym_id_;
    }
    auto SetActive(const int, const std::string&, const bool) const noexcept
        -> bool final
    {
        return false;
    }
    auto SetPrimary(const int, const std::string&, const bool) const noexcept
        -> bool final
    {
        return false;
    }
    auto SetValue(const int, const std::string&, const std::string&)
        const noexcept -> bool final
    {
        return false;
    }
    auto Type() const noexcept -> proto::ContactSectionName final { return {}; }

    auto reindex(
        const implementation::ProfileSortKey& key,
        implementation::CustomData& custom) noexcept -> bool final
    {
        return false;
    }

private:
    const OTNymID nym_id_{identifier::Nym::Factory()};
};
struct ProfileSubsection : public List<
                               internal::ProfileSubsection,
                               OTUIProfileItem,
                               implementation::ProfileSubsectionRowID> {
    auto AddItem(const std::string&, const bool, const bool) const noexcept
        -> bool final
    {
        return false;
    }
    auto Delete(const std::string&) const noexcept -> bool final
    {
        return false;
    }
    auto Name(const std::string&) const noexcept -> std::string final
    {
        return {};
    }
    auto NymID() const noexcept -> const identifier::Nym& final
    {
        return nym_id_;
    }
    auto Type() const noexcept -> proto::ContactItemType final { return {}; }
    auto Section() const noexcept -> proto::ContactSectionName final
    {
        return {};
    }
    auto SetActive(const std::string&, const bool) const noexcept -> bool final
    {
        return false;
    }
    auto SetPrimary(const std::string&, const bool) const noexcept -> bool final
    {
        return false;
    }
    auto SetValue(const std::string&, const std::string&) const noexcept
        -> bool final
    {
        return false;
    }

    auto reindex(
        const implementation::ProfileSortKey&,
        implementation::CustomData&) noexcept -> bool final
    {
        return false;
    }

private:
    const OTNymID nym_id_{identifier::Nym::Factory()};
};
struct UnitListItem final : virtual public Row,
                            virtual public internal::UnitListItem {
    auto Name() const noexcept -> std::string final { return {}; }
    auto Unit() const noexcept -> proto::ContactItemType final { return {}; }

    auto reindex(
        const implementation::UnitListSortKey&,
        implementation::CustomData&) noexcept -> bool final
    {
        return false;
    }
};
}  // namespace blank
}  // namespace opentxs::ui::internal

namespace opentxs::factory
{
auto AccountActivityModel(
    const api::client::internal::Manager& api,
    const identifier::Nym& nymID,
    const opentxs::Identifier& accountID,
    const SimpleCallback& cb) noexcept
    -> std::unique_ptr<ui::implementation::AccountActivity>;
#if OT_QT
auto AccountActivityQtModel(
    ui::implementation::AccountActivity& parent) noexcept
    -> std::unique_ptr<ui::AccountActivityQt>;
#endif
auto AccountListItem(
    const ui::implementation::AccountListInternalInterface& parent,
    const api::client::internal::Manager& api,
    const ui::implementation::AccountListRowID& rowID,
    const ui::implementation::AccountListSortKey& sortKey,
    ui::implementation::CustomData& custom) noexcept
    -> std::shared_ptr<ui::implementation::AccountListRowInternal>;
auto AccountListModel(
    const api::client::internal::Manager& api,
    const identifier::Nym& nymID,
    const SimpleCallback& cb) noexcept
    -> std::unique_ptr<ui::implementation::AccountList>;
#if OT_QT
auto AccountListQtModel(ui::implementation::AccountList& parent) noexcept
    -> std::unique_ptr<ui::AccountListQt>;
#endif
auto AccountSummaryItem(
    const ui::implementation::IssuerItemInternalInterface& parent,
    const api::client::internal::Manager& api,
    const ui::implementation::IssuerItemRowID& rowID,
    const ui::implementation::IssuerItemSortKey& sortKey,
    ui::implementation::CustomData& custom) noexcept
    -> std::shared_ptr<ui::implementation::IssuerItemRowInternal>;
auto AccountSummaryModel(
    const api::client::internal::Manager& api,
    const identifier::Nym& nymID,
    const proto::ContactItemType currency,
    const SimpleCallback& cb) noexcept
    -> std::unique_ptr<ui::implementation::AccountSummary>;
#if OT_QT
auto AccountSummaryQtModel(ui::implementation::AccountSummary& parent) noexcept
    -> std::unique_ptr<ui::AccountSummaryQt>;
#endif
auto ActivitySummaryItem(
    const ui::implementation::ActivitySummaryInternalInterface& parent,
    const api::client::internal::Manager& api,
    const identifier::Nym& nymID,
    const ui::implementation::ActivitySummaryRowID& rowID,
    const ui::implementation::ActivitySummarySortKey& sortKey,
    ui::implementation::CustomData& custom,
    const Flag& running) noexcept
    -> std::shared_ptr<ui::implementation::ActivitySummaryRowInternal>;
auto ActivitySummaryModel(
    const api::client::internal::Manager& api,
    const Flag& running,
    const identifier::Nym& nymID,
    const SimpleCallback& cb) noexcept
    -> std::unique_ptr<ui::implementation::ActivitySummary>;
#if OT_QT
auto ActivitySummaryQtModel(
    ui::implementation::ActivitySummary& parent) noexcept
    -> std::unique_ptr<ui::ActivitySummaryQt>;
#endif
auto ActivityThreadModel(
    const api::client::internal::Manager& api,
    const identifier::Nym& nymID,
    const opentxs::Identifier& threadID,
    const SimpleCallback& cb) noexcept
    -> std::unique_ptr<ui::implementation::ActivityThread>;
#if OT_QT
auto ActivityThreadQtModel(ui::implementation::ActivityThread& parent) noexcept
    -> std::unique_ptr<ui::ActivityThreadQt>;
#endif
#if OT_BLOCKCHAIN
auto BlockchainAccountActivityModel(
    const api::client::internal::Manager& api,
    const identifier::Nym& nymID,
    const opentxs::Identifier& accountID,
    const SimpleCallback& cb) noexcept
    -> std::unique_ptr<ui::implementation::AccountActivity>;
auto BlockchainAccountListItem(
    const ui::implementation::AccountListInternalInterface& parent,
    const api::client::internal::Manager& api,
    const ui::implementation::AccountListRowID& rowID,
    const ui::implementation::AccountListSortKey& sortKey,
    ui::implementation::CustomData& custom) noexcept
    -> std::shared_ptr<ui::implementation::AccountListRowInternal>;
auto BlockchainActivityThreadItem(
    const ui::implementation::ActivityThreadInternalInterface& parent,
    const api::client::internal::Manager& api,
    const identifier::Nym& nymID,
    const ui::implementation::ActivityThreadRowID& rowID,
    const ui::implementation::ActivityThreadSortKey& sortKey,
    ui::implementation::CustomData& custom) noexcept
    -> std::shared_ptr<ui::implementation::ActivityThreadRowInternal>;
auto BlockchainSelectionModel(
    const api::client::internal::Manager& api,
    const api::client::internal::Blockchain& blockchain,
    const ui::Blockchains type,
    const SimpleCallback& cb) noexcept
    -> std::unique_ptr<ui::implementation::BlockchainSelection>;
auto BlockchainSelectionItem(
    const ui::implementation::BlockchainSelectionInternalInterface& parent,
    const api::client::internal::Manager& api,
    const ui::implementation::BlockchainSelectionRowID& rowID,
    const ui::implementation::BlockchainSelectionSortKey& sortKey,
    ui::implementation::CustomData& custom) noexcept
    -> std::shared_ptr<ui::implementation::BlockchainSelectionRowInternal>;
#if OT_QT
auto BlockchainSelectionQtModel(
    ui::implementation::BlockchainSelection& parent) noexcept
    -> std::unique_ptr<ui::BlockchainSelectionQt>;
#endif  // OT_QT
#endif  // OT_BLOCKCHAIN
auto BalanceItem(
    const ui::implementation::AccountActivityInternalInterface& parent,
    const api::client::internal::Manager& api,
    const ui::implementation::AccountActivityRowID& rowID,
    const ui::implementation::AccountActivitySortKey& sortKey,
    ui::implementation::CustomData& custom,
    const identifier::Nym& nymID,
    const opentxs::Identifier& accountID) noexcept
    -> std::shared_ptr<ui::implementation::AccountActivityRowInternal>;
auto ContactItemWidget(
    const ui::implementation::ContactSubsectionInternalInterface& parent,
    const api::client::internal::Manager& api,
    const ui::implementation::ContactSubsectionRowID& rowID,
    const ui::implementation::ContactSubsectionSortKey& sortKey,
    ui::implementation::CustomData& custom) noexcept
    -> std::shared_ptr<ui::implementation::ContactSubsectionRowInternal>;
auto ContactListItem(
    const ui::implementation::ContactListInternalInterface& parent,
    const api::client::internal::Manager& api,
    const ui::implementation::ContactListRowID& rowID,
    const ui::implementation::ContactListSortKey& key) noexcept
    -> std::shared_ptr<ui::implementation::ContactListRowInternal>;
auto ContactListModel(
    const api::client::internal::Manager& api,
    const identifier::Nym& nymID,
    const SimpleCallback& cb) noexcept
    -> std::unique_ptr<ui::implementation::ContactList>;
#if OT_QT
auto ContactListQtModel(ui::implementation::ContactList& parent) noexcept
    -> std::unique_ptr<ui::ContactListQt>;
#endif
auto ContactModel(
    const api::client::internal::Manager& api,
    const opentxs::Identifier& contactID,
    const SimpleCallback& cb) noexcept
    -> std::unique_ptr<ui::implementation::Contact>;
#if OT_QT
auto ContactQtModel(ui::implementation::Contact& parent) noexcept
    -> std::unique_ptr<ui::ContactQt>;
#endif
auto ContactSectionWidget(
    const ui::implementation::ContactInternalInterface& parent,
    const api::client::internal::Manager& api,
    const ui::implementation::ContactRowID& rowID,
    const ui::implementation::ContactSortKey& key,
    ui::implementation::CustomData& custom) noexcept
    -> std::shared_ptr<ui::implementation::ContactRowInternal>;
auto ContactSubsectionWidget(
    const ui::implementation::ContactSectionInternalInterface& parent,
    const api::client::internal::Manager& api,
    const ui::implementation::ContactSectionRowID& rowID,
    const ui::implementation::ContactSectionSortKey& key,
    ui::implementation::CustomData& custom) noexcept
    -> std::shared_ptr<ui::implementation::ContactSectionRowInternal>;
auto IssuerItem(
    const ui::implementation::AccountSummaryInternalInterface& parent,
    const api::client::internal::Manager& api,
    const ui::implementation::AccountSummaryRowID& rowID,
    const ui::implementation::AccountSummarySortKey& sortKey,
    ui::implementation::CustomData& custom,
    const proto::ContactItemType currency) noexcept
    -> std::shared_ptr<ui::implementation::AccountSummaryRowInternal>;
auto MailItem(
    const ui::implementation::ActivityThreadInternalInterface& parent,
    const api::client::internal::Manager& api,
    const identifier::Nym& nymID,
    const ui::implementation::ActivityThreadRowID& rowID,
    const ui::implementation::ActivityThreadSortKey& sortKey,
    ui::implementation::CustomData& custom,
    const bool loading,
    const bool pending) noexcept
    -> std::shared_ptr<ui::implementation::ActivityThreadRowInternal>;
auto MailItem(
    const ui::implementation::ActivityThreadInternalInterface& parent,
    const api::client::internal::Manager& api,
    const identifier::Nym& nymID,
    const ui::implementation::ActivityThreadRowID& rowID,
    const ui::implementation::ActivityThreadSortKey& sortKey,
    ui::implementation::CustomData& custom) noexcept
    -> std::shared_ptr<ui::implementation::ActivityThreadRowInternal>;
auto MessagableListItem(
    const ui::implementation::ContactListInternalInterface& parent,
    const api::client::internal::Manager& api,
    const ui::implementation::ContactListRowID& rowID,
    const ui::implementation::ContactListSortKey& key) noexcept
    -> std::shared_ptr<ui::implementation::MessagableListRowInternal>;
auto MessagableListModel(
    const api::client::internal::Manager& api,
    const identifier::Nym& nymID,
    const SimpleCallback& cb) noexcept
    -> std::unique_ptr<ui::implementation::MessagableList>;
#if OT_QT
auto MessagableListQtModel(ui::implementation::MessagableList& parent) noexcept
    -> std::unique_ptr<ui::MessagableListQt>;
#endif
auto PayableListItem(
    const ui::implementation::PayableInternalInterface& parent,
    const api::client::internal::Manager& api,
    const ui::implementation::PayableListRowID& rowID,
    const ui::implementation::PayableListSortKey& key,
    const std::string& paymentcode,
    const proto::ContactItemType& currency) noexcept
    -> std::shared_ptr<ui::implementation::PayableListRowInternal>;
auto PaymentItem(
    const ui::implementation::ActivityThreadInternalInterface& parent,
    const api::client::internal::Manager& api,
    const identifier::Nym& nymID,
    const ui::implementation::ActivityThreadRowID& rowID,
    const ui::implementation::ActivityThreadSortKey& sortKey,
    ui::implementation::CustomData& custom) noexcept
    -> std::shared_ptr<ui::implementation::ActivityThreadRowInternal>;
auto PayableListModel(
    const api::client::internal::Manager& api,
    const identifier::Nym& nymID,
    const proto::ContactItemType& currency,
    const SimpleCallback& cb) noexcept
    -> std::unique_ptr<ui::implementation::PayableList>;
#if OT_QT
auto PayableListQtModel(ui::implementation::PayableList& parent) noexcept
    -> std::unique_ptr<ui::PayableListQt>;
#endif
auto PendingSend(
    const ui::implementation::ActivityThreadInternalInterface& parent,
    const api::client::internal::Manager& api,
    const identifier::Nym& nymID,
    const ui::implementation::ActivityThreadRowID& rowID,
    const ui::implementation::ActivityThreadSortKey& sortKey,
    ui::implementation::CustomData& custom) noexcept
    -> std::shared_ptr<ui::implementation::ActivityThreadRowInternal>;
auto ProfileModel(
    const api::client::internal::Manager& api,
    const identifier::Nym& nymID,
    const SimpleCallback& cb) noexcept
    -> std::unique_ptr<ui::implementation::Profile>;
#if OT_QT
auto ProfileQtModel(ui::implementation::Profile& parent) noexcept
    -> std::unique_ptr<ui::ProfileQt>;
#endif
auto ProfileItemWidget(
    const ui::implementation::ProfileSubsectionInternalInterface& parent,
    const api::client::internal::Manager& api,
    const ui::implementation::ProfileSubsectionRowID& rowID,
    const ui::implementation::ProfileSubsectionSortKey& sortKey,
    ui::implementation::CustomData& custom) noexcept
    -> std::shared_ptr<ui::implementation::ProfileSubsectionRowInternal>;
auto ProfileSectionWidget(
    const ui::implementation::ProfileInternalInterface& parent,
    const api::client::internal::Manager& api,
    const ui::implementation::ProfileRowID& rowID,
    const ui::implementation::ProfileSortKey& key,
    ui::implementation::CustomData& custom) noexcept
    -> std::shared_ptr<ui::implementation::ProfileRowInternal>;
auto ProfileSubsectionWidget(
    const ui::implementation::ProfileSectionInternalInterface& parent,
    const api::client::internal::Manager& api,
    const ui::implementation::ProfileSectionRowID& rowID,
    const ui::implementation::ProfileSectionSortKey& key,
    ui::implementation::CustomData& custom) noexcept
    -> std::shared_ptr<ui::implementation::ProfileSectionRowInternal>;
auto UnitListItem(
    const ui::implementation::UnitListInternalInterface& parent,
    const api::client::internal::Manager& api,
    const ui::implementation::UnitListRowID& rowID,
    const ui::implementation::UnitListSortKey& sortKey,
    ui::implementation::CustomData& custom) noexcept
    -> std::shared_ptr<ui::implementation::UnitListRowInternal>;
auto UnitListModel(
    const api::client::internal::Manager& api,
    const identifier::Nym& nymID,
    const SimpleCallback& cb) noexcept
    -> std::unique_ptr<ui::implementation::UnitList>;
#if OT_QT
auto UnitListQtModel(ui::implementation::UnitList& parent) noexcept
    -> std::unique_ptr<ui::UnitListQt>;
#endif
}  // namespace opentxs::factory
