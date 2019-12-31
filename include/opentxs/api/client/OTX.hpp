// Copyright (c) 2010-2020 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef OPENTXS_API_CLIENT_OTX_HPP
#define OPENTXS_API_CLIENT_OTX_HPP

#include "opentxs/Forward.hpp"

#include "opentxs/Proto.hpp"
#include "opentxs/Types.hpp"

#include <chrono>
#include <cstdint>
#include <future>
#include <tuple>
#include <memory>
#include <set>

#define OT_CHEQUE_DAYS 30
#define OT_CHEQUE_HOURS 24 * OT_CHEQUE_DAYS
#define DEFAULT_PROCESS_INBOX_ITEMS 5

namespace opentxs
{
namespace api
{
namespace client
{
class OTX
{
public:
    using TaskID = int;
    using MessageID = OTIdentifier;
    using Result = std::pair<proto::LastReplyStatus, std::shared_ptr<Message>>;
    using Future = std::shared_future<Result>;
    using BackgroundTask = std::pair<TaskID, Future>;
    using Finished = std::shared_future<void>;

    OPENTXS_EXPORT virtual BackgroundTask AcknowledgeBailment(
        const identifier::Nym& localNymID,
        const identifier::Server& serverID,
        const identifier::Nym& targetNymID,
        const Identifier& requestID,
        const std::string& instructions,
        const SetID setID = {}) const = 0;
    OPENTXS_EXPORT virtual BackgroundTask AcknowledgeNotice(
        const identifier::Nym& localNymID,
        const identifier::Server& serverID,
        const identifier::Nym& recipientID,
        const Identifier& requestID,
        const bool ack,
        const SetID setID = {}) const = 0;
    OPENTXS_EXPORT virtual BackgroundTask AcknowledgeOutbailment(
        const identifier::Nym& localNymID,
        const identifier::Server& serverID,
        const identifier::Nym& recipientID,
        const Identifier& requestID,
        const std::string& details,
        const SetID setID = {}) const = 0;
    OPENTXS_EXPORT virtual BackgroundTask AcknowledgeConnection(
        const identifier::Nym& localNymID,
        const identifier::Server& serverID,
        const identifier::Nym& recipientID,
        const Identifier& requestID,
        const bool ack,
        const std::string& url,
        const std::string& login,
        const std::string& password,
        const std::string& key,
        const SetID setID = {}) const = 0;
    OPENTXS_EXPORT virtual bool AutoProcessInboxEnabled() const = 0;
    OPENTXS_EXPORT virtual Depositability CanDeposit(
        const identifier::Nym& recipientNymID,
        const OTPayment& payment) const = 0;
    OPENTXS_EXPORT virtual Depositability CanDeposit(
        const identifier::Nym& recipientNymID,
        const Identifier& accountID,
        const OTPayment& payment) const = 0;
    OPENTXS_EXPORT virtual Messagability CanMessage(
        const identifier::Nym& senderNymID,
        const Identifier& recipientContactID,
        const bool startIntroductionServer = true) const = 0;
    OPENTXS_EXPORT virtual bool CheckTransactionNumbers(
        const identifier::Nym& nym,
        const identifier::Server& serverID,
        const std::size_t quantity) const = 0;
    OPENTXS_EXPORT virtual Finished ContextIdle(
        const identifier::Nym& nym,
        const identifier::Server& server) const = 0;
    /** Deposit all available cheques for specified nym
     *
     *  \returns the number of cheques queued for deposit
     */
    OPENTXS_EXPORT virtual std::size_t DepositCheques(
        const identifier::Nym& nymID) const = 0;
    /** Deposit the specified list of cheques for specified nym
     *
     *  If the list of chequeIDs is empty, then all cheques will be deposited
     *
     *  \returns the number of cheques queued for deposit
     */
    OPENTXS_EXPORT virtual std::size_t DepositCheques(
        const identifier::Nym& nymID,
        const std::set<OTIdentifier>& chequeIDs) const = 0;
    /** Pay all available invoices for specified nym
     *
     *  \returns the number of invoices queued for payment
     */
    OPENTXS_EXPORT virtual std::size_t PayInvoices(
        const identifier::Nym& nymID) const = 0;
    /** Deposit the specified list of invoices for specified nym
     *
     *  If the list of invoiceIDs is empty, then all invoices will be paid
     *
     *  \returns the number of invoices queued for payment
     */
    OPENTXS_EXPORT virtual std::size_t PayInvoices(
        const identifier::Nym& nymID,
        const std::set<OTIdentifier>& invoiceIDs) const = 0;
    /** Deposit all available vouchers for specified nym
     *
     *  \returns the number of vouchers queued for deposit
     */
    OPENTXS_EXPORT virtual std::size_t DepositVouchers(
        const identifier::Nym& nymID) const = 0;
    /** Deposit the specified list of vouchers for specified nym
     *
     *  If the list of voucherIDs is empty, then all vouchers will be deposited
     *
     *  \returns the number of vouchers queued for deposit
     */
    OPENTXS_EXPORT virtual std::size_t DepositVouchers(
        const identifier::Nym& nymID,
        const std::set<OTIdentifier>& voucherIDs) const = 0;
    OPENTXS_EXPORT virtual BackgroundTask DepositPayment(
        const identifier::Nym& recipientNymID,
        const std::shared_ptr<const OTPayment>& payment) const = 0;
    OPENTXS_EXPORT virtual BackgroundTask DepositPayment(
        const identifier::Nym& recipientNymID,
        const Identifier& accountID,
        const std::shared_ptr<const OTPayment>& payment) const = 0;
    /** Used by unit tests */
    OPENTXS_EXPORT virtual void DisableAutoaccept() const = 0;
#if OT_CASH
    OPENTXS_EXPORT virtual BackgroundTask DownloadMint(
        const identifier::Nym& nym,
        const identifier::Server& server,
        const identifier::UnitDefinition& unit) const = 0;
#endif
    OPENTXS_EXPORT virtual BackgroundTask DownloadNym(
        const identifier::Nym& localNymID,
        const identifier::Server& serverID,
        const identifier::Nym& targetNymID) const = 0;
    OPENTXS_EXPORT virtual BackgroundTask DownloadNymbox(
        const identifier::Nym& localNymID,
        const identifier::Server& serverID) const = 0;
    OPENTXS_EXPORT virtual BackgroundTask DownloadServerContract(
        const identifier::Nym& localNymID,
        const identifier::Server& serverID,
        const identifier::Server& contractID) const = 0;
    OPENTXS_EXPORT virtual BackgroundTask DownloadUnitDefinition(
        const identifier::Nym& localNymID,
        const identifier::Server& serverID,
        const identifier::UnitDefinition& contractID) const = 0;
    OPENTXS_EXPORT virtual BackgroundTask FindNym(
        const identifier::Nym& nymID) const = 0;
    OPENTXS_EXPORT virtual BackgroundTask FindNym(
        const identifier::Nym& nymID,
        const identifier::Server& serverIDHint) const = 0;
    OPENTXS_EXPORT virtual BackgroundTask FindServer(
        const identifier::Server& serverID) const = 0;
    OPENTXS_EXPORT virtual BackgroundTask FindUnitDefinition(
        const identifier::UnitDefinition& unit) const = 0;
    OPENTXS_EXPORT virtual BackgroundTask InitiateBailment(
        const identifier::Nym& localNymID,
        const identifier::Server& serverID,
        const identifier::Nym& targetNymID,
        const identifier::UnitDefinition& instrumentDefinitionID,
        const SetID setID = {}) const = 0;
    OPENTXS_EXPORT virtual BackgroundTask InitiateOutbailment(
        const identifier::Nym& localNymID,
        const identifier::Server& serverID,
        const identifier::Nym& targetNymID,
        const identifier::UnitDefinition& instrumentDefinitionID,
        const Amount amount,
        const std::string& message,
        const SetID setID = {}) const = 0;
    OPENTXS_EXPORT virtual BackgroundTask InitiateRequestConnection(
        const identifier::Nym& localNymID,
        const identifier::Server& serverID,
        const identifier::Nym& targetNymID,
        const proto::ConnectionInfoType& type,
        const SetID setID = {}) const = 0;
    OPENTXS_EXPORT virtual BackgroundTask InitiateStoreSecret(
        const identifier::Nym& localNymID,
        const identifier::Server& serverID,
        const identifier::Nym& targetNymID,
        const proto::SecretType& type,
        const std::string& primary,
        const std::string& secondary,
        const SetID setID = {}) const = 0;
    OPENTXS_EXPORT virtual const identifier::Server& IntroductionServer()
        const = 0;
    OPENTXS_EXPORT virtual BackgroundTask IssueUnitDefinition(
        const identifier::Nym& localNymID,
        const identifier::Server& serverID,
        const identifier::UnitDefinition& unitID,
        const proto::ContactItemType advertise = proto::CITEMTYPE_ERROR,
        const std::string& label = "") const = 0;
    OPENTXS_EXPORT virtual BackgroundTask MessageContact(
        const identifier::Nym& senderNymID,
        const Identifier& contactID,
        const std::string& message,
        const SetID setID = {}) const = 0;
    OPENTXS_EXPORT virtual std::pair<ThreadStatus, MessageID> MessageStatus(
        const TaskID taskID) const = 0;
    OPENTXS_EXPORT virtual BackgroundTask NotifyBailment(
        const identifier::Nym& localNymID,
        const identifier::Server& serverID,
        const identifier::Nym& targetNymID,
        const identifier::UnitDefinition& instrumentDefinitionID,
        const Identifier& requestID,
        const std::string& txid,
        const Amount amount,
        const SetID setID = {}) const = 0;
    OPENTXS_EXPORT virtual BackgroundTask PayContact(
        const identifier::Nym& senderNymID,
        const Identifier& contactID,
        std::shared_ptr<const OTPayment> payment) const = 0;
#if OT_CASH
    OPENTXS_EXPORT virtual BackgroundTask PayContactCash(
        const identifier::Nym& senderNymID,
        const Identifier& contactID,
        const Identifier& workflowID) const = 0;
#endif  // OT_CASH
    OPENTXS_EXPORT virtual BackgroundTask ProcessInbox(
        const identifier::Nym& localNymID,
        const identifier::Server& serverID,
        const Identifier& accountID) const = 0;
    OPENTXS_EXPORT virtual BackgroundTask PublishServerContract(
        const identifier::Nym& localNymID,
        const identifier::Server& serverID,
        const Identifier& contractID) const = 0;
    OPENTXS_EXPORT virtual void Refresh() const = 0;
    OPENTXS_EXPORT virtual std::uint64_t RefreshCount() const = 0;
    OPENTXS_EXPORT virtual BackgroundTask RegisterAccount(
        const identifier::Nym& localNymID,
        const identifier::Server& serverID,
        const identifier::UnitDefinition& unitID,
        const std::string& label = "") const = 0;
    OPENTXS_EXPORT virtual BackgroundTask RegisterNym(
        const identifier::Nym& localNymID,
        const identifier::Server& serverID,
        const bool resync = false) const = 0;
    OPENTXS_EXPORT virtual BackgroundTask RegisterNymPublic(
        const identifier::Nym& nymID,
        const identifier::Server& server,
        const bool setContactData,
        const bool forcePrimary = false,
        const bool resync = false) const = 0;
    OPENTXS_EXPORT virtual OTServerID SetIntroductionServer(
        const contract::Server& contract) const = 0;
    OPENTXS_EXPORT virtual BackgroundTask SendCheque(
        const identifier::Nym& localNymID,
        const Identifier& sourceAccountID,
        const Identifier& recipientContactID,
        const Amount value,
        const std::string& memo,
        const Time validFrom = Clock::now(),
        const Time validTo =
            (Clock::now() + std::chrono::hours(OT_CHEQUE_HOURS))) const = 0;
    OPENTXS_EXPORT virtual BackgroundTask SendInvoice(
        const identifier::Nym& localNymID,
        const Identifier& sourceAccountID,
        const Identifier& recipientContactID,
        const Amount value,
        const std::string& memo,
        const Time validFrom = Clock::now(),
        const Time validTo =
            (Clock::now() + std::chrono::hours(OT_CHEQUE_HOURS))) const = 0;
    OPENTXS_EXPORT virtual BackgroundTask SendVoucher(
        const identifier::Nym& localNymID,
        const Identifier& sourceAccountID,
        const Identifier& recipientContactID,
        const Amount value,
        const std::string& memo,
        const Time validFrom = Clock::now(),
        const Time validTo =
            (Clock::now() + std::chrono::hours(OT_CHEQUE_HOURS))) const = 0;
    OPENTXS_EXPORT virtual BackgroundTask SendExternalTransfer(
        const identifier::Nym& localNymID,
        const identifier::Server& serverID,
        const Identifier& sourceAccountID,
        const Identifier& targetAccountID,
        const Amount value,
        const std::string& memo) const = 0;
    OPENTXS_EXPORT virtual BackgroundTask SendTransfer(
        const identifier::Nym& localNymID,
        const identifier::Server& serverID,
        const Identifier& sourceAccountID,
        const Identifier& targetAccountID,
        const Amount value,
        const std::string& memo) const = 0;
    OPENTXS_EXPORT virtual void StartIntroductionServer(
        const identifier::Nym& localNymID) const = 0;
    OPENTXS_EXPORT virtual ThreadStatus Status(const TaskID taskID) const = 0;
#if OT_CASH
    OPENTXS_EXPORT virtual BackgroundTask WithdrawCash(
        const identifier::Nym& nymID,
        const identifier::Server& serverID,
        const Identifier& account,
        const Amount value) const = 0;
#endif
    OPENTXS_EXPORT virtual BackgroundTask WithdrawVoucher(
        const identifier::Nym& nymID,
        const Identifier& sourceAccountID,
        const Identifier& recipientContactID,
        const Amount value,
        const std::string& memo,
        const Time validFrom,
        const Time validTo) const = 0;

    OPENTXS_EXPORT virtual ~OTX() = default;

protected:
    OTX() = default;

private:
    OTX(const OTX&) = delete;
    OTX(OTX&&) = delete;
    OTX& operator=(const OTX&) = delete;
    OTX& operator=(OTX&&) = delete;
};
}  // namespace client
}  // namespace api
}  // namespace opentxs
#endif
