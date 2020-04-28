// Copyright (c) 2010-2020 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef OPENTXS_BLIND_TOKEN_HPP
#define OPENTXS_BLIND_TOKEN_HPP

// IWYU pragma: no_include "opentxs/Proto.hpp"

#include "opentxs/Forward.hpp"  // IWYU pragma: associated

#include <chrono>
#include <cstdint>

#if OT_CASH
#include "opentxs/Pimpl.hpp"
#include "opentxs/Proto.hpp"
#include "opentxs/protobuf/CashEnums.pb.h"

namespace opentxs
{
namespace blind
{
class Token;
}  // namespace blind

namespace proto
{
class Token;
}  // namespace proto

using OTToken = Pimpl<blind::Token>;
}  // namespace opentxs

namespace opentxs
{
namespace blind
{
class Token
{
public:
    using Clock = std::chrono::system_clock;
    using Time = Clock::time_point;
    //using Denomination = OTAmount;
    using MintSeries = std::uint64_t;

    OPENTXS_EXPORT virtual std::string ID(
        const PasswordPrompt& reason) const = 0;
    OPENTXS_EXPORT virtual bool IsSpent(const PasswordPrompt& reason) const = 0;
    OPENTXS_EXPORT virtual const identifier::Server& Notary() const = 0;
    OPENTXS_EXPORT virtual Purse& Owner() const noexcept = 0;
    OPENTXS_EXPORT virtual proto::Token Serialize() const = 0;
    OPENTXS_EXPORT virtual MintSeries Series() const = 0;
    OPENTXS_EXPORT virtual proto::TokenState State() const = 0;
    OPENTXS_EXPORT virtual proto::CashType Type() const = 0;
    OPENTXS_EXPORT virtual const identifier::UnitDefinition& Unit() const = 0;
    OPENTXS_EXPORT virtual Time ValidFrom() const = 0;
    OPENTXS_EXPORT virtual Time ValidTo() const = 0;
    OPENTXS_EXPORT virtual const Amount& Value() const = 0;

    OPENTXS_EXPORT virtual bool ChangeOwner(
        Purse& oldOwner,
        Purse& newOwner,
        const PasswordPrompt& reason) = 0;
    OPENTXS_EXPORT virtual bool MarkSpent(const PasswordPrompt& reason) = 0;
    OPENTXS_EXPORT virtual bool Process(
        const identity::Nym& owner,
        const Mint& mint,
        const PasswordPrompt& reason) = 0;

    OPENTXS_EXPORT virtual ~Token() = default;

protected:
    Token() = default;

private:
    friend OTToken;

    OPENTXS_EXPORT virtual Token* clone() const noexcept = 0;

    Token(const Token&) = delete;
    Token(Token&&) = delete;
    Token& operator=(const Token&) = delete;
    Token& operator=(Token&&) = delete;
};
}  // namespace blind
}  // namespace opentxs
#endif  // OT_CASH
#endif
