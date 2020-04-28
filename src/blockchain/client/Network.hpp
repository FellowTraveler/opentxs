// Copyright (c) 2010-2020 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#pragma once

#include <atomic>
#include <future>
#include <iosfwd>
#include <memory>
#include <string>
#include <vector>

#include "core/Shutdown.hpp"
#include "core/Worker.hpp"
#include "internal/api/Api.hpp"
#include "internal/api/client/Client.hpp"
#include "internal/blockchain/Blockchain.hpp"
#include "internal/blockchain/client/Client.hpp"
#include "opentxs/Bytes.hpp"
#include "opentxs/Forward.hpp"
#include "opentxs/Types.hpp"
#include "opentxs/api/client/Manager.hpp"
#include "opentxs/blockchain/Blockchain.hpp"
#include "opentxs/blockchain/BlockchainType.hpp"
#include "opentxs/blockchain/FilterType.hpp"
#include "opentxs/blockchain/Network.hpp"
#include "opentxs/blockchain/Types.hpp"
#include "opentxs/core/Flag.hpp"
#include "opentxs/network/zeromq/ListenCallback.hpp"
#include "opentxs/network/zeromq/Message.hpp"
#include "opentxs/network/zeromq/Pipeline.hpp"
#include "opentxs/network/zeromq/socket/Publish.hpp"
#include "opentxs/network/zeromq/socket/Subscribe.hpp"
#include "opentxs/util/WorkType.hpp"
#include "util/Work.hpp"

namespace opentxs
{
namespace api
{
namespace client
{
namespace blockchain
{
class BalanceTree;
class PaymentCode;
}  // namespace blockchain
}  // namespace client

class Core;
}  // namespace api

namespace blockchain
{
namespace block
{
namespace bitcoin
{
class Block;
class Transaction;
}  // namespace bitcoin

class Header;
}  // namespace block

namespace p2p
{
class Address;
}  // namespace p2p
}  // namespace blockchain

namespace identifier
{
class Nym;
}  // namespace identifier

namespace network
{
namespace zeromq
{
namespace socket
{
class Publish;
}  // namespace socket

class Message;
}  // namespace zeromq
}  // namespace network
}  // namespace opentxs

namespace zmq = opentxs::network::zeromq;

namespace opentxs::blockchain::client::implementation
{
class Network : virtual public internal::Network,
                public Worker<Network, api::Core>
{
public:
    class SyncServer;

    enum class Work : OTZMQWorkType {
        shutdown = value(WorkType::Shutdown),
        heartbeat = OT_ZMQ_HEARTBEAT_SIGNAL,
        filter = OT_ZMQ_NEW_FILTER_SIGNAL,
        statemachine = OT_ZMQ_STATE_MACHINE_SIGNAL,
    };

    auto AddBlock(const std::shared_ptr<const block::bitcoin::Block> block)
        const noexcept -> bool final;
    auto AddPeer(const p2p::Address& address) const noexcept -> bool final;
    auto BlockOracle() const noexcept -> const internal::BlockOracle& final
    {
        return *block_p_;
    }
    auto Blockchain() const noexcept
        -> const api::client::internal::Blockchain& final
    {
        return blockchain_;
    }
    auto BroadcastTransaction(
        const block::bitcoin::Transaction& tx) const noexcept -> bool final;
    auto Chain() const noexcept -> Type final { return chain_; }
    auto FeeRate() const noexcept -> Amount final;
    auto FilterOracleInternal() const noexcept
        -> const internal::FilterOracle& final
    {
        return *filter_p_;
    }
    auto GetBalance() const noexcept -> Balance final
    {
        return database_.GetBalance();
    }
    auto GetBalance(const identifier::Nym& owner) const noexcept
        -> Balance final
    {
        return database_.GetBalance(owner);
    }
    auto GetConfirmations(const std::string& txid) const noexcept
        -> ChainHeight final;
    auto GetHeight() const noexcept -> ChainHeight final
    {
        return local_chain_height_.load();
    }
    auto GetPeerCount() const noexcept -> std::size_t final;
    auto GetType() const noexcept -> Type final { return chain_; }
    auto HeaderOracleInternal() const noexcept
        -> const internal::HeaderOracle& final
    {
        return header_;
    }
    auto Heartbeat() const noexcept -> void final
    {
        pipeline_->Push(MakeWork(Task::Heartbeat));
    }
    auto IsSynchronized() const noexcept -> bool final
    {
        return is_synchronized_headers();
    }
    auto JobReady(const internal::PeerManager::Task type) const noexcept
        -> void final;
    auto Listen(const p2p::Address& address) const noexcept -> bool final;
    auto Reorg() const noexcept -> const network::zeromq::socket::Publish& final
    {
        return parent_.Reorg();
    }
    auto RequestBlock(const block::Hash& block) const noexcept -> bool final;
    auto RequestBlocks(const std::vector<ReadView>& hashes) const noexcept
        -> bool final;
    auto SendToAddress(
        const opentxs::identifier::Nym& sender,
        const std::string& address,
        const Amount& amount,
        const std::string& memo) const noexcept -> PendingOutgoing final;
    auto Submit(network::zeromq::Message& work) const noexcept -> void final;
    auto UpdateHeight(const block::Height height) const noexcept -> void final;
    auto UpdateLocalHeight(const block::Position position) const noexcept
        -> void final;

    auto Connect() noexcept -> bool final;
    auto Disconnect() noexcept -> bool final;
    auto FilterOracleInternal() noexcept -> internal::FilterOracle& final
    {
        return filters_;
    }
    auto HeaderOracleInternal() noexcept -> internal::HeaderOracle& final
    {
        return header_;
    }
    auto Shutdown() noexcept -> std::shared_future<void> final
    {
        return stop_worker();
    }

    ~Network() override;

private:
    opentxs::internal::ShutdownSender shutdown_sender_;
    std::unique_ptr<blockchain::internal::Database> database_p_;
    const internal::Config& config_;
    std::unique_ptr<internal::HeaderOracle> header_p_;
    std::unique_ptr<internal::BlockOracle> block_p_;
    std::unique_ptr<internal::FilterOracle> filter_p_;
    std::unique_ptr<internal::PeerManager> peer_p_;
    std::unique_ptr<internal::Wallet> wallet_p_;

protected:
    const api::client::internal::Blockchain& blockchain_;
    const Type chain_;
    blockchain::internal::Database& database_;
    internal::FilterOracle& filters_;
    internal::HeaderOracle& header_;
    internal::PeerManager& peer_;
    internal::BlockOracle& block_;
    internal::Wallet& wallet_;

    // NOTE call init in every final constructor body
    auto init() noexcept -> void;

    Network(
        const api::Core& api,
        const api::client::internal::Blockchain& blockchain,
        const Type type,
        const internal::Config& config,
        const std::string& seednode,
        const std::string& syncEndpoint) noexcept;

private:
    friend Worker<Network, api::Core>;

    enum class State : int {
        UpdatingHeaders,
        UpdatingBlocks,
        UpdatingFilters,
        UpdatingSyncData,
        Normal
    };

    const api::client::internal::Blockchain& parent_;
    const std::string sync_endpoint_;
    std::unique_ptr<SyncServer> sync_server_;
    mutable std::atomic<block::Height> local_chain_height_;
    mutable std::atomic<block::Height> remote_chain_height_;
    OTFlag waiting_for_headers_;
    Time headers_requested_;
    Time headers_received_;
    std::atomic<State> state_;
    std::promise<void> init_promise_;
    std::shared_future<void> init_;

    static auto shutdown_endpoint() noexcept -> std::string;

    virtual auto instantiate_header(const ReadView payload) const noexcept
        -> std::unique_ptr<block::Header> = 0;
    auto is_synchronized_blocks() const noexcept -> bool;
    auto is_synchronized_filters() const noexcept -> bool;
    auto is_synchronized_headers() const noexcept -> bool;
    auto is_synchronized_sync_server() const noexcept -> bool;
    auto target() const noexcept -> block::Height;

    auto pipeline(zmq::Message& in) noexcept -> void;
    auto process_block(zmq::Message& in) noexcept -> void;
    auto process_filter_update(zmq::Message& in) noexcept -> void;
    auto process_header(zmq::Message& in) noexcept -> void;
    auto process_sync_data(zmq::Message& in) noexcept -> void;
    auto shutdown(std::promise<void>& promise) noexcept -> void;
    auto state_machine() noexcept -> bool;
    auto state_machine_headers() noexcept -> void;
    auto state_transition_blocks() noexcept -> void;
    auto state_transition_filters() noexcept -> void;
    auto state_transition_normal() noexcept -> void;
    auto state_transition_sync() noexcept -> void;

    Network() = delete;
    Network(const Network&) = delete;
    Network(Network&&) = delete;
    auto operator=(const Network&) -> Network& = delete;
    auto operator=(Network&&) -> Network& = delete;
};
}  // namespace opentxs::blockchain::client::implementation
