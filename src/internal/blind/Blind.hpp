// Copyright (c) 2010-2021 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

// IWYU pragma: no_include "opentxs/blind/CashType.hpp"
// IWYU pragma: no_include "opentxs/blind/PurseType.hpp"
// IWYU pragma: no_include "opentxs/blind/TokenState.hpp"

#pragma once

#include <map>

#include "opentxs/blind/Types.hpp"
#include "opentxs/protobuf/CashEnums.pb.h"

namespace opentxs::blind::internal
{
using CashTypeMap = std::map<blind::CashType, proto::CashType>;
using CashTypeReverseMap = std::map<proto::CashType, blind::CashType>;
using PurseTypeMap = std::map<blind::PurseType, proto::PurseType>;
using PurseTypeReverseMap = std::map<proto::PurseType, blind::PurseType>;
using TokenStateMap = std::map<blind::TokenState, proto::TokenState>;
using TokenStateReverseMap = std::map<proto::TokenState, blind::TokenState>;

auto cashtype_map() noexcept -> const CashTypeMap&;
auto pursetype_map() noexcept -> const PurseTypeMap&;
auto tokenstate_map() noexcept -> const TokenStateMap&;
OPENTXS_EXPORT auto translate(const blind::CashType in) noexcept
    -> proto::CashType;
OPENTXS_EXPORT auto translate(const blind::PurseType in) noexcept
    -> proto::PurseType;
OPENTXS_EXPORT auto translate(const blind::TokenState in) noexcept
    -> proto::TokenState;
OPENTXS_EXPORT auto translate(const proto::CashType in) noexcept
    -> blind::CashType;
OPENTXS_EXPORT auto translate(const proto::PurseType in) noexcept
    -> blind::PurseType;
OPENTXS_EXPORT auto translate(const proto::TokenState in) noexcept
    -> blind::TokenState;
}  // namespace opentxs::blind::internal
