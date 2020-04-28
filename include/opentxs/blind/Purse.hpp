// Copyright (c) 2010-2020 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef OPENTXS_BLIND_PURSE_HPP
#define OPENTXS_BLIND_PURSE_HPP

// IWYU pragma: no_include "opentxs/Proto.hpp"

#include "opentxs/Forward.hpp"  // IWYU pragma: associated

#include <chrono>
#include <cstdint>

#if OT_CASH
#include "opentxs/Pimpl.hpp"
#include "opentxs/Proto.hpp"
#include "opentxs/iterator/Bidirectional.hpp"
#include "opentxs/protobuf/CashEnums.pb.h"

namespace opentxs
{
namespace api
{
namespace server
{
namespace internal
{
struct Manager;
}  // namespace internal
}  // namespace server
}  // namespace api

namespace blind
{
class Purse;
}  // namespace blind

namespace proto
{
class Purse;
}  // namespace proto

using OTPurse = Pimpl<blind::Purse>;
}  // namespace opentxs

namespace opentxs
{
namespace blind
{
class Purse
{
public:
    using Clock = std::chrono::system_clock;
    using Time = Clock::time_point;
    using iterator = opentxs::iterator::Bidirectional<Purse, Token>;
    using const_iterator =
        opentxs::iterator::Bidirectional<const Purse, const Token>;

    OPENTXS_EXPORT virtual const Token& at(
        const std::size_t position) const = 0;
    OPENTXS_EXPORT virtual const_iterator begin() const noexcept = 0;
    OPENTXS_EXPORT virtual const_iterator cbegin() const noexcept = 0;
    OPENTXS_EXPORT virtual const_iterator cend() const noexcept = 0;
    OPENTXS_EXPORT virtual Time EarliestValidTo() const = 0;
    OPENTXS_EXPORT virtual const_iterator end() const noexcept = 0;
    OPENTXS_EXPORT virtual bool IsUnlocked() const = 0;
    OPENTXS_EXPORT virtual Time LatestValidFrom() const = 0;
    OPENTXS_EXPORT virtual const identifier::Server& Notary() const = 0;
    OPENTXS_EXPORT virtual proto::Purse Serialize() const = 0;
    OPENTXS_EXPORT virtual std::size_t size() const noexcept = 0;
    OPENTXS_EXPORT virtual proto::PurseType State() const = 0;
    OPENTXS_EXPORT virtual proto::CashType Type() const = 0;
    OPENTXS_EXPORT virtual const identifier::UnitDefinition& Unit() const = 0;
    OPENTXS_EXPORT virtual bool Unlock(
        const identity::Nym& nym,
        const PasswordPrompt& reason) const = 0;
    OPENTXS_EXPORT virtual bool Verify(
        const api::server::internal::Manager& server) const = 0;
    OPENTXS_EXPORT virtual const Amount& Value() const = 0;

    OPENTXS_EXPORT virtual bool AddNym(
        const identity::Nym& nym,
        const PasswordPrompt& reason) = 0;
    OPENTXS_EXPORT virtual Token& at(const std::size_t position) = 0;
    OPENTXS_EXPORT virtual iterator begin() noexcept = 0;
    OPENTXS_EXPORT virtual iterator end() noexcept = 0;
    OPENTXS_EXPORT virtual crypto::key::Symmetric& PrimaryKey(
        PasswordPrompt& password) noexcept(false) = 0;
    OPENTXS_EXPORT virtual std::shared_ptr<Token> Pop() = 0;
    OPENTXS_EXPORT virtual bool Process(
        const identity::Nym& owner,
        const Mint& mint,
        const PasswordPrompt& reason) = 0;
    OPENTXS_EXPORT virtual bool Push(
        std::shared_ptr<Token> token,
        const PasswordPrompt& reason) = 0;
    OPENTXS_EXPORT virtual const crypto::key::Symmetric& SecondaryKey(
        const identity::Nym& owner,
        PasswordPrompt& password) = 0;

    OPENTXS_EXPORT virtual ~Purse() = default;

protected:
    Purse() noexcept = default;

private:
    friend OTPurse;

    OPENTXS_EXPORT virtual Purse* clone() const noexcept = 0;

    Purse(const Purse&) = delete;
    Purse(Purse&&) = delete;
    Purse& operator=(const Purse&) = delete;
    Purse& operator=(Purse&&) = delete;
};
}  // namespace blind
}  // namespace opentxs
#endif  // OT_CASH
#endif
