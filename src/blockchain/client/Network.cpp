// Copyright (c) 2010-2020 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include "0_stdafx.hpp"                   // IWYU pragma: associated
#include "1_Internal.hpp"                 // IWYU pragma: associated
#include "blockchain/client/Network.hpp"  // IWYU pragma: associated

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <iomanip>
#include <sstream>
#include <stdexcept>
#include <type_traits>
#include <utility>
#include <vector>

#include "blockchain/client/network/SyncServer.hpp"
#include "internal/blockchain/Params.hpp"
#include "internal/blockchain/client/Factory.hpp"
#include "opentxs/Pimpl.hpp"
#include "opentxs/Proto.tpp"
#include "opentxs/api/Core.hpp"
#include "opentxs/api/Endpoints.hpp"
#include "opentxs/api/Factory.hpp"
#include "opentxs/api/client/blockchain/AddressStyle.hpp"
#include "opentxs/blockchain/block/Header.hpp"
#include "opentxs/blockchain/block/bitcoin/Block.hpp"
#include "opentxs/blockchain/client/BlockOracle.hpp"
#include "opentxs/core/Data.hpp"
#include "opentxs/core/Identifier.hpp"
#include "opentxs/core/Log.hpp"
#include "opentxs/core/LogSource.hpp"
#include "opentxs/core/identifier/Nym.hpp"
#include "opentxs/network/zeromq/Frame.hpp"
#include "opentxs/network/zeromq/FrameSection.hpp"
#include "opentxs/network/zeromq/Message.hpp"
#include "opentxs/network/zeromq/Pipeline.hpp"
#include "opentxs/protobuf/BlockchainP2PChainState.pb.h"
#include "opentxs/protobuf/BlockchainP2PHello.pb.h"
#include "opentxs/protobuf/BlockchainTransactionProposal.pb.h"
#include "opentxs/protobuf/BlockchainTransactionProposedOutput.pb.h"

#define OT_METHOD "opentxs::blockchain::client::implementation::Network::"

namespace opentxs::blockchain::client::implementation
{
constexpr auto proposal_version_ = VersionNumber{1};
constexpr auto output_version_ = VersionNumber{1};

struct NullWallet final : public internal::Wallet {
    const api::Core& api_;

    auto ConstructTransaction(const proto::BlockchainTransactionProposal&)
        const noexcept -> std::future<block::pTxid> final
    {
        auto promise = std::promise<block::pTxid>{};
        promise.set_value(api_.Factory().Data());

        return promise.get_future();
    }
    auto Init() noexcept -> void final {}
    auto Shutdown() noexcept -> std::shared_future<void> final
    {
        auto promise = std::promise<void>{};
        promise.set_value();

        return promise.get_future();
    }

    NullWallet(const api::Core& api)
        : api_(api)
    {
    }
    NullWallet(const NullWallet&) = delete;
    NullWallet(NullWallet&&) = delete;
    auto operator=(const NullWallet&) -> NullWallet& = delete;
    auto operator=(NullWallet&&) -> NullWallet& = delete;

    ~NullWallet() final = default;
};

Network::Network(
    const api::Core& api,
    const api::client::internal::Blockchain& blockchain,
    const Type type,
    const internal::Config& config,
    const std::string& seednode,
    const std::string& syncEndpoint) noexcept
    : Worker(api, std::chrono::seconds(0))
    , shutdown_sender_(api.ZeroMQ(), shutdown_endpoint())
    , database_p_(factory::BlockchainDatabase(
          api,
          blockchain,
          *this,
          blockchain.BlockchainDB(),
          type))
    , config_(config)
    , header_p_(factory::HeaderOracle(api, *database_p_, type))
    , block_p_(factory::BlockOracle(
          api,
          *this,
          *header_p_,
          *database_p_,
          type,
          shutdown_sender_.endpoint_))
    , filter_p_(factory::BlockchainFilterOracle(
          api,
          blockchain,
          config_,
          *this,
          *header_p_,
          *block_p_,
          *database_p_,
          type,
          shutdown_sender_.endpoint_))
    , peer_p_(factory::BlockchainPeerManager(
          api,
          config_,
          *this,
          *header_p_,
          *filter_p_,
          *block_p_,
          *database_p_,
          blockchain.IO(),
          type,
          database_p_->BlockPolicy(),
          seednode,
          shutdown_sender_.endpoint_))
    , wallet_p_([&]() -> std::unique_ptr<blockchain::client::internal::Wallet> {
        if (config_.disable_wallet_) {

            return std::make_unique<NullWallet>(api);
        } else {
            return factory::BlockchainWallet(
                api,
                blockchain,
                *this,
                *database_p_,
                type,
                shutdown_sender_.endpoint_);
        }
    }())
    , blockchain_(blockchain)
    , chain_(type)
    , database_(*database_p_)
    , filters_(*filter_p_)
    , header_(*header_p_)
    , peer_(*peer_p_)
    , block_(*block_p_)
    , wallet_(*wallet_p_)
    , parent_(blockchain)
    , sync_endpoint_(syncEndpoint)
    , sync_server_([&] {
        if (config_.provide_sync_server_) {
            return std::make_unique<SyncServer>(
                api_,
                database_,
                header_,
                filters_,
                *this,
                chain_,
                filters_.DefaultType(),
                shutdown_sender_.endpoint_,
                sync_endpoint_);
        } else {
            return std::unique_ptr<SyncServer>{};
        }
    }())
    , local_chain_height_(0)
    , remote_chain_height_(params::Data::chains_.at(chain_).checkpoint_.height_)
    , waiting_for_headers_(Flag::Factory(false))
    , headers_requested_(Clock::now())
    , headers_received_()
    , state_(State::UpdatingHeaders)
    , init_promise_()
    , init_(init_promise_.get_future())
{
    OT_ASSERT(database_p_);
    OT_ASSERT(filter_p_);
    OT_ASSERT(header_p_);
    OT_ASSERT(peer_p_);
    OT_ASSERT(block_p_);
    OT_ASSERT(wallet_p_);

    database_.SetDefaultFilterType(filters_.DefaultType());
    header_.Init();
    init_executor({api_.Endpoints().InternalBlockchainFilterUpdated(chain_)});
    LogVerbose(config_.print()).Flush();
}

auto Network::AddBlock(
    const std::shared_ptr<const block::bitcoin::Block> pBlock) const noexcept
    -> bool
{
    if (!pBlock) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": invalid ")(DisplayString(chain_))(
            " block")
            .Flush();

        return false;
    }

    const auto& block = *pBlock;

    try {
        const auto bytes = [&] {
            auto output = Space{};

            if (false == block.Serialize(writer(output))) {
                throw std::runtime_error("Serialization error");
            }

            return output;
        }();
        block_.SubmitBlock(reader(bytes));
    } catch (...) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": failed to serialize ")(
            DisplayString(chain_))(" block")
            .Flush();

        return false;
    }

    const auto& id = block.ID();

    if (std::future_status::ready !=
        block_.LoadBitcoin(id).wait_for(std::chrono::seconds(10))) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": invalid ")(DisplayString(chain_))(
            " block")
            .Flush();

        return false;
    }

    if (false == filters_.ProcessBlock(block)) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": failed to index ")(
            DisplayString(chain_))(" block")
            .Flush();

        return false;
    }

    if (false == header_.AddHeader(block.Header().clone())) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": failed to process ")(
            DisplayString(chain_))(" header")
            .Flush();

        return false;
    }

    return peer_.BroadcastBlock(block);
}

auto Network::AddPeer(const p2p::Address& address) const noexcept -> bool
{
    if (false == running_.get()) { return false; }

    return peer_.AddPeer(address);
}

auto Network::BroadcastTransaction(
    const block::bitcoin::Transaction& tx) const noexcept -> bool
{
    if (false == running_.get()) { return false; }

    return peer_.BroadcastTransaction(tx);
}

auto Network::Connect() noexcept -> bool
{
    if (false == running_.get()) { return false; }

    return peer_.Connect();
}

auto Network::Disconnect() noexcept -> bool
{
    // TODO

    return false;
}

auto Network::FeeRate() const noexcept -> Amount
{
    // TODO the hardcoded default fee rate should be a fallback only
    // if there is no better data available.

    return params::Data::chains_.at(chain_).default_fee_rate_;
}

auto Network::GetConfirmations(const std::string& txid) const noexcept
    -> ChainHeight
{
    // TODO

    return -1;
}

auto Network::GetPeerCount() const noexcept -> std::size_t
{
    if (false == running_.get()) { return false; }

    return peer_.GetPeerCount();
}

auto Network::init() noexcept -> void
{
    local_chain_height_.store(header_.BestChain().first);

    {
        const auto best = database_.CurrentBest();

        OT_ASSERT(best);

        const auto position = best->Position();
        LogVerbose(DisplayString(chain_))(" chain initialized with best hash ")(
            position.second->asHex())(" at height ")(position.first)
            .Flush();
    }

    peer_.init();
    init_promise_.set_value();
    trigger();
}

auto Network::is_synchronized_blocks() const noexcept -> bool
{
    return block_.Tip().first >= this->target();
}

auto Network::is_synchronized_filters() const noexcept -> bool
{
    return filters_.Tip(filters_.DefaultType()).first >= this->target();
}

auto Network::is_synchronized_headers() const noexcept -> bool
{
    return local_chain_height_.load() >= remote_chain_height_.load();
}

auto Network::is_synchronized_sync_server() const noexcept -> bool
{
    if (sync_server_) {

        return sync_server_->Tip().first >= this->target();
    } else {

        return false;
    }
}

auto Network::JobReady(const internal::PeerManager::Task type) const noexcept
    -> void
{
    if (peer_p_) { peer_.JobReady(type); }
}

auto Network::Listen(const p2p::Address& address) const noexcept -> bool
{
    if (false == running_.get()) { return false; }

    return peer_.Listen(address);
}

auto Network::pipeline(zmq::Message& in) noexcept -> void
{
    if (false == running_.get()) { return; }

    init_.get();
    const auto body = in.Body();

    OT_ASSERT(0 < body.size());

    switch (body.at(0).as<Task>()) {
        case Task::SubmitBlock: {
            process_block(in);
        } break;
        case Task::SubmitBlockHeader: {
            process_header(in);
            [[fallthrough]];
        }
        case Task::StateMachine: {
            do_work();
        } break;
        case Task::FilterUpdate: {
            process_filter_update(in);
        } break;
        case Task::SyncData: {
            process_sync_data(in);
        } break;
        case Task::Heartbeat: {
            block_.Heartbeat();
            filters_.Heartbeat();
            peer_.Heartbeat();

            if (sync_server_) { sync_server_->Heartbeat(); }

            do_work();
        } break;
        case Task::Shutdown: {
            shutdown(shutdown_promise_);
        } break;
        default: {
            OT_FAIL;
        }
    }
}

auto Network::process_block(network::zeromq::Message& in) noexcept -> void
{
    if (false == running_.get()) { return; }

    const auto body = in.Body();

    if (2 > body.size()) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Invalid block").Flush();

        return;
    }

    block_.SubmitBlock(body.at(1).Bytes());
}

auto Network::process_filter_update(network::zeromq::Message& in) noexcept
    -> void
{
    if (false == running_.get()) { return; }

    const auto body = in.Body();

    OT_ASSERT(2 < body.size());

    const auto height = body.at(2).as<block::Height>();
    const auto target = this->target();

    {
        const auto progress =
            (0 == target) ? double{0}
                          : ((double(height) / double(target)) * double{100});
        auto display = std::stringstream{};
        display << std::setprecision(3) << progress << "%";

        if (false == config_.disable_wallet_) {
            LogNormal(DisplayString(chain_))(" chain sync progress: ")(height)(
                " of ")(target)(" (")(display.str())(")")
                .Flush();
        }
    }

    blockchain_.ReportProgress(chain_, height, target);
}

auto Network::process_header(network::zeromq::Message& in) noexcept -> void
{
    if (false == running_.get()) { return; }

    waiting_for_headers_->Off();
    headers_received_ = Clock::now();
    using Promise = std::promise<void>;
    auto pPromise = std::unique_ptr<Promise>{};
    auto input = std::vector<ReadView>{};

    {
        auto counter{-1};
        const auto body = in.Body();

        if (2 > body.size()) {
            LogOutput(OT_METHOD)(__FUNCTION__)(": Invalid message").Flush();

            return;
        }

        for (const auto& frame : body) {
            switch (++counter) {
                case 0: {
                    break;
                }
                case 1: {
                    pPromise.reset(
                        reinterpret_cast<Promise*>(frame.as<std::uintptr_t>()));
                } break;
                default: {
                    input.emplace_back(frame.Bytes());
                }
            }
        }
    }

    OT_ASSERT(pPromise);

    auto& promise = *pPromise;
    auto headers = std::vector<std::unique_ptr<block::Header>>{};

    for (const auto& header : input) {
        headers.emplace_back(instantiate_header(header));
    }

    if (false == headers.empty()) { header_.AddHeaders(headers); }

    promise.set_value();
}

auto Network::process_sync_data(network::zeromq::Message& in) noexcept -> void
{
    auto parsed = ParsedSyncData{api_.Factory().Data(), {}};

    {
        const auto hello =
            proto::Factory<proto::BlockchainP2PHello>(in.Body_at(1));
        auto found{false};

        for (const auto& state : hello.state()) {
            const auto chain = static_cast<Type>(state.chain());

            if (chain != chain_) { continue; }

            found = true;
            const auto height = static_cast<block::Height>(state.height());
            remote_chain_height_.store(
                std::max(height, remote_chain_height_.load()));
        }

        if (false == found) {
            LogOutput(OT_METHOD)(__FUNCTION__)(": Wrong chain").Flush();

            return;
        }
    }

    if (false == header_.ProcessSyncData(in, parsed)) { return; }

    LogVerbose(OT_METHOD)(__FUNCTION__)(": Accepted ")(parsed.second.size())(
        " of ")(in.Body().size() - 2)(" headers")
        .Flush();
    filters_.ProcessSyncData(parsed);
}

auto Network::RequestBlock(const block::Hash& block) const noexcept -> bool
{
    if (false == running_.get()) { return false; }

    return peer_.RequestBlock(block);
}

auto Network::RequestBlocks(const std::vector<ReadView>& hashes) const noexcept
    -> bool
{
    if (false == running_.get()) { return false; }

    return peer_.RequestBlocks(hashes);
}

auto Network::SendToAddress(
    const opentxs::identifier::Nym& sender,
    const std::string& address,
    const Amount& amount,
    const std::string& memo) const noexcept -> PendingOutgoing
{
    const auto [data, style, chains] = blockchain_.DecodeAddress(address);

    if (0 == chains.count(chain_)) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Address ")(address)(
            " not valid for ")(DisplayString(chain_))
            .Flush();

        return {};
    }

    auto id = api_.Factory().Identifier();
    id->Randomize(32);
    auto proposal = proto::BlockchainTransactionProposal{};
    proposal.set_version(proposal_version_);
    proposal.set_id(id->str());
    proposal.set_initiator(sender.str());
    proposal.set_expires(
        Clock::to_time_t(Clock::now() + std::chrono::hours(1)));
    proposal.set_memo(memo);
    using Style = api::client::blockchain::AddressStyle;
    auto& output = *proposal.add_output();
    output.set_version(output_version_);
    output.set_amount(amount);

    switch (style) {
        case Style::P2PKH: {
            output.set_pubkeyhash(data->str());
        } break;
        case Style::P2SH: {
            output.set_scripthash(data->str());
        } break;
        default: {
            LogOutput(OT_METHOD)(__FUNCTION__)(": Unsupported address type")
                .Flush();

            return {};
        }
    }

    return wallet_.ConstructTransaction(proposal);
}

auto Network::shutdown(std::promise<void>& promise) noexcept -> void
{
    init_.get();

    if (running_->Off()) {
        shutdown_sender_.Activate();
        wallet_.Shutdown().get();

        if (sync_server_) { sync_server_->Shutdown().get(); }

        peer_.Shutdown().get();
        filters_.Shutdown();
        block_.Shutdown().get();
        shutdown_sender_.Close();

        try {
            promise.set_value();
        } catch (...) {
        }
    }
}

auto Network::shutdown_endpoint() noexcept -> std::string
{
    return std::string{"inproc://"} + Identifier::Random()->str();
}

auto Network::state_machine() noexcept -> bool
{
    if (false == running_.get()) { return false; }

    LogDebug(OT_METHOD)(__FUNCTION__)(": Starting state machine for ")(
        DisplayString(chain_))
        .Flush();
    state_machine_headers();

    switch (state_.load()) {
        case State::UpdatingHeaders: {
            if (is_synchronized_headers()) {
                LogDetail(OT_METHOD)(__FUNCTION__)(": ")(DisplayString(chain_))(
                    " header oracle is synchronized")
                    .Flush();
                using Policy = api::client::blockchain::BlockStorage;

                if (Policy::All == database_.BlockPolicy()) {
                    state_transition_blocks();
                } else {
                    state_transition_filters();
                }
            } else {
                LogDebug(OT_METHOD)(__FUNCTION__)(": updating ")(
                    DisplayString(chain_))(" header oracle")
                    .Flush();
            }
        } break;
        case State::UpdatingBlocks: {
            if (is_synchronized_blocks()) {
                LogDetail(OT_METHOD)(__FUNCTION__)(": ")(DisplayString(chain_))(
                    " block oracle is synchronized")
                    .Flush();
                state_transition_filters();
            } else {
                LogDebug(OT_METHOD)(__FUNCTION__)(": updating ")(
                    DisplayString(chain_))(" block oracle")
                    .Flush();

                break;
            }
        } break;
        case State::UpdatingFilters: {
            if (is_synchronized_filters()) {
                LogDetail(OT_METHOD)(__FUNCTION__)(": ")(DisplayString(chain_))(
                    " filter oracle is synchronized")
                    .Flush();

                if (config_.provide_sync_server_) {
                    state_transition_sync();
                } else {
                    state_transition_normal();
                }
            } else {
                LogDebug(OT_METHOD)(__FUNCTION__)(": updating ")(
                    DisplayString(chain_))(" filter oracle")
                    .Flush();

                break;
            }
        } break;
        case State::UpdatingSyncData: {
            if (is_synchronized_sync_server()) {
                LogDetail(OT_METHOD)(__FUNCTION__)(": ")(DisplayString(chain_))(
                    " sync server is synchronized")
                    .Flush();
                state_transition_normal();
            } else {
                LogDebug(OT_METHOD)(__FUNCTION__)(": updating ")(
                    DisplayString(chain_))(" sync server")
                    .Flush();

                break;
            }
        } break;
        case State::Normal:
        default: {
        }
    }

    LogDebug(OT_METHOD)(__FUNCTION__)(": Completed state machine for ")(
        DisplayString(chain_))
        .Flush();

    return false;
}

auto Network::state_machine_headers() noexcept -> void
{
    constexpr auto limit = std::chrono::seconds{20};
    constexpr auto rateLimit = std::chrono::seconds{1};
    const auto interval = Clock::now() - headers_requested_;
    const auto requestHeaders = [&] {
        LogVerbose(OT_METHOD)(__FUNCTION__)(": Requesting ")(
            DisplayString(chain_))(" block headers from all connected peers")
            .Flush();
        waiting_for_headers_->On();
        peer_.RequestHeaders();
        headers_requested_ = Clock::now();
    };

    if (waiting_for_headers_.get()) {
        if (interval < limit) { return; }

        LogDetail(OT_METHOD)(__FUNCTION__)(": ")(DisplayString(chain_))(
            " headers not received before timeout")
            .Flush();
        requestHeaders();
    } else if ((false == is_synchronized_headers()) && (interval > rateLimit)) {
        requestHeaders();
    } else if ((Clock::now() - headers_received_) >= limit) {
        requestHeaders();
    }
}

auto Network::state_transition_blocks() noexcept -> void
{
    block_.Init();
    state_.store(State::UpdatingBlocks);
}

auto Network::state_transition_filters() noexcept -> void
{
    filters_.Start();
    state_.store(State::UpdatingFilters);
}

auto Network::state_transition_normal() noexcept -> void
{
    if (false == config_.disable_wallet_) { wallet_.Init(); }

    state_.store(State::Normal);
}

auto Network::state_transition_sync() noexcept -> void
{
    OT_ASSERT(sync_server_);

    sync_server_->Start();
    state_.store(State::UpdatingSyncData);
}

auto Network::Submit(network::zeromq::Message& work) const noexcept -> void
{
    if (false == running_.get()) { return; }

    pipeline_->Push(work);
}

auto Network::target() const noexcept -> block::Height
{
    return std::max(local_chain_height_.load(), remote_chain_height_.load());
}

auto Network::UpdateHeight(const block::Height height) const noexcept -> void
{
    if (false == running_.get()) { return; }

    remote_chain_height_.store(std::max(height, remote_chain_height_.load()));
    trigger();
}

auto Network::UpdateLocalHeight(const block::Position position) const noexcept
    -> void
{
    if (false == running_.get()) { return; }

    const auto& [height, hash] = position;
    LogNormal(DisplayString(chain_))(" block header chain updated to hash ")(
        hash->asHex())(" at height ")(height)
        .Flush();
    local_chain_height_.store(height);
    trigger();
}

Network::~Network() { Shutdown().get(); }
}  // namespace opentxs::blockchain::client::implementation
