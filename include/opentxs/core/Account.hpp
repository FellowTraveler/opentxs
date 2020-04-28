// Copyright (c) 2010-2020 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef OPENTXS_CORE_ACCOUNT_HPP
#define OPENTXS_CORE_ACCOUNT_HPP

#include "opentxs/Forward.hpp"  // IWYU pragma: associated

#include <irrxml/irrXML.hpp>
#include <cstdint>
#include <memory>
#include <string>

#include "opentxs/Exclusive.hpp"
#include "opentxs/Shared.hpp"
#include "opentxs/Types.hpp"
#include "opentxs/core/Contract.hpp"
#include "opentxs/core/Identifier.hpp"
#include "opentxs/core/Ledger.hpp"
#include "opentxs/core/OTTransactionType.hpp"
#include "opentxs/core/String.hpp"
#include "opentxs/core/identifier/UnitDefinition.hpp"

namespace opentxs
{
namespace api
{
namespace implementation
{
class Wallet;
}  // namespace implementation

namespace internal
{
struct Core;
}  // namespace internal

namespace server
{
namespace implementation
{
class Wallet;
}  // namespace implementation
}  // namespace server
}  // namespace api

namespace identifier
{
class Nym;
class Server;
}  // namespace identifier

namespace identity
{
class Nym;
}  // namespace identity

namespace otx
{
namespace context
{
class Base;
}  // namespace context
}  // namespace otx

class Account;
class Ledger;
class OTWallet;
class PasswordPrompt;
class Tag;
}  // namespace opentxs

namespace opentxs
{
using ExclusiveAccount = Exclusive<Account>;
using SharedAccount = Shared<Account>;

class Account : public OTTransactionType
{
public:
    // If you add any types to this list, update the list of strings at the
    // top of the .cpp file.
    enum AccountType {
        user,       // used by users
        issuer,     // used by issuers    (these can only go negative.)
        basket,     // issuer acct used by basket currencies (these can only go
                    // negative)
        basketsub,  // used by the server (to store backing reserves for basket
                    // sub-accounts)
        mint,       // used by mints (to store backing reserves for cash)
        voucher,    // used by the server (to store backing reserves for
                    // vouchers)
        stash,  // used by the server (to store backing reserves for stashes,
                // for smart contracts.)
        err_acct
    };

    OPENTXS_EXPORT static char const* _GetTypeString(AccountType accountType);

    OPENTXS_EXPORT std::string Alias() const;
    OPENTXS_EXPORT bool ConsensusHash(
        const otx::context::Base& context,
        Identifier& theOutput,
        const PasswordPrompt& reason) const;
    OPENTXS_EXPORT bool DisplayStatistics(String& contents) const override;
    OPENTXS_EXPORT const Amount& GetBalance() const;
    OPENTXS_EXPORT const identifier::UnitDefinition& GetInstrumentDefinitionID()
        const;
    OPENTXS_EXPORT TransactionNumber GetStashTransNum() const
    {
        return stashTransNum_;
    }
    OPENTXS_EXPORT char const* GetTypeString() const
    {
        return _GetTypeString(acctType_);
    }
    OPENTXS_EXPORT bool IsAllowedToGoNegative() const;
    OPENTXS_EXPORT bool IsInternalServerAcct() const;
    OPENTXS_EXPORT bool IsOwnedByUser() const;
    OPENTXS_EXPORT bool IsOwnedByEntity() const;
    OPENTXS_EXPORT bool IsIssuer() const;
    // For accounts used by smart contracts, to stash funds while running.
    OPENTXS_EXPORT bool IsStashAcct() const { return (acctType_ == stash); }
    OPENTXS_EXPORT std::unique_ptr<Ledger> LoadInbox(
        const identity::Nym& nym) const;
    OPENTXS_EXPORT std::unique_ptr<Ledger> LoadOutbox(
        const identity::Nym& nym) const;
    // Compares the NymID loaded from the account file with whatever Nym the
    // programmer wants to verify.
    OPENTXS_EXPORT bool VerifyOwner(const identity::Nym& candidate) const;
    OPENTXS_EXPORT bool VerifyOwnerByID(const identifier::Nym& nymId) const;

    // Debit a certain amount from the account (presumably the same amount is
    // being added somewhere)
    OPENTXS_EXPORT bool Debit(const Amount& amount);
    // Credit a certain amount from the account (presumably the same amount is
    // being subtracted somewhere)
    OPENTXS_EXPORT bool Credit(const Amount& amount);
    OPENTXS_EXPORT bool GetInboxHash(Identifier& output);
    OPENTXS_EXPORT bool GetOutboxHash(Identifier& output);
    OPENTXS_EXPORT bool InitBoxes(
        const identity::Nym& signer,
        const PasswordPrompt& reason);
    // If you pass the identifier in, the inbox hash is recorded there
    OPENTXS_EXPORT bool SaveInbox(Ledger& box);
    OPENTXS_EXPORT bool SaveInbox(Ledger& box, Identifier& hash);
    // If you pass the identifier in, the outbox hash is recorded there
    OPENTXS_EXPORT bool SaveOutbox(Ledger& box);
    OPENTXS_EXPORT bool SaveOutbox(Ledger& box, Identifier& hash);
    OPENTXS_EXPORT void SetAlias(const std::string& alias);
    OPENTXS_EXPORT void SetInboxHash(const Identifier& input);
    OPENTXS_EXPORT void SetOutboxHash(const Identifier& input);
    OPENTXS_EXPORT void SetStashTransNum(const TransactionNumber transNum)
    {
        stashTransNum_ = transNum;
    }

    OPENTXS_EXPORT ~Account() override;

private:
    friend OTWallet;
    friend opentxs::api::implementation::Wallet;
    friend opentxs::api::server::implementation::Wallet;

    AccountType acctType_{err_acct};
    // These are all the variables from the account file itself.
    OTUnitID acctInstrumentDefinitionID_;
    OTString balanceDate_;
    OTAmount balanceAmount_;
    // the Transaction Number of a smart contract running on cron, if this is a
    // stash account.
    TransactionNumber stashTransNum_{0};
    // Default FALSE. When set to true, saves a "DELETED" flag with this Account
    bool markForDeletion_{false};
    // for easy cleanup later when the server is doing some maintenance.
    // Hash of this account's Inbox, so we don't download it more often than
    // necessary.
    OTIdentifier inboxHash_;
    // Hash of this account's Outbox, so we don't download it more often than
    // necessary.
    OTIdentifier outboxHash_;
    std::string alias_;

    static Account* GenerateNewAccount(
        const api::internal::Core& api,
        const identifier::Nym& nymID,
        const identifier::Server& notaryID,
        const identity::Nym& serverNym,
        const Identifier& userNymID,
        const identifier::UnitDefinition& instrumentDefinitionID,
        const PasswordPrompt& reason,
        AccountType acctType = user,
        TransactionNumber stashTransNum = 0);
    // Let's say you don't have or know the NymID, and you just want to load
    // the damn thing up. Then call this function. It will set nymID for you.
    static Account* LoadExistingAccount(
        const api::internal::Core& api,
        const Identifier& accountId,
        const identifier::Server& notaryID);

    bool SaveContractWallet(Tag& parent) const override;

    bool create_box(
        std::unique_ptr<Ledger>& box,
        const identity::Nym& signer,
        const ledgerType type,
        const PasswordPrompt& reason);
    bool GenerateNewAccount(
        const identity::Nym& server,
        const Identifier& userNymID,
        const identifier::Server& notaryID,
        const identifier::UnitDefinition& instrumentDefinitionID,
        const PasswordPrompt& reason,
        AccountType acctType = user,
        std::int64_t stashTransNum = 0);
    void InitAccount();
    // overriding this so I can set filename automatically inside based on ID.
    bool LoadContract() override;
    bool LoadContractFromString(const String& theStr) override;
    // return -1 if error, 0 if nothing, and 1 if the node was processed.
    std::int32_t ProcessXMLNode(irr::io::IrrXMLReader*& xml) override;
    void Release() override;
    void Release_Account();
    // generates filename based on accounts path and account ID. Saves to the
    // standard location for an acct.
    bool SaveAccount();

    bool save_box(
        Ledger& box,
        Identifier& hash,
        bool (Ledger::*save)(Identifier&),
        void (Account::*set)(const Identifier&));

    void UpdateContents(const PasswordPrompt& reason) override;

    Account(
        const api::internal::Core& api,
        const identifier::Nym& nymID,
        const Identifier& accountId,
        const identifier::Server& notaryID,
        const String& name);
    Account(
        const api::internal::Core& api,
        const identifier::Nym& nymID,
        const Identifier& accountId,
        const identifier::Server& notaryID);
    Account(
        const api::internal::Core& api,
        const identifier::Nym& nymID,
        const identifier::Server& notaryID);
    Account(const api::internal::Core& api);
    Account() = delete;
};
}  // namespace opentxs
#endif
