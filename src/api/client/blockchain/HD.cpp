// Copyright (c) 2010-2021 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include "0_stdafx.hpp"                  // IWYU pragma: associated
#include "1_Internal.hpp"                // IWYU pragma: associated
#include "api/client/blockchain/HD.hpp"  // IWYU pragma: associated

#include <robin_hood.h>
#include <memory>
#include <set>
#include <stdexcept>
#include <tuple>
#include <utility>

#include "api/client/blockchain/Deterministic.hpp"
#include "api/client/blockchain/Element.hpp"
#include "internal/api/Api.hpp"
#include "internal/api/client/Client.hpp"
#include "internal/api/client/blockchain/Factory.hpp"
#include "opentxs/Version.hpp"
#include "opentxs/api/HDSeed.hpp"
#include "opentxs/api/client/blockchain/BalanceNodeType.hpp"
#include "opentxs/api/client/blockchain/Subchain.hpp"
#include "opentxs/api/storage/Storage.hpp"
#include "opentxs/core/Identifier.hpp"
#include "opentxs/core/Log.hpp"
#include "opentxs/core/LogSource.hpp"
#include "opentxs/core/identifier/Nym.hpp"
#include "opentxs/protobuf/BlockchainAddress.pb.h"
#include "opentxs/protobuf/HDAccount.pb.h"

#define OT_METHOD "opentxs::api::client::blockchain::implementation::HD::"

namespace opentxs::factory
{
using ReturnType = api::client::blockchain::implementation::HD;

auto BlockchainHDBalanceNode(
    const api::internal::Core& api,
    const api::client::blockchain::internal::BalanceTree& parent,
    const proto::HDPath& path,
    const PasswordPrompt& reason,
    Identifier& id) noexcept
    -> std::unique_ptr<api::client::blockchain::internal::HD>
{
    try {
        return std::make_unique<ReturnType>(api, parent, path, reason, id);
    } catch (const std::exception& e) {
        LogVerbose("opentxs::Factory::")(__FUNCTION__)(": ")(e.what()).Flush();

        return nullptr;
    }
}

auto BlockchainHDBalanceNode(
    const api::internal::Core& api,
    const api::client::blockchain::internal::BalanceTree& parent,
    const proto::HDAccount& serialized,
    Identifier& id) noexcept
    -> std::unique_ptr<api::client::blockchain::internal::HD>
{
    using ReturnType = api::client::blockchain::implementation::HD;

    try {
        return std::make_unique<ReturnType>(api, parent, serialized, id);
    } catch (const std::exception& e) {
        LogOutput("opentxs::Factory::")(__FUNCTION__)(": ")(e.what()).Flush();

        return nullptr;
    }
}
}  // namespace opentxs::factory

namespace opentxs::api::client::blockchain::implementation
{
constexpr auto internalType{Subchain::Internal};
constexpr auto externalType{Subchain::External};

HD::HD(
    const api::internal::Core& api,
    const internal::BalanceTree& parent,
    const proto::HDPath& path,
    const PasswordPrompt& reason,
    Identifier& id) noexcept(false)
    : Deterministic(
          api,
          parent,
          BalanceNodeType::HD,
          Identifier::Factory(Translate(parent.Chain()), path),
          path,
          {{internalType, false, {}}, {externalType, true, {}}},
          id)
    , version_(DefaultVersion)
    , cached_internal_()
    , cached_external_()
{
    init(reason);
}

HD::HD(
    const api::internal::Core& api,
    const internal::BalanceTree& parent,
    const SerializedType& serialized,
    Identifier& id) noexcept(false)
    : Deterministic(
          api,
          parent,
          BalanceNodeType::HD,
          serialized.deterministic(),
          serialized.internaladdress().size(),
          serialized.externaladdress().size(),
          [&] {
              auto out = ChainData{
                  {internalType, false, {}}, {externalType, true, {}}};
              auto& internal = out.internal_.map_;
              auto& external = out.external_.map_;
              internal.reserve(serialized.internaladdress().size());
              external.reserve(serialized.externaladdress().size());

              for (const auto& address : serialized.internaladdress()) {
                  internal.emplace(
                      std::piecewise_construct,
                      std::forward_as_tuple(address.index()),
                      std::forward_as_tuple(
                          std::make_unique<implementation::Element>(
                              api,
                              parent.Parent().Parent(),
                              *this,
                              parent.Chain(),
                              internalType,
                              address)));
              }

              for (const auto& address : serialized.externaladdress()) {
                  external.emplace(
                      std::piecewise_construct,
                      std::forward_as_tuple(address.index()),
                      std::forward_as_tuple(
                          std::make_unique<implementation::Element>(
                              api,
                              parent.Parent().Parent(),
                              *this,
                              parent.Chain(),
                              data_.external_.type_,
                              address)));
              }

              return out;
          }(),
          id)
    , version_(serialized.version())
    , cached_internal_()
    , cached_external_()
{
    init();
}

auto HD::account_already_exists(const rLock&) const noexcept -> bool
{
    const auto existing = api_.Storage().BlockchainAccountList(
        parent_.NymID().str(), Translate(chain_));

    return 0 < existing.count(id_->str());
}

auto HD::PrivateKey(
    const Subchain type,
    const Bip32Index index,
    const PasswordPrompt& reason) const noexcept -> ECKey
{
    switch (type) {
        case internalType:
        case externalType: {
            break;
        }
        default: {
            LogOutput(OT_METHOD)(__FUNCTION__)(": Invalid subchain").Flush();

            return {};
        }
    }

#if OT_CRYPTO_WITH_BIP32
    const auto change =
        (internalType == type) ? INTERNAL_CHAIN : EXTERNAL_CHAIN;
    auto& pKey = (internalType == type) ? cached_internal_ : cached_external_;
    auto lock = rLock{lock_};

    if (!pKey) {
        pKey = api_.Seeds().AccountKey(path_, change, reason);

        if (!pKey) {
            LogOutput(OT_METHOD)(__FUNCTION__)(": Failed to derive account key")
                .Flush();

            return {};
        }
    }

    OT_ASSERT(pKey);

    const auto& key = *pKey;

    return key.ChildKey(index, reason);
#else

    return {};
#endif  // OT_CRYPTO_WITH_BIP32
}

auto HD::save(const rLock& lock) const noexcept -> bool
{
    const auto type = Translate(chain_);
    auto serialized = SerializedType{};
    serialized.set_version(version_);
    serialize_deterministic(lock, *serialized.mutable_deterministic());

    for (const auto& [index, address] : data_.internal_.map_) {
        *serialized.add_internaladdress() = address->Serialize();
    }

    for (const auto& [index, address] : data_.external_.map_) {
        *serialized.add_externaladdress() = address->Serialize();
    }

    const bool saved =
        api_.Storage().Store(parent_.NymID().str(), type, serialized);

    if (false == saved) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Failed to save HD account.")
            .Flush();

        return false;
    }

    return saved;
}
}  // namespace opentxs::api::client::blockchain::implementation
