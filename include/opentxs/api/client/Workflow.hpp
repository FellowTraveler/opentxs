// Copyright (c) 2010-2020 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef OPENTXS_API_CLIENT_WORKFLOW_HPP
#define OPENTXS_API_CLIENT_WORKFLOW_HPP

#include "opentxs/Forward.hpp"

#include "opentxs/Proto.hpp"

#include <memory>
#include <vector>

namespace opentxs
{
namespace api
{
namespace internal
{
struct Core;
}  // namespace internal

namespace client
{
/** Store and retrieve payment workflow events
 *
 *  Sequence for sending a cheque:
 *    1. WriteCheque
 *    2. SendCheque or ExportCheque
 *       a. SendCheque may be repeated as necessary until successful
 *    3. (optional) ExpireCheque
 *       a. May be performed after step 1 or after step 2
 *       b. It's possible that a cheque that's been expired locally was already
 *          accepted by the notary
 *    4. (optional) CancelCheque
 *       a. May be performed after step 1 or after step 2
 *    5. ClearCheque
 *       a. Called after receiving a cheque deposit receipt
 *    6. FinishCheque
 *       a. Called after a process inbox attempt
 *       b. May be repeated as necessary until successful
 *
 *  Sequence for receiving a cheque:
 *    1. ReceiveCheque or ImportCheque
 *    2. (optional) ExpireCheque
 *    3. DepositCheque
 *       a. May be repeated as necessary until successful
 *
 *  Sequence for sending a transfer:
 *    1. CreateTransfer
 *       a. Called just before sending a notarizeTransaction message to notary.
 *    2. AcknowledgeTransfer
 *       a. Called after receiving a server response with a successful
 *          transaction reply
 *    3. AbortTransfer
 *       a. Called after receiving a server response with an unsuccessful
 *          transaction reply, or after proving the server did not receive the
 *          original transaction
 *    4. ClearTransfer
 *       a. Called after receiving a transfer receipt
 *    5. CompleteTransfer
 *       a. Called after a process inbox attempt
 *       b. May be repeated as necessary until successful
 *
 *  Sequence for receiving a transfer:
 *    1. ConveyTransfer
 *       a. Called after receiving a transfer
 *    3. AcceptTransfer
 *       a. May be repeated as necessary until successful
 *
 *  Sequence for an internal transfer:
 *    NOTE: AcknowledgeTransfer and ConveyTransfer may be called out of order
 *
 *    1. CreateTransfer
 *       a. Called just before sending a notarizeTransaction message to notary.
 *    2. AcknowledgeTransfer
 *       a. Called after receiving a server response with a successful
 *          transaction reply
 *    3. AbortTransfer
 *       a. Called after receiving a server response with an unsuccessful
 *          transaction reply, or after proving the server did not receive the
 *          original transaction
 *    4. ConveyTransfer
 *       a. Called after receiving a transfer
 *    5. ClearTransfer
 *       a. Called after receiving a transfer receipt
 *    6. CompleteTransfer
 *       a. Called after a process inbox attempt
 *       b. May be repeated as necessary until successful
 *
 *  Sequence for sending cash
 *
 *    1. AllocateCash
 *    2. SendCash
 *
 *  Sequence for receiving cash
 *
 *    1. ReceiveCash
 *    2. AcceptCash
 *    2. RejectCash
 *
 *  Sequence for sending a voucher:
 *    1. Create Voucher
 *    2. Accept Voucher
 *    3. SendVoucher or ExportVoucher
 *       a. SendVoucher may be repeated as necessary until successful
 *    4. (optional) ExpireVoucher
 *       a. May be performed after step 1 or after step 2
 *       b. It's possible that a voucher that's been expired locally was already
 *          accepted by the notary
 *    5. (optional) CancelVoucher
 *       a. May be performed after step 1 or after step 2
 *    6. ClearVoucher
 *       a. Called after receiving a voucher deposit notification
 *
 *  Sequence for receiving a voucher:
 *    1. ReceiveVoucher or ImportVoucher
 *    2. (optional) ExpireVoucher
 *    3. DepositVoucher
 *       a. May be repeated as necessary until successful
 *
 *  Sequence for sending an invoice:
 *    1. WriteInvoice
 *    2. SendInvoice or ExportInvoice
 *       a. SendInvoice may be repeated as necessary until successful
 *    3. (optional) ExpireInvoice
 *       a. May be performed after step 1 or after step 2
 *       b. It's possible that an invoice that's been expired locally was
 *          already accepted by the notary
 *    4. (optional) CancelInvoice
 *       a. May be performed after step 1 or after step 2
 *    5. ClearInvoice
 *       a. Called after receiving a paid invoice receipt
 *    6. FinishInvoice
 *       a. Called after a process inbox attempt
 *       b. May be repeated as necessary until successful
 *
 *  Sequence for receiving an invoice:
 *    1. ReceiveInvoice or ImportInvoice
 *    2. (optional) ExpireInvoice
 *    3. PayInvoice
 *       a. May be repeated as necessary until successful
 *
 */
class Workflow
{
public:
    using Cheque = std::
        pair<proto::PaymentWorkflowState, std::unique_ptr<opentxs::Cheque>>;
//    using Invoice = std::
//        pair<proto::PaymentWorkflowState, std::unique_ptr<opentxs::Cheque>>;
//    using Voucher = std::
//        pair<proto::PaymentWorkflowState, std::unique_ptr<opentxs::Cheque>>;
#if OT_CASH
    using Purse =
        std::pair<proto::PaymentWorkflowState, std::unique_ptr<blind::Purse>>;
#endif
    using Transfer =
        std::pair<proto::PaymentWorkflowState, std::unique_ptr<opentxs::Item>>;

#if OT_CASH
    OPENTXS_EXPORT static bool ContainsCash(
        const proto::PaymentWorkflow& workflow);
#endif
    OPENTXS_EXPORT static bool ContainsCheque(
        const proto::PaymentWorkflow& workflow);
    OPENTXS_EXPORT static bool ContainsOnlyCheque(
        const proto::PaymentWorkflow& workflow);
    OPENTXS_EXPORT static bool ContainsOnlyInvoice(
        const proto::PaymentWorkflow& workflow);
    OPENTXS_EXPORT static bool ContainsTransfer(
        const proto::PaymentWorkflow& workflow);
    OPENTXS_EXPORT static bool ContainsOnlyVoucher(
        const proto::PaymentWorkflow& workflow);
    OPENTXS_EXPORT static std::string ExtractCheque(
        const proto::PaymentWorkflow& workflow);
    OPENTXS_EXPORT static std::string ExtractInvoice(
        const proto::PaymentWorkflow& workflow);
    OPENTXS_EXPORT static std::string ExtractVoucher(
        const proto::PaymentWorkflow& workflow);
#if OT_CASH
    OPENTXS_EXPORT static std::unique_ptr<proto::Purse> ExtractPurse(
        const proto::PaymentWorkflow& workflow);
#endif
    OPENTXS_EXPORT static std::string ExtractTransfer(
        const proto::PaymentWorkflow& workflow);
    OPENTXS_EXPORT static Cheque InstantiateCheque(
        const api::internal::Core& api,
        const proto::PaymentWorkflow& workflow);
    OPENTXS_EXPORT static Cheque InstantiateInvoice(
        const api::internal::Core& api,
        const proto::PaymentWorkflow& workflow);
    OPENTXS_EXPORT static Cheque InstantiateVoucher(
        const api::internal::Core& api,
        const proto::PaymentWorkflow& workflow);
#if OT_CASH
    OPENTXS_EXPORT static Purse InstantiatePurse(
        const api::internal::Core& api,
        const proto::PaymentWorkflow& workflow);
#endif
    OPENTXS_EXPORT static Transfer InstantiateTransfer(
        const api::internal::Core& api,
        const proto::PaymentWorkflow& workflow);
    OPENTXS_EXPORT static OTIdentifier UUID(
        const api::internal::Core& api,
        const proto::PaymentWorkflow& workflown);
    OPENTXS_EXPORT static OTIdentifier UUID(
        const Identifier& notary,
        const TransactionNumber& number);

    /** Record a failed transfer attempt */
    OPENTXS_EXPORT virtual bool AbortTransfer(
        const identifier::Nym& nymID,
        const Item& transfer,
        const Message& reply) const = 0;
    /** Record a transfer accept, or accept attempt */
    OPENTXS_EXPORT virtual bool AcceptTransfer(
        const identifier::Nym& nymID,
        const identifier::Server& notaryID,
        const OTTransaction& pending,
        const Message& reply) const = 0;
    /** Record a voucher withdrawal attempt */
    OPENTXS_EXPORT virtual bool AcceptVoucher(
        const identifier::Nym& nymID,
        const opentxs::Cheque& voucher,
        const Message& request,
        const Message* reply) const = 0;
    /** Record a successful transfer attempt */
    OPENTXS_EXPORT virtual bool AcknowledgeTransfer(
        const identifier::Nym& nymID,
        const Item& transfer,
        const Message& reply) const = 0;
#if OT_CASH
    OPENTXS_EXPORT virtual OTIdentifier AllocateCash(
        const identifier::Nym& id,
        const blind::Purse& purse) const = 0;
#endif
    /** Record a cheque cancellation or cancellation attempt */
    OPENTXS_EXPORT virtual bool CancelCheque(
        const opentxs::Cheque& cheque,
        const Message& request,
        const Message* reply) const = 0;
    /** Record an invoice cancellation or cancellation attempt */
    OPENTXS_EXPORT virtual bool CancelInvoice(
        const opentxs::Cheque& invoice,
        const Message& request,
        const Message* reply) const = 0;
    /** Record a voucher cancellation or cancellation attempt */
    OPENTXS_EXPORT virtual bool CancelVoucher(
        const opentxs::Cheque& voucher,
        const Message& request,
        const Message* reply) const = 0;
    /** Record a cheque deposit receipt */
    OPENTXS_EXPORT virtual bool ClearCheque(
        const identifier::Nym& recipientNymID,
        const OTTransaction& receipt) const = 0;
    /** Record an invoice payment receipt */
    OPENTXS_EXPORT virtual bool ClearInvoice(
        const identifier::Nym& recipientNymID,
        const OTTransaction& receipt) const = 0;
    /** Record a voucher deposit notice */
    OPENTXS_EXPORT virtual bool ClearVoucher(
        const identifier::Nym& recipientNymID,
        const OTTransaction& receipt) const = 0;
    /** Record receipt of a transfer receipt */
    OPENTXS_EXPORT virtual bool ClearTransfer(
        const identifier::Nym& nymID,
        const identifier::Server& notaryID,
        const OTTransaction& receipt) const = 0;
    /** Record a process inbox for sender that accepts a transfer receipt */
    OPENTXS_EXPORT virtual bool CompleteTransfer(
        const identifier::Nym& nymID,
        const identifier::Server& notaryID,
        const OTTransaction& receipt,
        const Message& reply) const = 0;
    /** Create a new incoming transfer workflow, or update an existing internal
     *  transfer workflow. */
    OPENTXS_EXPORT virtual OTIdentifier ConveyTransfer(
        const identifier::Nym& nymID,
        const identifier::Server& notaryID,
        const OTTransaction& pending) const = 0;
    /** Record a new outgoing or internal "sent transfer" (or attempt) workflow
     */
    OPENTXS_EXPORT virtual OTIdentifier CreateTransfer(
        const Item& transfer,
        const Message& request) const = 0;
    /** Create a new outgoing voucher workflow */
    OPENTXS_EXPORT virtual OTIdentifier CreateVoucher(
        const opentxs::Cheque& protoVoucher) const = 0;
    /** Record a cheque deposit or deposit attempt */
    OPENTXS_EXPORT virtual bool DepositCheque(
        const identifier::Nym& nymID,
        const Identifier& accountID,
        const opentxs::Cheque& cheque,
        const Message& request,
        const Message* reply) const = 0;
    /** Record an invoice payment or payment attempt */
    OPENTXS_EXPORT virtual bool PayInvoice(
        const identifier::Nym& nymID,
        const Identifier& accountID,
        const opentxs::Cheque& invoice,
        const Message& request,
        const Message* reply) const = 0;
    /** Record a voucher deposit or deposit attempt */
    OPENTXS_EXPORT virtual bool DepositVoucher(
        const identifier::Nym& nymID,
        const Identifier& accountID,
        const opentxs::Cheque& voucher,
        const Message& request,
        const Message* reply) const = 0;
    /** Mark a cheque workflow as expired */
    OPENTXS_EXPORT virtual bool ExpireCheque(
        const identifier::Nym& nymID,
        const opentxs::Cheque& cheque) const = 0;
    /** Mark an invoice workflow as expired */
    OPENTXS_EXPORT virtual bool ExpireInvoice(
        const identifier::Nym& nymID,
        const opentxs::Cheque& invoice) const = 0;
    /** Mark a voucher workflow as expired */
    OPENTXS_EXPORT virtual bool ExpireVoucher(
        const identifier::Nym& nymID,
        const opentxs::Cheque& voucher) const = 0;
    /** Record an out of band cheque conveyance */
    OPENTXS_EXPORT virtual bool ExportCheque(
        const opentxs::Cheque& cheque) const = 0;
    /** Record an out of band invoice conveyance */
    OPENTXS_EXPORT virtual bool ExportInvoice(
        const opentxs::Cheque& invoice) const = 0;
    /** Record an out of band voucher conveyance */
    OPENTXS_EXPORT virtual bool ExportVoucher(
        const opentxs::Cheque& voucher) const = 0;
    /** Record a process inbox that accepts a cheque deposit receipt */
    OPENTXS_EXPORT virtual bool FinishCheque(
        const opentxs::Cheque& cheque,
        const Message& request,
        const Message* reply) const = 0;
    /** Record a process inbox that accepts an invoice payment receipt */
    OPENTXS_EXPORT virtual bool FinishInvoice(
        const opentxs::Cheque& invoice,
        const Message& request,
        const Message* reply) const = 0;
    /** Record a process inbox that accepts a voucher deposit receipt */
    OPENTXS_EXPORT virtual bool FinishVoucher(
        const opentxs::Cheque& voucher,
        const Message& request,
        const Message* reply) const = 0;
    /** Create a new incoming cheque workflow from an out of band cheque */
    OPENTXS_EXPORT virtual OTIdentifier ImportCheque(
        const identifier::Nym& nymID,
        const opentxs::Cheque& cheque) const = 0;
    /** Create a new incoming invoice workflow from an out of band invoice */
    OPENTXS_EXPORT virtual OTIdentifier ImportInvoice(
        const identifier::Nym& nymID,
        const opentxs::Cheque& invoice) const = 0;
    /** Create a new incoming voucher workflow from an out of band voucher */
    OPENTXS_EXPORT virtual OTIdentifier ImportVoucher(
        const identifier::Nym& nymID,
        const opentxs::Cheque& voucher) const = 0;
    OPENTXS_EXPORT virtual std::set<OTIdentifier> List(
        const identifier::Nym& nymID,
        const proto::PaymentWorkflowType type,
        const proto::PaymentWorkflowState state) const = 0;
    OPENTXS_EXPORT virtual Cheque LoadCheque(
        const identifier::Nym& nymID,
        const Identifier& chequeID) const = 0;
    OPENTXS_EXPORT virtual Cheque LoadChequeByWorkflow(
        const identifier::Nym& nymID,
        const Identifier& workflowID) const = 0;
    OPENTXS_EXPORT virtual Cheque LoadInvoice(
        const identifier::Nym& nymID,
        const Identifier& invoiceID) const = 0;
    OPENTXS_EXPORT virtual Cheque LoadInvoiceByWorkflow(
        const identifier::Nym& nymID,
        const Identifier& workflowID) const = 0;
    OPENTXS_EXPORT virtual Cheque LoadVoucher(
        const identifier::Nym& nymID,
        const Identifier& voucherID) const = 0;
    OPENTXS_EXPORT virtual Cheque LoadVoucherByWorkflow(
        const identifier::Nym& nymID,
        const Identifier& workflowID) const = 0;
    OPENTXS_EXPORT virtual Transfer LoadTransfer(
        const identifier::Nym& nymID,
        const Identifier& transferID) const = 0;
    OPENTXS_EXPORT virtual Transfer LoadTransferByWorkflow(
        const identifier::Nym& nymID,
        const Identifier& workflowID) const = 0;
    /** Load a serialized workflow, if it exists*/
    OPENTXS_EXPORT virtual std::shared_ptr<proto::PaymentWorkflow> LoadWorkflow(
        const identifier::Nym& nymID,
        const Identifier& workflowID) const = 0;
#if OT_CASH
    OPENTXS_EXPORT virtual OTIdentifier ReceiveCash(
        const identifier::Nym& receiver,
        const blind::Purse& purse,
        const Message& message) const = 0;
#endif
    /** Create a new incoming cheque workflow from an OT message */
    OPENTXS_EXPORT virtual OTIdentifier ReceiveCheque(
        const identifier::Nym& nymID,
        const opentxs::Cheque& cheque,
        const Message& message) const = 0;
    /** Create a new incoming invoice workflow from an OT message */
    OPENTXS_EXPORT virtual OTIdentifier ReceiveInvoice(
        const identifier::Nym& nymID,
        const opentxs::Cheque& invoice,
        const Message& message) const = 0;
    /** Create a new incoming voucher workflow from an OT message */
    OPENTXS_EXPORT virtual OTIdentifier ReceiveVoucher(
        const identifier::Nym& nymID,
        const opentxs::Cheque& voucher,
        const Message& message) const = 0;
#if OT_CASH
    OPENTXS_EXPORT virtual bool SendCash(
        const identifier::Nym& sender,
        const identifier::Nym& recipient,
        const Identifier& workflowID,
        const Message& request,
        const Message* reply) const = 0;
#endif
    /** Record a cheque send or send attempt via an OT notary */
    OPENTXS_EXPORT virtual bool SendCheque(
        const opentxs::Cheque& cheque,
        const Message& request,
        const Message* reply) const = 0;
    /** Record an invoice send or send attempt via an OT notary */
    OPENTXS_EXPORT virtual bool SendInvoice(
        const opentxs::Cheque& invoice,
        const Message& request,
        const Message* reply) const = 0;
    /** Record a voucher send or send attempt via an OT notary */
    OPENTXS_EXPORT virtual bool SendVoucher(
        const opentxs::Cheque& voucher,
        const Message& request,
        const Message* reply) const = 0;
    /** Get a list of workflow IDs relevant to a specified account */
    OPENTXS_EXPORT virtual std::vector<OTIdentifier> WorkflowsByAccount(
        const identifier::Nym& nymID,
        const Identifier& accountID) const = 0;
    /** Create a new outgoing cheque workflow */
    OPENTXS_EXPORT virtual OTIdentifier WriteCheque(
        const opentxs::Cheque& cheque) const = 0;
    /** Create a new outgoing invoice workflow */
    OPENTXS_EXPORT virtual OTIdentifier WriteInvoice(
        const opentxs::Cheque& invoice) const = 0;

    OPENTXS_EXPORT virtual ~Workflow() = default;

protected:
    Workflow() = default;

private:
    Workflow(const Workflow&) = delete;
    Workflow(Workflow&&) = delete;
    Workflow& operator=(const Workflow&) = delete;
    Workflow& operator=(Workflow&&) = delete;
};
}  // namespace client
}  // namespace api
}  // namespace opentxs
#endif
