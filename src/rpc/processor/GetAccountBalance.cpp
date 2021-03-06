// Copyright (c) 2010-2021 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include "0_stdafx.hpp"    // IWYU pragma: associated
#include "1_Internal.hpp"  // IWYU pragma: associated
#include "rpc/RPC.tpp"     // IWYU pragma: associated

#include <iosfwd>
#include <string>
#include <type_traits>
#include <vector>

#include "internal/api/client/Client.hpp"
#include "internal/core/Core.hpp"
#include "opentxs/Pimpl.hpp"
#include "opentxs/Shared.hpp"
#include "opentxs/api/Core.hpp"
#include "opentxs/api/Factory.hpp"
#include "opentxs/api/Wallet.hpp"
#include "opentxs/api/client/Blockchain.hpp"
#include "opentxs/api/storage/Storage.hpp"
#include "opentxs/blockchain/Network.hpp"
#include "opentxs/core/Account.hpp"
#include "opentxs/core/Identifier.hpp"
#include "opentxs/core/identifier/Nym.hpp"
#include "opentxs/core/identifier/UnitDefinition.hpp"
#include "opentxs/rpc/AccountData.hpp"
#include "opentxs/rpc/AccountType.hpp"
#include "opentxs/rpc/ResponseCode.hpp"
#include "opentxs/rpc/request/Base.hpp"
#include "opentxs/rpc/request/GetAccountBalance.hpp"
#include "opentxs/rpc/response/Base.hpp"
#include "opentxs/rpc/response/GetAccountBalance.hpp"
#include "rpc/RPC.hpp"

namespace opentxs::rpc::implementation
{
auto RPC::get_account_balance(const request::Base& base) const noexcept
    -> response::Base
{
    const auto& in = base.asGetAccountBalance();
    auto codes = response::Base::Responses{};
    auto balances = response::GetAccountBalance::Data{};
    const auto reply = [&] {
        return response::GetAccountBalance{
            in, std::move(codes), std::move(balances)};
    };

    try {
        const auto& api = session(base);

        for (const auto& id : in.Accounts()) {
            const auto index = codes.size();

            if (id.empty()) {
                codes.emplace_back(index, ResponseCode::invalid);

                continue;
            }

            const auto accountID = api.Factory().Identifier(id);

            if (is_blockchain_account(base, accountID)) {
                get_account_balance_blockchain(
                    base, index, accountID, balances, codes);
            } else {
                get_account_balance_custodial(
                    api, index, accountID, balances, codes);
            }
        }
    } catch (...) {
        codes.emplace_back(0, ResponseCode::bad_session);
    }

    return reply();
}

auto RPC::get_account_balance_blockchain(
    const request::Base& base,
    const std::size_t index,
    const Identifier& accountID,
    std::vector<AccountData>& balances,
    response::Base::Responses& codes) const noexcept -> void
{
    try {
        const auto& api = client_session(base);
        const auto& blockchain = api.Blockchain();
        const auto [chain, owner] = blockchain.LookupAccount(accountID);
        blockchain.Start(chain);
        const auto& client = blockchain.GetChain(chain);
        const auto [confirmed, unconfirmed] = client.GetBalance(owner);
        balances.emplace_back(
            accountID.str(),
            blockchain::AccountName(chain),
            blockchain::UnitID(api, chain).str(),
            owner->str(),
            blockchain::IssuerID(api, chain).str(),
            confirmed,
            unconfirmed,
            AccountType::blockchain);
        codes.emplace_back(index, ResponseCode::success);
    } catch (...) {
        codes.emplace_back(index, ResponseCode::account_not_found);
    }
}

auto RPC::get_account_balance_custodial(
    const api::Core& api,
    const std::size_t index,
    const Identifier& accountID,
    std::vector<AccountData>& balances,
    response::Base::Responses& codes) const noexcept -> void
{
    const auto account = api.Wallet().Account(accountID);

    if (account) {
        balances.emplace_back(
            accountID.str(),
            account.get().Alias(),
            account.get().GetInstrumentDefinitionID().str(),
            api.Storage().AccountOwner(accountID)->str(),
            api.Storage().AccountIssuer(accountID)->str(),
            account.get().GetBalance(),
            account.get().GetBalance(),
            (account.get().IsIssuer()) ? AccountType::issuer
                                       : AccountType::normal);
        codes.emplace_back(index, ResponseCode::success);
    } else {
        codes.emplace_back(index, ResponseCode::account_not_found);
    }
}
}  // namespace opentxs::rpc::implementation
