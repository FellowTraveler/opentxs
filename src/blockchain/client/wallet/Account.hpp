// Copyright (c) 2010-2021 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#pragma once

#include <memory>

#include "internal/api/client/blockchain/Blockchain.hpp"
#include "internal/blockchain/client/Client.hpp"
#include "opentxs/Types.hpp"
#include "opentxs/blockchain/Blockchain.hpp"
#include "opentxs/blockchain/FilterType.hpp"

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
struct BalanceTree;
}  // namespace internal
}  // namespace blockchain

namespace internal
{
struct Blockchain;
}  // namespace internal
}  // namespace client

class Core;
}  // namespace api

namespace blockchain
{
namespace client
{
namespace internal
{
struct Network;
struct WalletDatabase;
}  // namespace internal

namespace wallet
{
class SubchainStateData;
}  // namespace wallet
}  // namespace client
}  // namespace blockchain

namespace network
{
namespace zeromq
{
namespace socket
{
class Push;
}  // namespace socket
}  // namespace zeromq
}  // namespace network

class Outstanding;
}  // namespace opentxs

namespace opentxs::blockchain::client::wallet
{
class Account
{
public:
    using BalanceTree = api::client::blockchain::internal::BalanceTree;

    auto reorg(const block::Position& parent) noexcept -> bool;
    auto shutdown() noexcept -> void;
    auto state_machine() noexcept -> bool;

    Account(
        const api::Core& api,
        const api::client::internal::Blockchain& blockchain,
        const BalanceTree& ref,
        const internal::Network& network,
        const internal::WalletDatabase& db,
        const network::zeromq::socket::Push& threadPool,
        const filter::Type filter,
        Outstanding&& jobs,
        const SimpleCallback& taskFinished) noexcept;
    Account(Account&&) noexcept;

    ~Account();

private:
    struct Imp;

    std::unique_ptr<Imp> imp_;

    Account(const Account&) = delete;
    auto operator=(const Account&) -> Account& = delete;
    auto operator=(Account&&) -> Account& = delete;
};
}  // namespace opentxs::blockchain::client::wallet
