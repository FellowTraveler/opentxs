// Copyright (c) 2010-2021 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include "0_stdafx.hpp"                            // IWYU pragma: associated
#include "1_Internal.hpp"                          // IWYU pragma: associated
#include "blockchain/client/wallet/Proposals.hpp"  // IWYU pragma: associated

#include <chrono>
#include <deque>
#include <functional>
#include <map>
#include <mutex>
#include <optional>
#include <set>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

#include "blockchain/client/wallet/BitcoinTransactionBuilder.hpp"
#include "internal/api/client/blockchain/Blockchain.hpp"
#include "internal/blockchain/block/bitcoin/Bitcoin.hpp"
#include "internal/blockchain/client/Client.hpp"
#include "opentxs/Pimpl.hpp"
#include "opentxs/Types.hpp"
#include "opentxs/api/Core.hpp"
#include "opentxs/api/Factory.hpp"
#include "opentxs/api/client/Blockchain.hpp"
#include "opentxs/api/client/blockchain/PaymentCode.hpp"
#include "opentxs/blockchain/BlockchainType.hpp"
#include "opentxs/core/Identifier.hpp"
#include "opentxs/core/Log.hpp"
#include "opentxs/core/LogSource.hpp"
#include "opentxs/core/crypto/PaymentCode.hpp"
#include "opentxs/core/identifier/Nym.hpp"
#include "opentxs/protobuf/BlockchainTransaction.pb.h"
#include "opentxs/protobuf/BlockchainTransactionProposal.pb.h"
#include "opentxs/protobuf/BlockchainTransactionProposedNotification.pb.h"
#include "util/ScopeGuard.hpp"

#define OT_METHOD "opentxs::blockchain::client::wallet::Proposals::"

namespace opentxs::blockchain::client::wallet
{
struct Proposals::Imp {
public:
    auto Add(const Proposal& tx) const noexcept -> std::future<block::pTxid>
    {
        auto id = api_.Factory().Identifier(tx.id());

        if (false == db_.AddProposal(id, tx)) {
            LogOutput(OT_METHOD)(__FUNCTION__)(": Database error").Flush();

            return {};
        }

        return pending_.Add(std::move(id));
    }
    auto Run() noexcept -> bool
    {
        Lock lock{lock_};
        send(lock);
        rebroadcast(lock);
        cleanup(lock);

        return pending_.HasData();
    }

    Imp(const api::Core& api,
        const api::client::Blockchain& blockchain,
        const internal::Network& network,
        const internal::WalletDatabase& db,
        const Type chain) noexcept
        : api_(api)
        , blockchain_(blockchain)
        , network_(network)
        , db_(db)
        , chain_(chain)
        , lock_()
        , pending_()
        , confirming_()
    {
        for (const auto& serialized : db_.LoadProposals()) {
            auto id = api_.Factory().Identifier(serialized.id());

            if (serialized.has_finished()) {
                confirming_.emplace(std::move(id), Time{});
            } else {
                pending_.Add(std::move(id));
            }
        }
    }

private:
    enum class BuildResult : std::int8_t {
        PermanentFailure = -1,
        Success = 0,
        TemporaryFailure = 1,
    };

    using Builder = std::function<BuildResult(
        const Identifier& id,
        Proposal&,
        std::promise<block::pTxid>&)>;
    using Promise = std::promise<block::pTxid>;
    using Future = std::future<block::pTxid>;
    using Data = std::pair<OTIdentifier, Promise>;

    struct Pending {
        auto Exists(const Identifier& id) const noexcept -> bool
        {
            auto lock = Lock{lock_};

            return 0 < ids_.count(id);
        }
        auto HasData() const noexcept -> bool
        {
            auto lock = Lock{lock_};

            return 0 < data_.size();
        }

        auto Add(OTIdentifier&& id) noexcept -> Future
        {
            auto lock = Lock{lock_};

            if (0 < ids_.count(id)) {
                LogOutput(OT_METHOD)(__FUNCTION__)(": Proposal already exists")
                    .Flush();

                return {};
            }

            ids_.emplace(id);
            data_.emplace_back(std::move(id), Promise{});

            return data_.back().second.get_future();
        }
        auto Add(Data&& job) noexcept -> void
        {
            auto lock = Lock{lock_};
            const auto& [id, promise] = job;

            OT_ASSERT(0 == ids_.count(id));

            ids_.emplace(id);
            data_.emplace_back(std::move(job));
        }
        auto Delete(const Identifier& id) noexcept -> void
        {
            auto lock = Lock{lock_};
            auto copy = OTIdentifier{id};

            if (0 < ids_.count(copy)) {
                ids_.erase(copy);

                for (auto i{data_.begin()}; i != data_.end();) {
                    if (i->first == copy) {
                        i = data_.erase(i);
                    } else {
                        ++i;
                    }
                }
            }
        }
        auto Pop() noexcept -> Data
        {
            auto lock = Lock{lock_};
            auto post = ScopeGuard{[&] { data_.pop_front(); }};

            return std::move(data_.front());
        }

    private:
        mutable std::mutex lock_{};
        std::deque<Data> data_{};
        std::set<OTIdentifier> ids_{};
    };

    const api::Core& api_;
    const api::client::Blockchain& blockchain_;
    const internal::Network& network_;
    const internal::WalletDatabase& db_;
    const Type chain_;
    mutable std::mutex lock_;
    mutable Pending pending_;
    mutable std::map<OTIdentifier, Time> confirming_;

    static auto is_expired(const Proposal& tx) noexcept -> bool
    {
        return Clock::now() > Clock::from_time_t(tx.expires());
    }

    auto build_transaction_bitcoin(
        const Identifier& id,
        Proposal& proposal,
        std::promise<block::pTxid>& promise) const noexcept -> BuildResult
    {
        auto output = BuildResult::Success;
        auto builder = BitcoinTransactionBuilder{
            api_, blockchain_, db_, id, proposal, chain_, network_.FeeRate()};
        auto post = ScopeGuard{[&] {
            if (BuildResult::PermanentFailure != output) { return; }

            builder.ReleaseKeys();
        }};

        if (false == builder.CreateOutputs(proposal)) {
            LogOutput(OT_METHOD)(__FUNCTION__)(": Failed to create outputs")
                .Flush();
            output = BuildResult::PermanentFailure;

            return output;
        }

        if (false == builder.AddChange(proposal)) {
            LogOutput(OT_METHOD)(__FUNCTION__)(
                ": Failed to allocate change output")
                .Flush();
            output = BuildResult::PermanentFailure;

            return output;
        }

        while (false == builder.IsFunded()) {
            using Spend = internal::WalletDatabase::Spend;
            auto utxo =
                db_.ReserveUTXO(builder.Spender(), id, Spend::ConfirmedOnly);

            if (false == utxo.has_value()) {
                LogOutput(OT_METHOD)(__FUNCTION__)(": Insufficient funds")
                    .Flush();
                output = BuildResult::PermanentFailure;

                return output;
            }

            if (false == builder.AddInput(utxo.value())) {
                LogOutput(OT_METHOD)(__FUNCTION__)(": Failed to add input")
                    .Flush();
                output = BuildResult::PermanentFailure;

                return output;
            }
        }

        OT_ASSERT(builder.IsFunded());

        builder.FinalizeOutputs();

        if (false == builder.SignInputs()) {
            LogOutput(OT_METHOD)(__FUNCTION__)(": Failed to sign inputs")
                .Flush();
            output = BuildResult::PermanentFailure;

            return output;
        }

        auto pTransaction = builder.FinalizeTransaction();

        if (false == bool(pTransaction)) {
            LogOutput(OT_METHOD)(__FUNCTION__)(
                ": Failed to instantiate transaction")
                .Flush();
            output = BuildResult::PermanentFailure;

            return output;
        }

        const auto& transaction = *pTransaction;
        auto proto = transaction.Serialize(blockchain_);

        if (false == proto.has_value()) {
            LogOutput(OT_METHOD)(__FUNCTION__)(
                ": Failed to serialize transaction")
                .Flush();
            output = BuildResult::PermanentFailure;

            return output;
        }

        *proposal.mutable_finished() = proto.value();

        if (!db_.AddProposal(id, proposal)) {
            LogOutput(OT_METHOD)(__FUNCTION__)(": Database error (proposal)")
                .Flush();
            output = BuildResult::PermanentFailure;

            return output;
        }

        if (!db_.AddOutgoingTransaction(id, proposal, transaction)) {
            LogOutput(OT_METHOD)(__FUNCTION__)(": Database error (transaction)")
                .Flush();
            output = BuildResult::PermanentFailure;

            return output;
        }

        promise.set_value(transaction.ID());
        const auto sent = network_.BroadcastTransaction(transaction);

        try {
            if (false == sent) {
                throw std::runtime_error{"Failed to send tx"};
            }

            for (const auto& notif : proposal.notification()) {
                using PC = api::client::blockchain::internal::PaymentCode;
                const auto accountID = PC::GetID(
                    api_,
                    chain_,
                    api_.Factory().PaymentCode(notif.sender()),
                    api_.Factory().PaymentCode(notif.recipient()));
                const auto nymID = [&] {
                    auto out = api_.Factory().NymID();
                    const auto& bytes = proposal.initiator();
                    out->Assign(bytes.data(), bytes.size());

                    OT_ASSERT(false == out->empty());

                    return out;
                }();
                const auto& account =
                    blockchain_.PaymentCodeSubaccount(nymID, accountID);
                account.AddNotification(transaction.ID());
            }
        } catch (const std::exception& e) {
            LogOutput(OT_METHOD)(__FUNCTION__)(": ")(e.what()).Flush();
        }

        return output;
    }
    auto cleanup(const Lock& lock) noexcept -> void
    {
        const auto finished = db_.CompletedProposals();

        for (const auto& id : finished) {
            pending_.Delete(id);
            confirming_.erase(id);
        }

        db_.ForgetProposals(finished);
    }
    auto get_builder() const noexcept -> Builder
    {
        switch (chain_) {
            case Type::Bitcoin:
            case Type::Bitcoin_testnet3:
            case Type::BitcoinCash:
            case Type::BitcoinCash_testnet3:
            case Type::Litecoin:
            case Type::Litecoin_testnet4:
            case Type::PKT:
            case Type::PKT_testnet:
            case Type::UnitTest: {

                return [this](
                           const Identifier& id,
                           Proposal& in,
                           std::promise<block::pTxid>& out) -> auto
                {
                    return build_transaction_bitcoin(id, in, out);
                };
            }
            case Type::Unknown:
            case Type::Ethereum_frontier:
            case Type::Ethereum_ropsten:
            default: {
                LogOutput(OT_METHOD)(__FUNCTION__)(": Unsupported chain")
                    .Flush();

                return {};
            }
        }
    }
    auto rebroadcast(const Lock& lock) noexcept -> void
    {
        // TODO monitor inv messages from peers and wait until a peer
        // we didn't transmit the transaction to tells us about it
        constexpr auto limit = std::chrono::minutes(1);

        for (auto& [id, time] : confirming_) {
            if ((Clock::now() - time) < limit) { continue; }

            auto proposal = db_.LoadProposal(id);

            if (false == proposal.has_value()) { continue; }

            auto pTx = factory::BitcoinTransaction(
                api_, blockchain_, proposal.value().finished());

            if (false == bool(pTx)) { continue; }

            const auto& tx = *pTx;
            network_.BroadcastTransaction(tx);
        }
    }
    auto send(const Lock& lock) noexcept -> void
    {
        if (false == pending_.HasData()) { return; }

        auto job = pending_.Pop();
        auto& [id, promise] = job;
        auto wipe{false};
        auto erase{false};
        auto loop = ScopeGuard{[&]() {
            const auto& [id, promise] = job;

            if (wipe) {
                db_.CancelProposal(id);
                erase = true;
            }

            if (false == erase) { pending_.Add(std::move(job)); }
        }};
        auto serialized = db_.LoadProposal(id);

        if (false == serialized.has_value()) { return; }

        auto& data = *serialized;

        if (is_expired(data)) {
            wipe = true;

            return;
        }

        if (auto builder = get_builder(); builder) {
            switch (builder(id, data, promise)) {
                case BuildResult::PermanentFailure: {
                    wipe = true;
                    [[fallthrough]];
                }
                case BuildResult::TemporaryFailure: {

                    return;
                }
                case BuildResult::Success:
                default: {
                    confirming_.emplace(std::move(id), Clock::now());
                    erase = true;
                }
            }
        }
    }
};

Proposals::Proposals(
    const api::Core& api,
    const api::client::Blockchain& blockchain,
    const internal::Network& network,
    const internal::WalletDatabase& db,
    const Type chain) noexcept
    : imp_(std::make_unique<Imp>(api, blockchain, network, db, chain))
{
    OT_ASSERT(imp_);
}

auto Proposals::Add(const Proposal& tx) const noexcept
    -> std::future<block::pTxid>
{
    return imp_->Add(tx);
}

auto Proposals::Run() noexcept -> bool { return imp_->Run(); }

Proposals::~Proposals() = default;
}  // namespace opentxs::blockchain::client::wallet
