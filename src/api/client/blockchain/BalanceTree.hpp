// Copyright (c) 2010-2020 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

// IWYU pragma: private
// IWYU pragma: friend ".*src/api/client/blockchain/BalanceTree.cpp"

#pragma once

#include <algorithm>
#include <iosfwd>
#include <map>
#include <memory>
#include <mutex>
#include <optional>
#include <set>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

#include "internal/api/Api.hpp"
#include "internal/api/client/blockchain/Blockchain.hpp"
#include "opentxs/Types.hpp"
#include "opentxs/api/client/blockchain/BalanceNode.hpp"
#include "opentxs/api/client/blockchain/BalanceTree.hpp"
#include "opentxs/api/client/blockchain/Types.hpp"
#include "opentxs/blockchain/BlockchainType.hpp"
#include "opentxs/core/Identifier.hpp"
#include "opentxs/core/Log.hpp"
#include "opentxs/core/LogSource.hpp"
#include "opentxs/core/identifier/Nym.hpp"

namespace opentxs
{
namespace api
{
namespace internal
{
struct Core;
}  // namespace internal
}  // namespace api

namespace proto
{
class HDPath;
}  // namespace proto

class PasswordPrompt;
}  // namespace opentxs

namespace opentxs::api::client::blockchain::implementation
{
class BalanceTree final : public internal::BalanceTree
{
public:
    auto AssociateTransaction(
        const std::vector<Activity>& unspent,
        const std::vector<Activity>& spent,
        std::set<OTIdentifier>& contacts,
        const PasswordPrompt& reason) const noexcept -> bool final;
    auto Chain() const noexcept -> opentxs::blockchain::Type final
    {
        return chain_;
    }
    void ClaimAccountID(const std::string& id, internal::BalanceNode* node)
        const noexcept final;
    auto GetDepositAddress(const std::string& memo) const noexcept
        -> std::string final;
    auto GetDepositAddress(const Identifier& contact, const std::string& memo)
        const noexcept -> std::string final;
    auto GetHD() const noexcept -> const HDAccounts& final { return hd_; }
    auto GetImported() const noexcept -> const ImportedAccounts& final
    {
        return imported_;
    }
    auto GetNextChangeKey(const PasswordPrompt& reason) const noexcept(false)
        -> const Element& final;
    auto GetNextDepositKey(const PasswordPrompt& reason) const noexcept(false)
        -> const Element& final;
    auto GetPaymentCode() const noexcept -> const PaymentCodeAccounts& final
    {
        return payment_code_;
    }
    auto HDChain(const Identifier& account) const noexcept(false)
        -> const blockchain::internal::HD& final
    {
        return hd_.at(account);
    }
    auto LookupUTXO(const Coin& coin) const noexcept
        -> std::optional<std::pair<Key, std::uint64_t>> final;
    auto Node(const Identifier& id) const noexcept(false)
        -> internal::BalanceNode& final;
    auto NymID() const noexcept -> const identifier::Nym& final
    {
        return nym_id_;
    }
    auto Parent() const noexcept -> const internal::BalanceList& final
    {
        return parent_;
    }

    auto AddHDNode(const proto::HDPath& path, Identifier& id) noexcept
        -> bool final
    {
        return hd_.Construct(path, id);
    }

    BalanceTree(
        const api::internal::Core& api,
        const internal::BalanceList& parent,
        const identifier::Nym& nym,
        const std::set<OTIdentifier>& accounts) noexcept;

    ~BalanceTree() final = default;

private:
    template <typename InterfaceType, typename PayloadType>
    class NodeGroup final : virtual public InterfaceType
    {
    public:
        using const_iterator = typename InterfaceType::const_iterator;
        using value_type = typename InterfaceType::value_type;

        auto at(const std::size_t position) const -> const value_type& final
        {
            Lock lock(lock_);

            return *nodes_.at(position);
        }
        auto at(const Identifier& id) const -> const PayloadType& final
        {
            Lock lock(lock_);

            return *nodes_.at(index_.at(id));
        }
        auto begin() const noexcept -> const_iterator final
        {
            return const_iterator(this, 0);
        }
        auto cbegin() const noexcept -> const_iterator final
        {
            return const_iterator(this, 0);
        }
        auto cend() const noexcept -> const_iterator final
        {
            return const_iterator(this, nodes_.size());
        }
        auto end() const noexcept -> const_iterator final
        {
            return const_iterator(this, nodes_.size());
        }
        auto size() const noexcept -> std::size_t final
        {
            return nodes_.size();
        }

        auto at(const std::size_t position) -> value_type&
        {
            Lock lock(lock_);

            return *nodes_.at(position);
        }
        auto at(const Identifier& id) -> PayloadType&
        {
            Lock lock(lock_);

            return *nodes_.at(index_.at(id));
        }
        template <typename ArgumentType>
        auto Construct(const ArgumentType& data, Identifier& id) noexcept
            -> bool
        {
            Lock lock(lock_);

            return construct(lock, data, id);
        }

        NodeGroup(const api::internal::Core& api, BalanceTree& parent) noexcept
            : api_(api)
            , parent_(parent)
            , lock_()
            , nodes_()
            , index_()
        {
        }

    private:
        const api::internal::Core& api_;
        BalanceTree& parent_;
        mutable std::mutex lock_;
        std::vector<std::unique_ptr<PayloadType>> nodes_;
        std::map<OTIdentifier, std::size_t> index_;

        auto add(
            const Lock& lock,
            const Identifier& id,
            std::unique_ptr<PayloadType> node) noexcept -> bool;
        template <typename ArgumentType>
        auto construct(
            const Lock& lock,
            const ArgumentType& data,
            Identifier& id) noexcept -> bool
        {
            auto node{Factory<PayloadType, ArgumentType>::get(
                api_, parent_, data, id)};

            if (false == bool(node)) { return false; }

            if (0 < index_.count(id)) {
                LogOutput("Blockchain account ")(id)(" already exists").Flush();

                return false;
            }

            return add(lock, id, std::move(node));
        }
    };

    template <typename ReturnType, typename ArgumentType>
    struct Factory {
        static auto get(
            const api::internal::Core& api,
            const BalanceTree& parent,
            const ArgumentType& data,
            Identifier& id) noexcept -> std::unique_ptr<ReturnType>;
    };

    struct NodeIndex {
        auto Find(const std::string& id) const noexcept
            -> internal::BalanceNode*;

        void Add(const std::string& id, internal::BalanceNode* node) noexcept;

        NodeIndex() noexcept
            : lock_()
            , index_()
        {
        }

    private:
        mutable std::mutex lock_;
        std::map<std::string, internal::BalanceNode*> index_;
    };

    using HDNodes = NodeGroup<HDAccounts, internal::HD>;
    using ImportedNodes = NodeGroup<ImportedAccounts, internal::Imported>;
    using PaymentCodeNodes =
        NodeGroup<PaymentCodeAccounts, internal::PaymentCode>;

    const api::internal::Core& api_;
    const internal::BalanceList& parent_;
    const opentxs::blockchain::Type chain_;
    const OTNymID nym_id_;
    HDNodes hd_;
    ImportedNodes imported_;
    PaymentCodeNodes payment_code_;
    mutable NodeIndex node_index_;
    mutable std::mutex lock_;
    mutable internal::ActivityMap unspent_;
    mutable internal::ActivityMap spent_;

    void init(const std::set<OTIdentifier>& HDAccounts) noexcept;

    auto find_best_deposit_address() const noexcept -> const Element&;
    auto find_next_element(Subchain subchain, const PasswordPrompt& reason)
        const noexcept(false) -> const Element&;

    BalanceTree() = delete;
    BalanceTree(const BalanceTree&) = delete;
    BalanceTree(BalanceTree&&) = delete;
    auto operator=(const BalanceTree&) -> BalanceTree& = delete;
    auto operator=(BalanceTree&&) -> BalanceTree& = delete;
};
}  // namespace opentxs::api::client::blockchain::implementation
