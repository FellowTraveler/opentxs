// Copyright (c) 2010-2020 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#pragma once

#include <memory>
#include <mutex>
#include <set>
#include <string>
#include <vector>

#include "internal/api/client/blockchain/Blockchain.hpp"
#include "opentxs/Bytes.hpp"
#include "opentxs/Proto.hpp"
#include "opentxs/Types.hpp"
#include "opentxs/api/client/blockchain/BalanceNode.hpp"
#include "opentxs/api/client/blockchain/BalanceTree.hpp"
#include "opentxs/api/client/blockchain/Types.hpp"
#include "opentxs/blockchain/BlockchainType.hpp"
#include "opentxs/blockchain/Types.hpp"
#include "opentxs/core/Data.hpp"
#include "opentxs/core/Identifier.hpp"
#include "opentxs/core/identifier/Nym.hpp"
#include "opentxs/crypto/Types.hpp"
#include "opentxs/protobuf/BlockchainActivity.pb.h"

namespace opentxs
{
namespace api
{
namespace client
{
namespace internal
{
struct Blockchain;
}  // namespace internal
}  // namespace client

namespace internal
{
struct Core;
}  // namespace internal
}  // namespace api

namespace crypto
{
namespace key
{
class EllipticCurve;
class HD;
}  // namespace key
}  // namespace crypto

namespace identifier
{
class Nym;
}  // namespace identifier

namespace proto
{
class AsymmetricKey;
}  // namespace proto

class PasswordPrompt;
}  // namespace opentxs

namespace opentxs::api::client::blockchain::implementation
{
class BalanceNode : virtual public internal::BalanceNode
{
public:
    auto AssociateTransaction(
        const std::vector<Activity>& unspent,
        const std::vector<Activity>& outgoing,
        std::set<OTIdentifier>& contacts,
        const PasswordPrompt& reason) const noexcept -> bool final;
    auto ID() const noexcept -> const Identifier& final { return id_; }
    auto IncomingTransactions(const Key& key) const noexcept
        -> std::set<std::string> final;
    auto Parent() const noexcept -> const internal::BalanceTree& final
    {
        return parent_;
    }
    auto PrivateKey(
        const Subchain type,
        const Bip32Index index,
        const PasswordPrompt& reason) const noexcept -> ECKey override
    {
        return {};
    }
    auto SetContact(
        const Subchain type,
        const Bip32Index index,
        const Identifier& id) noexcept(false) -> bool final;
    auto SetLabel(
        const Subchain type,
        const Bip32Index index,
        const std::string& label) noexcept(false) -> bool final;
    auto Type() const noexcept -> BalanceNodeType final { return type_; }
    auto UpdateElement(std::vector<ReadView>& pubkeyHashes) const noexcept
        -> void final;

    ~BalanceNode() override = default;

protected:
    struct Element final : virtual public internal::BalanceElement {
        auto Address(const AddressStyle format) const noexcept
            -> std::string final;
        auto Contact() const noexcept -> OTIdentifier final;
        auto Elements() const noexcept -> std::set<OTData> final;
        auto ID() const noexcept -> const Identifier& final
        {
            return parent_.ID();
        }
        auto IncomingTransactions() const noexcept
            -> std::set<std::string> final;
        auto Index() const noexcept -> Bip32Index final { return index_; }
        auto Key() const noexcept -> ECKey final;
        auto KeyID() const noexcept -> blockchain::Key final
        {
            return {ID().str(), subchain_, index_};
        }
        auto Label() const noexcept -> std::string final;
        auto NymID() const noexcept -> const identifier::Nym& final
        {
            return parent_.Parent().NymID();
        }
        auto Parent() const noexcept -> const blockchain::BalanceNode& final
        {
            return parent_;
        }
        auto PrivateKey(const PasswordPrompt& reason) const noexcept
            -> ECKey final
        {
            return parent_.PrivateKey(subchain_, index_, reason);
        }
        auto PubkeyHash() const noexcept -> OTData final;
        auto Serialize() const noexcept -> SerializedType final;
        auto Subchain() const noexcept -> blockchain::Subchain final
        {
            return subchain_;
        }

        auto elements(const Lock& lock) const noexcept -> std::set<OTData>;

        void SetContact(const Identifier& id) noexcept final;
        void SetLabel(const std::string& label) noexcept final;
        void SetMetadata(
            const Identifier& contact,
            const std::string& label) noexcept final;

        Element(
            const api::internal::Core& api,
            const client::internal::Blockchain& blockchain,
            const internal::BalanceNode& parent,
            const opentxs::blockchain::Type chain,
            const blockchain::Subchain subchain,
            const Bip32Index index,
            std::unique_ptr<opentxs::crypto::key::HD> key) noexcept(false);
        Element(
            const api::internal::Core& api,
            const client::internal::Blockchain& blockchain,
            const internal::BalanceNode& parent,
            const opentxs::blockchain::Type chain,
            const blockchain::Subchain subchain,
            const SerializedType& address) noexcept(false);
        ~Element() final = default;

    private:
        static const VersionNumber DefaultVersion{1};

        const api::internal::Core& api_;
        const client::internal::Blockchain& blockchain_;
        const internal::BalanceNode& parent_;
        const opentxs::blockchain::Type chain_;
        mutable std::mutex lock_;
        const VersionNumber version_;
        const blockchain::Subchain subchain_;
        const Bip32Index index_;
        std::string label_;
        OTIdentifier contact_;
        std::shared_ptr<opentxs::crypto::key::EllipticCurve> pkey_;
        opentxs::crypto::key::EllipticCurve& key_;

        static auto instantiate(
            const api::internal::Core& api,
            const proto::AsymmetricKey& serialized) noexcept(false)
            -> std::unique_ptr<opentxs::crypto::key::EllipticCurve>;

        auto update_element(Lock& lock) const noexcept -> void;

        Element(
            const api::internal::Core& api,
            const client::internal::Blockchain& blockchain,
            const internal::BalanceNode& parent,
            const opentxs::blockchain::Type chain,
            const VersionNumber version,
            const blockchain::Subchain subchain,
            const Bip32Index index,
            const std::string label,
            const OTIdentifier contact,
            std::unique_ptr<opentxs::crypto::key::EllipticCurve>
                key) noexcept(false);
        Element() = delete;
    };

    const api::internal::Core& api_;
    const internal::BalanceTree& parent_;
    const opentxs::blockchain::Type chain_;
    const BalanceNodeType type_;
    const OTIdentifier id_;
    mutable std::mutex lock_;
    mutable internal::ActivityMap unspent_;
    mutable internal::ActivityMap spent_;

    static auto convert(Activity&& in) noexcept -> proto::BlockchainActivity;
    static auto convert(const proto::BlockchainActivity& in) noexcept
        -> Activity;
    static auto convert(const std::vector<Activity>& in) noexcept
        -> internal::ActivityMap;

    void process_spent(
        const Lock& lock,
        const Coin& coin,
        const blockchain::Key key,
        const std::uint64_t value) const noexcept;
    void process_unspent(
        const Lock& lock,
        const Coin& coin,
        const blockchain::Key key,
        const Amount value) const noexcept;
    virtual auto save(const Lock& lock) const noexcept -> bool = 0;

    // NOTE call only from final constructor bodies
    void init() noexcept;
    virtual auto mutable_element(
        const Lock& lock,
        const Subchain type,
        const Bip32Index index) noexcept(false)
        -> internal::BalanceElement& = 0;

    BalanceNode(
        const api::internal::Core& api,
        const internal::BalanceTree& parent,
        const BalanceNodeType type,
        const OTIdentifier id,
        std::vector<Activity> unspent,
        std::vector<Activity> spent) noexcept;

private:
    static const VersionNumber ActivityVersion{1};

    virtual auto check_activity(
        const Lock& lock,
        const std::vector<Activity>& unspent,
        std::set<OTIdentifier>& contacts,
        const PasswordPrompt& reason) const noexcept -> bool = 0;

    BalanceNode() = delete;
    BalanceNode(const BalanceNode&) = delete;
    BalanceNode(BalanceNode&&) = delete;
    auto operator=(const BalanceNode&) -> BalanceNode& = delete;
    auto operator=(BalanceNode&&) -> BalanceNode& = delete;
};
}  // namespace opentxs::api::client::blockchain::implementation
