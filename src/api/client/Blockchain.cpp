// Copyright (c) 2010-2020 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include "0_stdafx.hpp"               // IWYU pragma: associated
#include "1_Internal.hpp"             // IWYU pragma: associated
#include "api/client/Blockchain.hpp"  // IWYU pragma: associated

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <iterator>
#include <map>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string_view>
#include <thread>
#include <type_traits>

#if OT_BLOCKCHAIN
#include "api/client/blockchain/SyncClient.hpp"
#include "api/client/blockchain/SyncServer.hpp"
#include "core/Worker.hpp"
#endif  // OT_BLOCKCHAIN
#include "internal/api/client/Client.hpp"
#include "internal/api/client/Factory.hpp"
#include "internal/api/client/blockchain/Blockchain.hpp"
#if OT_BLOCKCHAIN
#include "internal/blockchain/Params.hpp"
#include "internal/blockchain/block/bitcoin/Bitcoin.hpp"
#include "internal/blockchain/client/Client.hpp"
#include "internal/blockchain/client/Factory.hpp"
#include "network/zeromq/socket/Socket.hpp"
#endif  // OT_BLOCKCHAIN
#include "opentxs/Pimpl.hpp"
#if OT_BLOCKCHAIN
#include "opentxs/Proto.tpp"
#include "opentxs/api/Endpoints.hpp"
#endif  // OT_BLOCKCHAIN
#include "opentxs/api/Factory.hpp"
#include "opentxs/api/Wallet.hpp"
#include "opentxs/api/client/Activity.hpp"
#include "opentxs/api/client/blockchain/Subchain.hpp"
#include "opentxs/api/crypto/Crypto.hpp"
#include "opentxs/api/crypto/Encode.hpp"
#include "opentxs/api/crypto/Hash.hpp"
#include "opentxs/api/storage/Storage.hpp"
#if OT_BLOCKCHAIN
#include "opentxs/blockchain/Blockchain.hpp"
#include "opentxs/blockchain/FilterType.hpp"
#endif  // OT_BLOCKCHAIN
#include "opentxs/blockchain/BlockchainType.hpp"
#if OT_BLOCKCHAIN
#include "opentxs/blockchain/client/FilterOracle.hpp"
#include "opentxs/blockchain/p2p/Types.hpp"
#endif  // OT_BLOCKCHAIN
//#include "opentxs/core/Amount.hpp"
#include "opentxs/core/Data.hpp"
#include "opentxs/core/Identifier.hpp"
#include "opentxs/core/Log.hpp"
#include "opentxs/core/LogSource.hpp"
#include "opentxs/crypto/Bip32Child.hpp"
#include "opentxs/crypto/Bip43Purpose.hpp"
#include "opentxs/crypto/Bip44Type.hpp"
#include "opentxs/identity/Nym.hpp"
#if OT_BLOCKCHAIN
#include "opentxs/network/zeromq/Context.hpp"
#include "opentxs/network/zeromq/Frame.hpp"
#include "opentxs/network/zeromq/FrameIterator.hpp"
#include "opentxs/network/zeromq/FrameSection.hpp"
#include "opentxs/network/zeromq/Message.hpp"
#include "opentxs/network/zeromq/socket/Sender.tpp"  // IWYU pragma: keep
#include "opentxs/protobuf/BlockchainP2PChainState.pb.h"
#include "opentxs/protobuf/BlockchainP2PHello.pb.h"
#include "opentxs/protobuf/BlockchainP2PSync.pb.h"
#include "opentxs/protobuf/Check.hpp"
#endif  // OT_BLOCKCHAIN
#include "opentxs/protobuf/ContactEnums.pb.h"
#include "opentxs/protobuf/Enums.pb.h"
#include "opentxs/protobuf/HDPath.pb.h"
#include "opentxs/protobuf/StorageThread.pb.h"
#include "opentxs/protobuf/StorageThreadItem.pb.h"
#if OT_BLOCKCHAIN
#include "opentxs/protobuf/verify/BlockchainP2PSync.hpp"
#include "opentxs/util/WorkType.hpp"
#endif  // OT_BLOCKCHAIN
#include "util/Container.hpp"
#include "util/HDIndex.hpp"
#if OT_BLOCKCHAIN
#include "util/Work.hpp"
#endif  // OT_BLOCKCHAIN

#define LOCK_NYM()                                                             \
    Lock mapLock(lock_);                                                       \
    auto& nymMutex = nym_lock_[nymID];                                         \
    mapLock.unlock();                                                          \
    Lock nymLock(nymMutex);

#define PATH_VERSION 1
#define COMPRESSED_PUBKEY_SIZE 33

#define OT_METHOD "opentxs::api::client::implementation::Blockchain::"

namespace zmq = opentxs::network::zeromq;

using ReturnType = opentxs::api::client::implementation::Blockchain;

namespace opentxs::factory
{
auto BlockchainAPI(
    const api::internal::Core& api,
    const api::client::Activity& activity,
    const api::client::Contacts& contacts,
    const api::Legacy& legacy,
    const std::string& dataFolder,
    const ArgList& args) noexcept
    -> std::shared_ptr<api::client::internal::Blockchain>
{
    return std::make_shared<ReturnType>(
        api, activity, contacts, legacy, dataFolder, args);
}
}  // namespace opentxs::factory

namespace opentxs::api::client::implementation
{
const Blockchain::AddressReverseMap Blockchain::address_prefix_reverse_map_{
    {"00", Prefix::BitcoinP2PKH},
    {"05", Prefix::BitcoinP2SH},
    {"30", Prefix::LitecoinP2PKH},
    {"32", Prefix::LitecoinP2SH},
    {"3a", Prefix::LitecoinTestnetP2SH},
    {"38", Prefix::PKTP2SH},
    {"6f", Prefix::BitcoinTestnetP2PKH},
    {"c4", Prefix::BitcoinTestnetP2SH},
    {"75", Prefix::PKTP2PKH},
};
const Blockchain::AddressMap Blockchain::address_prefix_map_{
    reverse_map(address_prefix_reverse_map_)};
const Blockchain::StyleMap Blockchain::address_style_map_{
    {{Style::P2PKH, Chain::UnitTest}, {Prefix::BitcoinTestnetP2PKH, {}}},
    {{Style::P2PKH, Chain::BitcoinCash_testnet3},
     {Prefix::BitcoinTestnetP2PKH, {}}},
    {{Style::P2PKH, Chain::BitcoinCash}, {Prefix::BitcoinP2PKH, {}}},
    {{Style::P2PKH, Chain::Bitcoin_testnet3},
     {Prefix::BitcoinTestnetP2PKH, {}}},
    {{Style::P2PKH, Chain::Bitcoin}, {Prefix::BitcoinP2PKH, {}}},
    {{Style::P2PKH, Chain::Litecoin_testnet4},
     {Prefix::BitcoinTestnetP2PKH, {}}},
    {{Style::P2PKH, Chain::Litecoin}, {Prefix::LitecoinP2PKH, {}}},
    {{Style::P2PKH, Chain::PKT_testnet}, {Prefix::BitcoinTestnetP2PKH, {}}},
    {{Style::P2PKH, Chain::PKT}, {Prefix::PKTP2PKH, {}}},
    {{Style::P2SH, Chain::UnitTest}, {Prefix::BitcoinTestnetP2SH, {}}},
    {{Style::P2SH, Chain::BitcoinCash_testnet3},
     {Prefix::BitcoinTestnetP2SH, {}}},
    {{Style::P2SH, Chain::BitcoinCash}, {Prefix::BitcoinP2SH, {}}},
    {{Style::P2SH, Chain::Bitcoin_testnet3}, {Prefix::BitcoinTestnetP2SH, {}}},
    {{Style::P2SH, Chain::Bitcoin}, {Prefix::BitcoinP2SH, {}}},
    {{Style::P2SH, Chain::Litecoin_testnet4},
     {Prefix::LitecoinTestnetP2SH, {Prefix::BitcoinTestnetP2SH}}},
    {{Style::P2SH, Chain::Litecoin},
     {Prefix::LitecoinP2SH, {Prefix::BitcoinP2SH}}},
    {{Style::P2SH, Chain::PKT_testnet}, {Prefix::BitcoinTestnetP2SH, {}}},
    {{Style::P2SH, Chain::PKT}, {Prefix::PKTP2SH, {}}},
};
const Blockchain::StyleReverseMap Blockchain::address_style_reverse_map_{
    ReturnType::reverse(address_style_map_)};

Blockchain::Blockchain(
    const api::internal::Core& api,
    [[maybe_unused]] const api::client::Activity& activity,
    const api::client::Contacts& contacts,
    [[maybe_unused]] const api::Legacy& legacy,
    [[maybe_unused]] const std::string& dataFolder,
    [[maybe_unused]] const ArgList& args) noexcept
    : api_(api)
#if OT_BLOCKCHAIN
    , activity_(activity)
#endif  // OT_BLOCKCHAIN
    , contacts_(contacts)
    , lock_()
    , nym_lock_()
    , accounts_(api_)
    , balance_lists_(api_, *this)
#if OT_BLOCKCHAIN
    , key_generated_endpoint_(opentxs::network::zeromq::socket::implementation::
                                  Socket::random_inproc_endpoint())
    , thread_pool_(api_)
    , io_(api_)
    , db_(api_, *this, legacy, dataFolder, args)
    , reorg_(api_.ZeroMQ().PublishSocket())
    , transaction_updates_(api_.ZeroMQ().PublishSocket())
    , peer_updates_(api_.ZeroMQ().PublishSocket())
    , key_updates_(api_.ZeroMQ().PublishSocket())
    , sync_updates_(api_.ZeroMQ().PublishSocket())
    , new_blockchain_accounts_(api_.ZeroMQ().PublishSocket())
    , base_config_([&] {
        auto output = opentxs::blockchain::client::internal::Config{};
        const auto sync = [&] {
            try {
                const auto& arg = args.at(OPENTXS_ARG_BLOCKCHAIN_SYNC);

                if (0 == arg.size()) { return false; }

                output.sync_endpoint_ = *arg.begin();

                return true;
            } catch (...) {

                return false;
            }
        }();

        using Policy = api::client::blockchain::BlockStorage;

        if (Policy::All == db_.BlockPolicy()) {
            output.generate_cfilters_ = true;

            if (sync) {
                output.provide_sync_server_ = true;
                output.disable_wallet_ = true;
            }
        } else if (sync) {
            output.use_sync_server_ = true;

        } else {
            output.download_cfilters_ = true;
        }

        try {
            const auto& arg = args.at("disableblockchainwallet");

            if (0 < arg.size()) { output.disable_wallet_ = true; }
        } catch (...) {
        }

        return output;
    }())
    , config_()
    , networks_()
    , sync_client_([&]() -> std::unique_ptr<blockchain::SyncClient> {
        if (base_config_.use_sync_server_) {
            return std::make_unique<blockchain::SyncClient>(
                api_, *this, base_config_.sync_endpoint_);
        }

        return {};
    }())
    , sync_server_([&]() -> std::unique_ptr<blockchain::SyncServer> {
        if (base_config_.provide_sync_server_) {
            return std::make_unique<blockchain::SyncServer>(api_, *this);
        }

        return {};
    }())
    , balances_(*this, api_)
    , enabled_callbacks_(api_)
    , last_hello_([&] {
        auto output = LastHello{};

        for (const auto& chain : opentxs::blockchain::SupportedChains()) {
            output.emplace(chain, Clock::from_time_t(0));
        }

        return output;
    }())
    , running_(true)
    , heartbeat_(&Blockchain::heartbeat, this)
#endif  // OT_BLOCKCHAIN
{
    // WARNING: do not access api_.Wallet() during construction
#if OT_BLOCKCHAIN
    auto listen = reorg_->Start(api_.Endpoints().BlockchainReorg());

    OT_ASSERT(listen);

    listen =
        transaction_updates_->Start(api_.Endpoints().BlockchainTransactions());

    OT_ASSERT(listen);

    listen = peer_updates_->Start(api_.Endpoints().BlockchainPeer());

    OT_ASSERT(listen);

    listen = key_updates_->Start(key_generated_endpoint_);

    OT_ASSERT(listen);

    listen = sync_updates_->Start(api_.Endpoints().BlockchainSyncProgress());

    OT_ASSERT(listen);

    listen = new_blockchain_accounts_->Start(
        api_.Endpoints().BlockchainAccountCreated());

    OT_ASSERT(listen);

    if (sync_client_) { sync_client_->Heartbeat(Hello()); }
#endif  // OT_BLOCKCHAIN
}

auto Blockchain::ActivityDescription(
    const identifier::Nym& nym,
    const Identifier& thread,
    const std::string& threadItemID) const noexcept -> std::string
{
#if OT_BLOCKCHAIN
    auto pThread = std::shared_ptr<proto::StorageThread>{};
    api_.Storage().Load(nym.str(), thread.str(), pThread);

    if (false == bool(pThread)) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": thread ")(thread.str())(
            " does not exist for nym ")(nym.str())
            .Flush();

        return {};
    }

    const auto& data = *pThread;

    for (const auto& item : data.item()) {
        if (item.id() != threadItemID) { continue; }

        const auto txid = api_.Factory().Data(item.txid(), StringStyle::Raw);
        const auto chain = static_cast<opentxs::blockchain::Type>(item.chain());
        const auto pTx = LoadTransactionBitcoin(txid);

        if (false == bool(pTx)) {
            LogOutput(OT_METHOD)(__FUNCTION__)(": failed to load transaction ")(
                txid->asHex())
                .Flush();

            return {};
        }

        const auto& tx = *pTx;

        return this->ActivityDescription(nym, chain, tx);
    }

    LogOutput(OT_METHOD)(__FUNCTION__)(": item ")(threadItemID)(" not found ")
        .Flush();
#endif  // OT_BLOCKCHAIN

    return {};
}

#if OT_BLOCKCHAIN
auto Blockchain::ActivityDescription(
    const identifier::Nym& nym,
    const Chain chain,
    const Tx& transaction) const noexcept -> std::string
{
    auto output = std::stringstream{};
    const auto amount = transaction.NetBalanceChange(*this, nym);
    const auto memo = transaction.Memo(*this);

    if (0 < amount) {
        output << "Incoming ";
    } else if (0 > amount) {
        output << "Outgoing ";
    }

    output << opentxs::blockchain::DisplayString(chain);
    output << " transaction";

    if (false == memo.empty()) { output << ": " << memo; }

    return output.str();
}
#endif  // OT_BLOCKCHAIN

auto Blockchain::address_prefix(const Style style, const Chain chain) const
    noexcept(false) -> OTData
{
    return api_.Factory().Data(
        address_prefix_map_.at(address_style_map_.at({style, chain}).first),
        StringStyle::Hex);
}

auto Blockchain::AssignContact(
    const identifier::Nym& nymID,
    const Identifier& accountID,
    const blockchain::Subchain subchain,
    const Bip32Index index,
    const Identifier& contactID) const noexcept -> bool
{
    if (false == validate_nym(nymID)) { return false; }

    LOCK_NYM()

    const auto chain = Translate(
        api_.Storage().BlockchainAccountType(nymID.str(), accountID.str()));

    OT_ASSERT(Chain::Unknown != chain);

    try {
        auto& node = balance_lists_.Get(chain).Nym(nymID).Node(accountID);

        try {
            const auto& element = node.BalanceElement(subchain, index);
            const auto existing = element.Contact();

            if (contactID == existing) { return true; }

            return node.SetContact(subchain, index, contactID);
        } catch (...) {
            LogOutput(OT_METHOD)(__FUNCTION__)(
                ": Failed to load balance element")
                .Flush();

            return false;
        }
    } catch (...) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Failed to load account").Flush();

        return false;
    }
}

auto Blockchain::AssignLabel(
    const identifier::Nym& nymID,
    const Identifier& accountID,
    const blockchain::Subchain subchain,
    const Bip32Index index,
    const std::string& label) const noexcept -> bool
{
    if (false == validate_nym(nymID)) { return false; }

    LOCK_NYM()

    const auto chain = Translate(
        api_.Storage().BlockchainAccountType(nymID.str(), accountID.str()));

    OT_ASSERT(Chain::Unknown != chain);

    try {
        auto& node = balance_lists_.Get(chain).Nym(nymID).Node(accountID);

        try {
            const auto& element = node.BalanceElement(subchain, index);

            if (label == element.Label()) { return true; }

            return node.SetLabel(subchain, index, label);
        } catch (...) {
            LogOutput(OT_METHOD)(__FUNCTION__)(
                ": Failed to load balance element")
                .Flush();

            return false;
        }
    } catch (...) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Failed to load account").Flush();

        return false;
    }
}

#if OT_BLOCKCHAIN
auto Blockchain::AssignTransactionMemo(
    const TxidHex& id,
    const std::string& label) const noexcept -> bool
{
    auto lock = Lock{lock_};
    auto pTransaction = load_transaction(lock, id);

    if (false == bool(pTransaction)) { return false; }

    auto& transaction = *pTransaction;
    transaction.SetMemo(label);
    const auto serialized = transaction.Serialize(*this);

    OT_ASSERT(serialized.has_value());

    if (false == db_.StoreTransaction(serialized.value())) { return false; }

    broadcast_update_signal(transaction);

    return true;
}
#endif  // OT_BLOCKCHAIN

auto Blockchain::BalanceTree(const identifier::Nym& nymID, const Chain chain)
    const noexcept(false) -> const blockchain::internal::BalanceTree&
{
    if (false == validate_nym(nymID)) {
        throw std::runtime_error("Invalid nym");
    }

    if (Chain::Unknown == chain) { throw std::runtime_error("Invalid chain"); }

    auto& balanceList = balance_lists_.Get(chain);

    return balanceList.Nym(nymID);
}

auto Blockchain::bip44_type(const proto::ContactItemType type) const noexcept
    -> Bip44Type
{
    switch (type) {
        case proto::CITEMTYPE_BTC: {

            return Bip44Type::BITCOIN;
        }
        case proto::CITEMTYPE_LTC: {

            return Bip44Type::LITECOIN;
        }
        case proto::CITEMTYPE_DOGE: {

            return Bip44Type::DOGECOIN;
        }
        case proto::CITEMTYPE_DASH: {

            return Bip44Type::DASH;
        }
        case proto::CITEMTYPE_BCH: {

            return Bip44Type::BITCOINCASH;
        }
        case proto::CITEMTYPE_PKT: {

            return Bip44Type::PKT;
        }
        case proto::CITEMTYPE_TNBCH:
        case proto::CITEMTYPE_TNBTC:
        case proto::CITEMTYPE_TNXRP:
        case proto::CITEMTYPE_TNLTC:
        case proto::CITEMTYPE_TNXEM:
        case proto::CITEMTYPE_TNDASH:
        case proto::CITEMTYPE_TNMAID:
        case proto::CITEMTYPE_TNLSK:
        case proto::CITEMTYPE_TNDOGE:
        case proto::CITEMTYPE_TNXMR:
        case proto::CITEMTYPE_TNWAVES:
        case proto::CITEMTYPE_TNNXT:
        case proto::CITEMTYPE_TNSC:
        case proto::CITEMTYPE_TNSTEEM:
        case proto::CITEMTYPE_TNPKT:
        case proto::CITEMTYPE_REGTEST: {
            return Bip44Type::TESTNET;
        }
        default: {
            OT_FAIL;
        }
    }
}

#if OT_BLOCKCHAIN
auto Blockchain::broadcast_update_signal(
    const std::vector<pTxid>& transactions) const noexcept -> void
{
    std::for_each(
        std::begin(transactions),
        std::end(transactions),
        [this](const auto& txid) {
            const auto data = db_.LoadTransaction(txid->Bytes());

            OT_ASSERT(data.has_value());

            const auto tx =
                factory::BitcoinTransaction(api_, *this, data.value());

            OT_ASSERT(tx);

            broadcast_update_signal(*tx);
        });
}

auto Blockchain::broadcast_update_signal(
    const opentxs::blockchain::block::bitcoin::internal::Transaction& tx)
    const noexcept -> void
{
    const auto chains = tx.Chains();
    std::for_each(std::begin(chains), std::end(chains), [&](const auto& chain) {
        auto out =
            api_.ZeroMQ().TaggedMessage(WorkType::BlockchainNewTransaction);
        out->AddFrame(tx.ID());
        out->AddFrame(chain);
        transaction_updates_->Send(out);
    });
}
#endif  // OT_BLOCKCHAIN

auto Blockchain::CalculateAddress(
    const Chain chain,
    const Style format,
    const Data& pubkey) const noexcept -> std::string
{
    auto data = api_.Factory().Data();

    switch (format) {
        case Style::P2PKH: {
            try {
                data = PubkeyHash(chain, pubkey);
            } catch (...) {
                LogOutput(OT_METHOD)(__FUNCTION__)(": Invalid public key.")
                    .Flush();

                return {};
            }
        } break;
        default: {
            LogOutput(OT_METHOD)(__FUNCTION__)(": Unsupported address style (")(
                static_cast<std::uint16_t>(format))(")")
                .Flush();

            return {};
        }
    }

    return EncodeAddress(format, chain, data);
}

#if OT_BLOCKCHAIN
auto Blockchain::check_hello(const Lock&) const noexcept -> Chains
{
    constexpr auto limit = std::chrono::seconds(30);
    auto output = Chains{};
    const auto now = Clock::now();

    for (const auto& [chain, network] : networks_) {
        auto& last = last_hello_[chain];

        if ((now - last) > limit) {
            last = now;
            output.emplace_back(chain);
        }
    }

    return output;
}
#endif  // OT_BLOCKCHAIN

auto Blockchain::DecodeAddress(const std::string& encoded) const noexcept
    -> DecodedAddress
{
    auto output = DecodedAddress{api_.Factory().Data(), Style::Unknown, {}};
    auto& [data, style, chains] = output;
    const auto bytes = api_.Factory().Data(
        api_.Crypto().Encode().IdentifierDecode(encoded), StringStyle::Raw);
    auto type = api_.Factory().Data();

    switch (bytes->size()) {
        case 21: {
            bytes->Extract(1, type, 0);
            auto prefix{Prefix::Unknown};

            try {
                prefix = address_prefix_reverse_map_.at(type->asHex());
            } catch (...) {
                LogOutput(OT_METHOD)(__FUNCTION__)(
                    ": Unable to decode version byte (")(type->asHex())
                    .Flush();

                return output;
            }

            const auto& map = address_style_reverse_map_.at(prefix);

            for (const auto& [decodeStyle, decodeChain] : map) {
                style = decodeStyle;
                chains.emplace(decodeChain);
            }

            bytes->Extract(20, data, 1);
        } break;
        default: {
            LogOutput(OT_METHOD)(__FUNCTION__)(": Unable to decode address (")(
                bytes->asHex())
                .Flush();
        }
    }

    return output;
}

#if OT_BLOCKCHAIN
auto Blockchain::Disable(const Chain type) const noexcept -> bool
{
    auto lock = Lock{lock_};

    return disable(lock, type);
}

auto Blockchain::disable(const Lock& lock, const Chain type) const noexcept
    -> bool
{
    if (0 == opentxs::blockchain::SupportedChains().count(type)) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Unsupported chain").Flush();

        return false;
    }

    stop(lock, type);

    if (db_.Disable(type)) {
        enabled_callbacks_.Execute(type, false);

        return true;
    }

    return false;
}

auto Blockchain::Enable(const Chain type, const std::string& seednode)
    const noexcept -> bool
{
    auto lock = Lock{lock_};

    return enable(lock, type, seednode);
}

auto Blockchain::enable(
    const Lock& lock,
    const Chain type,
    const std::string& seednode) const noexcept -> bool
{
    if (0 == opentxs::blockchain::SupportedChains().count(type)) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Unsupported chain").Flush();

        return false;
    }

    if (false == db_.Enable(type, seednode)) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Database error").Flush();

        return false;
    }

    if (start(lock, type, seednode)) {
        enabled_callbacks_.Execute(type, true);

        return true;
    }

    return false;
}
#endif  // OT_BLOCKCHAIN

auto Blockchain::EncodeAddress(
    const Style style,
    const Chain chain,
    const Data& data) const noexcept -> std::string
{
    switch (style) {
        case Style::P2PKH: {

            return p2pkh(chain, data);
        }
        case Style::P2SH: {

            return p2sh(chain, data);
        }
        default: {
            LogOutput(OT_METHOD)(__FUNCTION__)(": Unsupported address style (")(
                static_cast<std::uint16_t>(style))(")")
                .Flush();

            return {};
        }
    }
}

#if OT_BLOCKCHAIN
auto Blockchain::GetChain(const Chain type) const noexcept(false)
    -> const opentxs::blockchain::Network&
{
    auto lock = Lock{lock_};

    return *networks_.at(type);
}
#endif  // OT_BLOCKCHAIN

auto Blockchain::GetKey(const blockchain::Key& id) const noexcept(false)
    -> const blockchain::BalanceNode::Element&
{
    const auto [str, subchain, index] = id;
    const auto account = api_.Factory().Identifier(str);

    switch (accounts_.Type(account)) {
        case AccountType::HD: {
            const auto& hd = HDSubaccount(accounts_.Owner(account), account);

            return hd.BalanceElement(subchain, index);
        }
        case AccountType::Error:
        case AccountType::Imported:
        case AccountType::PaymentCode:
        default: {
        }
    }

    throw std::out_of_range("key not found");
}

auto Blockchain::HDSubaccount(
    const identifier::Nym& nymID,
    const Identifier& accountID) const noexcept(false) -> const blockchain::HD&
{
    const auto type =
        api_.Storage().BlockchainAccountType(nymID.str(), accountID.str());

    if (proto::CITEMTYPE_ERROR == type) {
        throw std::out_of_range("Account does not exist");
    }

    auto& balanceList = balance_lists_.Get(Translate(type));
    auto& nym = balanceList.Nym(nymID);

    return nym.HDChain(accountID);
}

#if OT_BLOCKCHAIN
auto Blockchain::heartbeat() const noexcept -> void
{
    while (running_) {
        auto counter{-1};

        while (running_ && (20 > ++counter)) {
            Sleep(std::chrono::milliseconds{250});
        }

        auto lock = Lock{lock_};

        if (sync_client_) {
            sync_client_->Heartbeat(hello(lock, check_hello(lock)));
        }

        for (const auto& [key, value] : networks_) {
            if (false == running_) { return; }

            value->Heartbeat();
        }
    }
}

auto Blockchain::Hello() const noexcept -> proto::BlockchainP2PHello
{
    auto lock = Lock{lock_};
    auto chains = [&] {
        auto output = Chains{};

        for (const auto& [chain, network] : networks_) {
            output.emplace_back(chain);
        }

        return output;
    }();

    return hello(lock, chains);
}

auto Blockchain::hello(const Lock&, const Chains& chains) const noexcept
    -> proto::BlockchainP2PHello
{
    auto output = proto::BlockchainP2PHello{};
    output.set_version(opentxs::blockchain::client::sync_hello_version_);

    for (const auto chain : chains) {
        const auto& network = networks_.at(chain);
        const auto& filter = network->FilterOracle();
        const auto type = filter.DefaultType();
        const auto best = filter.FilterTip(type);
        auto& state = *output.add_state();
        state.set_version(opentxs::blockchain::client::sync_state_version_);
        state.set_chain(static_cast<std::uint32_t>(chain));
        state.set_height(static_cast<std::uint64_t>(best.first));
        const auto bytes = best.second->Bytes();
        state.set_hash(bytes.data(), bytes.size());
    }

    return output;
}

auto Blockchain::IndexItem(const ReadView bytes) const noexcept -> PatternID
{
    auto output = PatternID{};
    const auto hashed = api_.Crypto().Hash().HMAC(
        proto::HASHTYPE_SIPHASH24,
        db_.HashKey(),
        bytes,
        preallocated(sizeof(output), &output));

    OT_ASSERT(hashed);

    return output;
}
#endif  // OT_BLOCKCHAIN

auto Blockchain::init_path(
    const std::string& root,
    const proto::ContactItemType chain,
    const Bip32Index account,
    const BlockchainAccountType standard,
    proto::HDPath& path) const noexcept -> void
{
    path.set_version(PATH_VERSION);
    path.set_root(root);

    switch (standard) {
        case BlockchainAccountType::BIP32: {
            path.add_child(HDIndex{account, Bip32Child::HARDENED});
        } break;
        case BlockchainAccountType::BIP44: {
            path.add_child(
                HDIndex{Bip43Purpose::HDWALLET, Bip32Child::HARDENED});
            path.add_child(HDIndex{bip44_type(chain), Bip32Child::HARDENED});
            path.add_child(account);
        } break;
        default: {
            OT_FAIL;
        }
    }
}

#if OT_BLOCKCHAIN
auto Blockchain::IsEnabled(const opentxs::blockchain::Type chain) const noexcept
    -> bool
{
    auto lock = Lock{lock_};

    for (const auto& [enabled, peer] : db_.LoadEnabledChains()) {
        if (chain == enabled) { return true; }
    }

    return false;
}

auto Blockchain::KeyGenerated(const Chain chain) const noexcept -> void
{
    auto work = MakeWork(api_, OT_ZMQ_NEW_BLOCKCHAIN_WALLET_KEY_SIGNAL);
    work->AddFrame(chain);
    key_updates_->Send(work);
}

auto Blockchain::load_transaction(const Lock& lock, const TxidHex& txid)
    const noexcept -> std::unique_ptr<
        opentxs::blockchain::block::bitcoin::internal::Transaction>
{
    return load_transaction(lock, api_.Factory().Data(txid, StringStyle::Hex));
}

auto Blockchain::load_transaction(const Lock& lock, const Txid& txid)
    const noexcept -> std::unique_ptr<
        opentxs::blockchain::block::bitcoin::internal::Transaction>
{
    const auto serialized = db_.LoadTransaction(txid.Bytes());

    if (false == serialized.has_value()) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Transaction ")(txid.asHex())(
            " not found")
            .Flush();

        return {};
    }

    return factory::BitcoinTransaction(api_, *this, serialized.value());
}

auto Blockchain::LoadTransactionBitcoin(const TxidHex& txid) const noexcept
    -> std::unique_ptr<const Tx>
{
    auto lock = Lock{lock_};

    return load_transaction(lock, txid);
}

auto Blockchain::LoadTransactionBitcoin(const Txid& txid) const noexcept
    -> std::unique_ptr<const Tx>
{
    auto lock = Lock{lock_};

    return load_transaction(lock, txid);
}

auto Blockchain::LookupContacts(const std::string& address) const noexcept
    -> ContactList
{
    const auto [pubkeyHash, style, chain] = DecodeAddress(address);

    return LookupContacts(pubkeyHash);
}

auto Blockchain::LookupContacts(const Data& pubkeyHash) const noexcept
    -> ContactList
{
    return db_.LookupContact(pubkeyHash);
}
#endif  // OT_BLOCKCHAIN

auto Blockchain::NewHDSubaccount(
    const identifier::Nym& nymID,
    const BlockchainAccountType standard,
    const Chain chain,
    const PasswordPrompt& reason) const noexcept -> OTIdentifier
{
    if (false == validate_nym(nymID)) { return Identifier::Factory(); }

    if (Chain::Unknown == chain) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Invalid chain").Flush();

        return Identifier::Factory();
    }

    auto nym = api_.Wallet().Nym(nymID);

    if (false == bool(nym)) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Nym does not exist.").Flush();

        return Identifier::Factory();
    }

    proto::HDPath nymPath{};

    if (false == nym->Path(nymPath)) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": No nym path.").Flush();

        return Identifier::Factory();
    }

    if (0 == nymPath.root().size()) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Missing root.").Flush();

        return Identifier::Factory();
    }

    if (2 > nymPath.child().size()) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Invalid path.").Flush();

        return Identifier::Factory();
    }

    proto::HDPath accountPath{};
    init_path(
        nymPath.root(),
        Translate(chain),
        HDIndex{nymPath.child(1), Bip32Child::HARDENED},
        standard,
        accountPath);

    try {
        auto accountID = Identifier::Factory();
        auto& tree = balance_lists_.Get(chain).Nym(nymID);
        tree.AddHDNode(accountPath, accountID);
        accounts_.New(chain, accountID, nymID);

#if OT_BLOCKCHAIN
        {
            auto work =
                api_.ZeroMQ().TaggedMessage(WorkType::BlockchainAccountCreated);
            work->AddFrame(chain);
            work->AddFrame(nymID);
            work->AddFrame(AccountType::HD);
            work->AddFrame(accountID);
            new_blockchain_accounts_->Send(work);
        }

        balances_.RefreshBalance(nymID, chain);
#endif  // OT_BLOCKCHAIN

        return accountID;
    } catch (...) {
        LogVerbose(OT_METHOD)(__FUNCTION__)(": Failed to create account")
            .Flush();

        return Identifier::Factory();
    }
}

auto Blockchain::p2pkh(const Chain chain, const Data& pubkeyHash) const noexcept
    -> std::string
{
    try {
        auto preimage = address_prefix(Style::P2PKH, chain);

        OT_ASSERT(1 == preimage->size());

        preimage += pubkeyHash;

        OT_ASSERT(21 == preimage->size());

        return api_.Crypto().Encode().IdentifierEncode(preimage);
    } catch (...) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Unsupported chain (")(
            static_cast<std::uint32_t>(chain))(")")
            .Flush();

        return "";
    }
}

auto Blockchain::p2sh(const Chain chain, const Data& pubkeyHash) const noexcept
    -> std::string
{
    try {
        auto preimage = address_prefix(Style::P2SH, chain);

        OT_ASSERT(1 == preimage->size());

        preimage += pubkeyHash;

        OT_ASSERT(21 == preimage->size());

        return api_.Crypto().Encode().IdentifierEncode(preimage);
    } catch (...) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Unsupported chain (")(
            static_cast<std::uint32_t>(chain))(")")
            .Flush();

        return "";
    }
}

#if OT_BLOCKCHAIN
auto Blockchain::ProcessContact(const Contact& contact) const noexcept -> bool
{
    broadcast_update_signal(db_.UpdateContact(contact));

    return true;
}

auto Blockchain::ProcessMergedContact(
    const Contact& parent,
    const Contact& child) const noexcept -> bool
{
    broadcast_update_signal(db_.UpdateMergedContact(parent, child));

    return true;
}

auto Blockchain::ProcessSyncData(OTZMQMessage&& in) const noexcept -> void
{
    const auto b = in->Body();

    OT_ASSERT(2 < b.size());

    using Network = opentxs::blockchain::client::internal::Network;
    auto chain = std::optional<Chain>{std::nullopt};
    using FilterType = opentxs::blockchain::filter::Type;
    auto filterType = std::optional<FilterType>{std::nullopt};
    using Height = opentxs::blockchain::block::Height;
    auto height = Height{-1};
    auto work = MakeWork(api_, Network::Task::SyncData);
    work->AddFrame(b.at(0));

    for (auto i{std::next(b.begin(), 2)}; i != b.end(); std::advance(i, 1)) {
        const auto sync = proto::Factory<proto::BlockchainP2PSync>(*i);

        if (false == proto::Validate(sync, VERBOSE)) {
            LogOutput(OT_METHOD)(__FUNCTION__)(": Invalid sync data").Flush();

            return;
        }

        const auto incomingChain = static_cast<Chain>(sync.chain());
        const auto incomingHeight = static_cast<Height>(sync.height());
        const auto incomingType = static_cast<FilterType>(sync.filter_type());

        if (chain.has_value()) {
            if (incomingHeight != ++height) {
                LogOutput(OT_METHOD)(__FUNCTION__)(": Non-contiguous sync data")
                    .Flush();

                return;
            }

            if (incomingChain != chain.value()) {
                LogOutput(OT_METHOD)(__FUNCTION__)(": Invalid chain").Flush();

                return;
            }

            if (incomingType != filterType.value()) {
                LogOutput(OT_METHOD)(__FUNCTION__)(": Invalid filter type")
                    .Flush();

                return;
            }
        } else {
            chain = incomingChain;
            height = incomingHeight;
            filterType = incomingType;
        }

        work->AddFrame(std::move(*i));  // TODO overload AddFrame so this avoids
                                        // making a copy
    }

    auto lock = Lock{lock_};

    try {
        auto& network = *networks_.at(chain.value());
        network.Submit(work);
    } catch (...) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Chain not active").Flush();

        return;
    }
}

auto Blockchain::ProcessTransaction(
    const Chain chain,
    const Tx& in,
    const PasswordPrompt& reason) const noexcept -> bool
{
    auto lock = Lock{lock_};
    auto pTransaction = in.clone();

    OT_ASSERT(pTransaction);

    auto& transaction = *pTransaction;
    const auto& id = transaction.ID();
    const auto txid = id.Bytes();

    if (const auto tx = db_.LoadTransaction(txid); tx.has_value()) {
        transaction.MergeMetadata(*this, chain, tx.value());
        auto updated = transaction.Serialize(*this);

        OT_ASSERT(updated.has_value());

        if (false == db_.StoreTransaction(updated.value())) { return false; }
    } else {
        auto serialized = transaction.Serialize(*this);

        OT_ASSERT(serialized.has_value());

        if (false == db_.StoreTransaction(serialized.value())) { return false; }
    }

    if (false == db_.AssociateTransaction(id, transaction.GetPatterns())) {
        return false;
    }

    return reconcile_activity_threads(lock, transaction);
}

#endif  // OT_BLOCKCHAIN

auto Blockchain::PubkeyHash(
    [[maybe_unused]] const Chain chain,
    const Data& pubkey) const noexcept(false) -> OTData
{
    if (pubkey.empty()) { throw std::runtime_error("Empty pubkey"); }

    if (COMPRESSED_PUBKEY_SIZE != pubkey.size()) {
        throw std::runtime_error("Incorrect pubkey size");
    }

    auto output = Data::Factory();

    if (false ==
        api_.Crypto().Hash().Digest(
            proto::HASHTYPE_BITCOIN, pubkey.Bytes(), output->WriteInto())) {
        throw std::runtime_error("Unable to calculate hash.");
    }

    return output;
}

auto Blockchain::reverse(const StyleMap& in) noexcept -> StyleReverseMap
{
    auto output = StyleReverseMap{};
    std::for_each(std::begin(in), std::end(in), [&](const auto& data) {
        const auto& [metadata, prefixData] = data;
        const auto& [preferred, additional] = prefixData;
        output[preferred].emplace(metadata);

        for (const auto& prefix : additional) {
            output[prefix].emplace(metadata);
        }
    });

    return output;
}

#if OT_BLOCKCHAIN
auto Blockchain::reconcile_activity_threads(const Lock& lock, const Txid& txid)
    const noexcept -> bool
{
    const auto tx = load_transaction(lock, txid);

    if (false == bool(tx)) { return false; }

    return reconcile_activity_threads(lock, *tx);
}

auto Blockchain::reconcile_activity_threads(
    const Lock& lock,
    const opentxs::blockchain::block::bitcoin::internal::Transaction& tx)
    const noexcept -> bool
{
    if (!activity_.AddBlockchainTransaction(*this, tx)) { return false; }

    broadcast_update_signal(tx);

    return true;
}

auto Blockchain::ReportProgress(
    const Chain chain,
    const opentxs::blockchain::block::Height current,
    const opentxs::blockchain::block::Height target) const noexcept -> void
{
    auto work = api_.ZeroMQ().TaggedMessage(WorkType::BlockchainSyncProgress);
    work->AddFrame(chain);
    work->AddFrame(current);
    work->AddFrame(target);
    sync_updates_->Send(work);

    if (sync_client_ && sync_client_->IsActive(chain)) {
        auto lock = Lock{lock_};
        last_hello_[chain] = Clock::now();
        sync_client_->Heartbeat(hello(lock, {chain}));
    }
}

auto Blockchain::RestoreNetworks() const noexcept -> void
{
    for (const auto& [chain, peer] : db_.LoadEnabledChains()) {
        Start(chain, peer);
    }
}

auto Blockchain::Start(const Chain type, const std::string& seednode)
    const noexcept -> bool
{
    auto lock = Lock{lock_};

    return start(lock, type, seednode);
}

auto Blockchain::start(
    const Lock& lock,
    const Chain type,
    const std::string& seednode) const noexcept -> bool
{
    if (Chain::UnitTest != type) {
        if (0 == opentxs::blockchain::SupportedChains().count(type)) {
            LogOutput(OT_METHOD)(__FUNCTION__)(": Unsupported chain").Flush();

            return false;
        }
    }

    if (0 != networks_.count(type)) {
        LogVerbose(OT_METHOD)(__FUNCTION__)(": Chain already running").Flush();

        return true;
    }

    thread_pool_.Reset(type);

    namespace p2p = opentxs::blockchain::p2p;

    switch (opentxs::blockchain::params::Data::chains_.at(type).p2p_protocol_) {
        case p2p::Protocol::bitcoin: {
            auto endpoint = std::string{};

            if (sync_server_) {
                sync_server_->Enable(type);
                endpoint = sync_server_->Endpoint(type);
            }

            auto& config = [&]() -> const Config& {
                {
                    auto it = config_.find(type);

                    if (config_.end() != it) { return it->second; }
                }

                auto [it, added] = config_.emplace(type, base_config_);
                auto& active = it->second.use_sync_server_;

                OT_ASSERT(added);

                if (active) {
                    OT_ASSERT(sync_client_);

                    active = sync_client_->IsConnected() &&
                             sync_client_->IsActive(type);
                }

                return it->second;
            }();

            auto [it, added] = networks_.emplace(
                type,
                factory::BlockchainNetworkBitcoin(
                    api_, *this, type, config, seednode, endpoint));

            return it->second->Connect();
        }
        case p2p::Protocol::opentxs:
        case p2p::Protocol::ethereum:
        default: {
        }
    }

    return false;
}

auto Blockchain::StartSyncServer(
    const std::string& sync,
    const std::string& publicSync,
    const std::string& update,
    const std::string& publicUpdate) const noexcept -> bool
{
    auto lock = Lock{lock_};

    if (sync_server_) {
        return sync_server_->Start(sync, publicSync, update, publicUpdate);
    }

    LogNormal("Blockchain sync server must be enabled at library "
              "initialization time by passing the ")(
        OPENTXS_ARG_BLOCKCHAIN_SYNC)(" option.")
        .Flush();

    return false;
}

auto Blockchain::Stop(const Chain type) const noexcept -> bool
{
    auto lock = Lock{lock_};

    return stop(lock, type);
}

auto Blockchain::stop(const Lock& lock, const Chain type) const noexcept -> bool
{
    auto it = networks_.find(type);

    if (networks_.end() == it) { return true; }

    OT_ASSERT(it->second);

    if (sync_server_) { sync_server_->Disable(type); }

    it->second->Shutdown().get();
    thread_pool_.Stop(type).get();
    networks_.erase(it);

    return true;
}
#endif  // OT_BLOCKCHAIN

auto Blockchain::UpdateElement(
    [[maybe_unused]] std::vector<ReadView>& pubkeyHashes) const noexcept -> void
{
#if OT_BLOCKCHAIN
    auto patterns = std::vector<PatternID>{};
    std::for_each(
        std::begin(pubkeyHashes),
        std::end(pubkeyHashes),
        [&](const auto& bytes) { patterns.emplace_back(IndexItem(bytes)); });
    LogTrace(OT_METHOD)(__FUNCTION__)(": ")(patterns.size())(
        " pubkey hashes have changed:")
        .Flush();
    auto transactions = std::vector<pTxid>{};
    std::for_each(
        std::begin(patterns), std::end(patterns), [&](const auto& pattern) {
            LogTrace("    * ")(pattern).Flush();
            auto matches = db_.LookupTransactions(pattern);
            transactions.reserve(transactions.size() + matches.size());
            std::move(
                std::begin(matches),
                std::end(matches),
                std::back_inserter(transactions));
        });
    dedup(transactions);
    auto lock = Lock{lock_};
    std::for_each(
        std::begin(transactions),
        std::end(transactions),
        [&](const auto& txid) { reconcile_activity_threads(lock, txid); });
#endif  // OT_BLOCKCHAIN
}

#if OT_BLOCKCHAIN
auto Blockchain::UpdatePeer(
    const opentxs::blockchain::Type chain,
    const std::string& address) const noexcept -> void
{
    auto work = MakeWork(api_, WorkType::BlockchainPeerAdded);
    work->AddFrame(chain);
    work->AddFrame(address);
    peer_updates_->Send(work);
}

#endif  // OT_BLOCKCHAIN
auto Blockchain::validate_nym(const identifier::Nym& nymID) const noexcept
    -> bool
{
    if (false == nymID.empty()) {
        if (0 < api_.Wallet().LocalNyms().count(nymID)) { return true; }
    }

    return false;
}

Blockchain::~Blockchain()
{
#if OT_BLOCKCHAIN
    running_ = false;

    if (heartbeat_.joinable()) { heartbeat_.join(); }

    LogVerbose("Shutting down ")(networks_.size())(" blockchain clients")
        .Flush();

    for (auto& [chain, network] : networks_) { network->Shutdown().get(); }

    networks_.clear();
    thread_pool_.Shutdown();
    io_.Shutdown();

#endif  // OT_BLOCKCHAIN
}
}  // namespace opentxs::api::client::implementation
