// Copyright (c) 2010-2021 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

// IWYU pragma: no_include "opentxs/rpc/CommandType.hpp"

#ifndef OPENTXS_RPC_REQUEST_BASE_HPP
#define OPENTXS_RPC_REQUEST_BASE_HPP

#include "opentxs/Version.hpp"  // IWYU pragma: associated

#include <string>
#include <vector>

#include "opentxs/Bytes.hpp"
#include "opentxs/Types.hpp"
#include "opentxs/rpc/Types.hpp"

namespace opentxs
{
namespace proto
{
class RPCCommand;
}  // namespace proto

namespace rpc
{
namespace request
{
class Base;
class GetAccountActivity;
class GetAccountBalance;
class ListAccounts;
class ListNyms;
class SendPayment;
}  // namespace request
}  // namespace rpc
}  // namespace opentxs

namespace opentxs
{
namespace rpc
{
namespace request
{
auto Factory(ReadView serialized) noexcept -> Base;
auto Factory(const proto::RPCCommand& serialized) noexcept -> Base;

class OPENTXS_EXPORT Base
{
public:
    using SessionIndex = int;
    using Identifiers = std::vector<std::string>;
    using AssociateNyms = Identifiers;

    struct Imp;

    auto asGetAccountActivity() const noexcept -> const GetAccountActivity&;
    auto asGetAccountBalance() const noexcept -> const GetAccountBalance&;
    auto asListAccounts() const noexcept -> const ListAccounts&;
    auto asListNyms() const noexcept -> const ListNyms&;
    auto asSendPayment() const noexcept -> const SendPayment&;

    auto AssociatedNyms() const noexcept -> const AssociateNyms&;
    auto Cookie() const noexcept -> const std::string&;
    auto Serialize(AllocateOutput dest) const noexcept -> bool;
    OPENTXS_NO_EXPORT auto Serialize(proto::RPCCommand& dest) const noexcept
        -> bool;
    auto Session() const noexcept -> SessionIndex;
    auto Type() const noexcept -> CommandType;
    auto Version() const noexcept -> VersionNumber;

    Base() noexcept;
    Base(const Base&) noexcept;
    Base(Base&&) noexcept;
    auto operator=(const Base&) noexcept -> Base&;
    auto operator=(Base&&) noexcept -> Base&;

    virtual ~Base();

protected:
    Imp* imp_;

    Base(Imp* imp) noexcept;
};
}  // namespace request
}  // namespace rpc
}  // namespace opentxs
#endif
