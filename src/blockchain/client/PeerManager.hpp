// Copyright (c) 2010-2019 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#pragma once

namespace zmq = opentxs::network::zeromq;

namespace opentxs::blockchain::client::implementation
{
class PeerManager final : virtual public internal::PeerManager,
                          public opentxs::internal::StateMachine
{
public:
    static const std::map<Type, std::uint16_t> default_port_map_;
    static const std::map<Type, p2p::Protocol> protocol_map_;

    bool AddPeer(const p2p::Address& address) const noexcept final;
    const internal::PeerDatabase& Database() const noexcept final
    {
        return database_;
    }
    bool Connect() noexcept { return Trigger(); }
    void Disconnect(const int id) const noexcept final;
    std::string Endpoint(const Task type) const noexcept final
    {
        return jobs_.Endpoint(type);
    }
    std::size_t GetPeerCount() const noexcept final { return peers_.Count(); }
    void Heartbeat() const noexcept { peers_.Heartbeat(); }
    void RequestFilters(
        const filter::Type type,
        const block::Height start,
        const block::Hash& stop) const noexcept final;
    void Shutdown() noexcept final;

    void init() noexcept final;

    ~PeerManager() final;

private:
    friend opentxs::Factory;

    struct IO {
        operator boost::asio::io_context&() noexcept { return context_; }

        void Shutdown() noexcept;

        IO() noexcept;

    private:
        boost::asio::io_context context_;
        std::unique_ptr<boost::asio::io_context::work> work_;
        boost::thread_group thread_pool_;
    };

    struct Jobs {
        std::string Endpoint(const Task type) const noexcept;

        void Dispatch(const Task type, zmq::Message& work) noexcept;
        void Shutdown() noexcept;

        Jobs(const api::internal::Core& api) noexcept;

    private:
        using EndpointMap = std::map<Task, std::string>;
        using SocketMap = std::map<Task, zmq::socket::Push*>;

        OTZMQPushSocket getcfilters_;
        const EndpointMap endpoint_map_;
        const SocketMap socket_map_;

        void listen(const Task type, const zmq::socket::Push& socket) noexcept;

        Jobs() = delete;
    };

    struct Peers {
        std::size_t Count() const noexcept;
        void Heartbeat() const noexcept;

        bool AddPeer(const p2p::Address& address) noexcept;
        void QueueDisconnect(const int id) noexcept;
        bool Run() noexcept;
        void Shutdown() noexcept;

        Peers(
            const api::internal::Core& api,
            const internal::Network& network,
            const internal::PeerDatabase& database,
            const internal::PeerManager& parent,
            const Type chain,
            const std::string& seednode,
            boost::asio::io_context& context) noexcept;

    private:
        using Endpoint = std::unique_ptr<p2p::internal::Address>;
        using Peer = std::unique_ptr<p2p::internal::Peer>;
        using DyingPeer = std::pair<Time, Peer>;

        const api::internal::Core& api_;
        const internal::Network& network_;
        const internal::PeerDatabase& database_;
        const internal::PeerManager& parent_;
        const Type chain_;
        const OTData default_peer_;
        boost::asio::io_context& context_;
        mutable std::mutex lock_;
        mutable std::mutex queue_lock_;
        std::atomic<int> next_id_;
        std::atomic<std::size_t> minimum_peers_;
        std::queue<int> disconnect_peers_;
        std::map<int, Peer> peers_;
        std::map<OTIdentifier, int> active_;
        std::queue<DyingPeer> dying_peers_;

        static OTData set_default_peer(const std::string node) noexcept;

        Endpoint get_peer() const noexcept;

        void add_peer(const Lock& lock, Endpoint endpoint) noexcept;
        p2p::internal::Peer* peer_factory(
            Endpoint endpoint,
            const int id) noexcept;
        void purge() noexcept;
    };

    const api::internal::Core& api_;
    const internal::PeerDatabase& database_;
    mutable std::mutex shutdown_;
    OTFlag running_;
    mutable IO io_context_;
    mutable Peers peers_;
    mutable Jobs jobs_;
    int heartbeat_task_;

    bool state_machine() noexcept;

    PeerManager(
        const api::internal::Core& api,
        const internal::Network& network,
        const internal::PeerDatabase& database,
        const Type chain,
        const std::string& seednode) noexcept;
    PeerManager() = delete;
    PeerManager(const PeerManager&) = delete;
    PeerManager(PeerManager&&) = delete;
    PeerManager& operator=(const PeerManager&) = delete;
    PeerManager& operator=(PeerManager&&) = delete;
};
}  // namespace opentxs::blockchain::client::implementation