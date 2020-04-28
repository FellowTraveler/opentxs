// Copyright (c) 2010-2020 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#pragma once

#include <cstdint>
#include <functional>
#include <iosfwd>
#include <map>
#include <memory>
#include <optional>
#include <set>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

#include "internal/blockchain/client/Client.hpp"
#include "internal/blockchain/p2p/P2P.hpp"
#include "opentxs/Bytes.hpp"
#include "opentxs/Types.hpp"
#include "opentxs/Version.hpp"
#include "opentxs/api/client/blockchain/BalanceList.hpp"
#include "opentxs/api/client/blockchain/BalanceNode.hpp"
#include "opentxs/api/client/blockchain/BalanceTree.hpp"
#include "opentxs/api/client/blockchain/Deterministic.hpp"
#include "opentxs/api/client/blockchain/HD.hpp"
#include "opentxs/api/client/blockchain/Imported.hpp"
#include "opentxs/api/client/blockchain/PaymentCode.hpp"
#include "opentxs/api/client/blockchain/Types.hpp"
#include "opentxs/blockchain/Blockchain.hpp"
#include "opentxs/blockchain/BlockchainType.hpp"
#include "opentxs/blockchain/FilterType.hpp"
#include "opentxs/blockchain/Types.hpp"
#include "opentxs/blockchain/p2p/Types.hpp"
#include "opentxs/core/Data.hpp"
#include "opentxs/core/Identifier.hpp"
#include "opentxs/crypto/Types.hpp"
#include "opentxs/protobuf/BlockchainAddress.pb.h"

namespace std
{
using COIN = std::tuple<std::string, std::size_t, std::int64_t>;

template <>
struct less<COIN> {
    auto operator()(const COIN& lhs, const COIN& rhs) const -> bool
    {
        /* TODO: these lines will cause a segfault in the clang ast parser.
                const auto & [ lID, lIndex, lAmount ] = lhs;
                const auto & [ rID, rIndex, rAmount ] = rhs;
        */

        const auto& lID = std::get<0>(lhs);
        const auto& lIndex = std::get<1>(lhs);
        const auto& lAmount = std::get<2>(lhs);
        const auto& rID = std::get<0>(rhs);
        const auto& rIndex = std::get<1>(rhs);
        const auto& rAmount = std::get<2>(rhs);

        if (lID < rID) { return true; }

        if (rID < lID) { return false; }

        if (lIndex < rIndex) { return true; }

        if (rIndex < lIndex) { return false; }

        return (lAmount < rAmount);
    }
};
}  // namespace std

namespace opentxs
{
namespace api
{
namespace client
{
namespace blockchain
{
namespace internal
{
struct BalanceList;
struct BalanceNode;
struct BalanceTree;
struct Deterministic;
struct HD;
struct Imported;
struct PaymentCode;
}  // namespace internal
}  // namespace blockchain

namespace internal
{
struct Blockchain;
}  // namespace internal
}  // namespace client

namespace internal
{
struct Core;
}  // namespace internal

class Crypto;
}  // namespace api

namespace identifier
{
class Nym;
}  // namespace identifier

namespace proto
{
class HDPath;
}  // namespace proto

class Identifier;
class PasswordPrompt;
}  // namespace opentxs

namespace opentxs
{
auto blockchain_thread_item_id(
    const api::Crypto& crypto,
    const opentxs::blockchain::Type chain,
    const Data& txid) noexcept -> OTIdentifier;
}  // namespace opentxs

#if OT_BLOCKCHAIN
namespace opentxs::api::client::blockchain
{
using Address = opentxs::blockchain::p2p::internal::Address;
using Address_p = std::unique_ptr<Address>;
using Chain = opentxs::blockchain::Type;
using FilterData =
    opentxs::blockchain::client::internal::FilterDatabase::Filter;
using FilterHash = opentxs::blockchain::client::internal::FilterDatabase::Hash;
using FilterHeader =
    opentxs::blockchain::client::internal::FilterDatabase::Header;
using FilterType = opentxs::blockchain::filter::Type;
using Position = opentxs::blockchain::block::Position;
using Protocol = opentxs::blockchain::p2p::Protocol;
using Service = opentxs::blockchain::p2p::Service;
using Type = opentxs::blockchain::p2p::Network;
using UpdatedHeader = opentxs::blockchain::client::UpdatedHeader;
using SyncTableData = std::pair<int, std::string>;

enum Table {
    BlockHeaders = 0,
    PeerDetails = 1,
    PeerChainIndex = 2,
    PeerProtocolIndex = 3,
    PeerServiceIndex = 4,
    PeerNetworkIndex = 5,
    PeerConnectedIndex = 6,
    FiltersBasic = 7,
    FiltersBCH = 8,
    FiltersOpentxs = 9,
    FilterHeadersBasic = 10,
    FilterHeadersBCH = 11,
    FilterHeadersOpentxs = 12,
    Config = 13,
    BlockIndex = 14,
    Enabled = 15,
    SyncTips = 16,
};

auto ChainToSyncTable(const Chain chain) noexcept(false) -> int;
auto SyncTables() noexcept -> const std::vector<SyncTableData>&;
}  // namespace opentxs::api::client::blockchain
#endif  // OT_BLOCKCHAIN

namespace opentxs::api::client::blockchain::internal
{
    using ActivityMap = std::map<Coin, std::pair<blockchain::Key, std::uint64_t>>;

struct BalanceList : virtual public blockchain::BalanceList {
    virtual auto AddHDNode(
        const identifier::Nym& nym,
        const proto::HDPath& path,
        Identifier& id) noexcept -> bool = 0;
    virtual auto Nym(const identifier::Nym& id) noexcept -> BalanceTree& = 0;
    virtual auto Parent() const noexcept
        -> const client::internal::Blockchain& = 0;
};

struct BalanceElement : virtual public blockchain::BalanceNode::Element {
    using SerializedType = proto::BlockchainAddress;

    virtual auto Elements() const noexcept -> std::set<OTData> = 0;
    virtual auto ID() const noexcept -> const Identifier& = 0;
    virtual auto IncomingTransactions() const noexcept
        -> std::set<std::string> = 0;
    virtual auto NymID() const noexcept -> const identifier::Nym& = 0;
    virtual auto Serialize() const noexcept -> SerializedType = 0;

    virtual void SetContact(const Identifier& id) noexcept = 0;
    virtual void SetLabel(const std::string& label) noexcept = 0;
    virtual void SetMetadata(
        const Identifier& contact,
        const std::string& label) noexcept = 0;
};

struct BalanceNode : virtual public blockchain::BalanceNode {
    virtual auto AssociateTransaction(
        const std::vector<Activity>& unspent,
        const std::vector<Activity>& spent,
        std::set<OTIdentifier>& contacts,
        const PasswordPrompt& reason) const noexcept -> bool = 0;
    virtual auto BalanceElement(const Subchain type, const Bip32Index index)
        const noexcept(false) -> const internal::BalanceElement& override = 0;
    virtual auto IncomingTransactions(const Key& key) const noexcept
        -> std::set<std::string> = 0;
    virtual auto PrivateKey(
        const Subchain type,
        const Bip32Index index,
        const PasswordPrompt& reason) const noexcept -> ECKey = 0;

    virtual auto SetContact(
        const Subchain type,
        const Bip32Index index,
        const Identifier& id) noexcept(false) -> bool = 0;
    virtual auto SetLabel(
        const Subchain type,
        const Bip32Index index,
        const std::string& label) noexcept(false) -> bool = 0;
    virtual auto UpdateElement(
        std::vector<ReadView>& pubkeyHashes) const noexcept -> void = 0;
};

struct BalanceTree : virtual public blockchain::BalanceTree {
    virtual auto AssociateTransaction(
        const std::vector<Activity>& unspent,
        const std::vector<Activity>& spent,
        std::set<OTIdentifier>& contacts,
        const PasswordPrompt& reason) const noexcept -> bool = 0;
    virtual auto Chain() const noexcept -> opentxs::blockchain::Type = 0;
    virtual void ClaimAccountID(
        const std::string& id,
        internal::BalanceNode* node) const noexcept = 0;
    virtual auto LookupUTXO(const Coin& coin) const noexcept
        -> std::optional<std::pair<Key, OTAmount>> = 0;
    virtual auto HDChain(const Identifier& account) const noexcept(false)
        -> const blockchain::internal::HD& = 0;
    virtual auto Parent() const noexcept
        -> const internal::BalanceList& override = 0;

    virtual auto AddHDNode(const proto::HDPath& path, Identifier& id) noexcept
        -> bool = 0;
    virtual auto Node(const Identifier& id) const noexcept(false)
        -> internal::BalanceNode& = 0;
};

struct Deterministic : virtual public blockchain::Deterministic,
                       virtual public BalanceNode {
};

struct HD : virtual public blockchain::HD, virtual public Deterministic {
};

struct Imported : virtual public blockchain::Imported,
                  virtual public BalanceNode {
};

struct PaymentCode : virtual public blockchain::PaymentCode,
                     virtual public Deterministic {
};
}  // namespace opentxs::api::client::blockchain::internal
