// Copyright (c) 2010-2020 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef OPENTXS_UI_ACTIVITYTHREAD_HPP
#define OPENTXS_UI_ACTIVITYTHREAD_HPP

#ifndef Q_MOC_RUN

// IWYU pragma: no_include "opentxs/Proto.hpp"

#include "opentxs/Forward.hpp"  // IWYU pragma: associated

#include <string>

#include "opentxs/Proto.hpp"
#include "opentxs/SharedPimpl.hpp"
#include "opentxs/ui/List.hpp"

#ifdef SWIG
// clang-format off
%extend opentxs::ui::ActivityThread {
    std::string PaymentCode(const int currency) const
    {
        return $self->PaymentCode(
            static_cast<opentxs::proto::ContactItemType>(currency));
    }
}
%ignore opentxs::ui::ActivityThread::PaymentCode;
%rename(UIActivityThread) opentxs::ui::ActivityThread;
// clang-format on
#endif  // SWIG

namespace opentxs
{
namespace ui
{
namespace implementation
{
class ActivityThread;
}  // namespace implementation

class ActivityThread;
class ActivityThreadItem;

#if OT_QT
class ActivityThreadQt;
#endif
}  // namespace ui
}  // namespace opentxs

namespace opentxs
{
namespace ui
{
class ActivityThread : virtual public List
{
public:
    OPENTXS_EXPORT virtual std::string DisplayName() const noexcept = 0;
    OPENTXS_EXPORT virtual opentxs::SharedPimpl<opentxs::ui::ActivityThreadItem>
    First() const noexcept = 0;
    OPENTXS_EXPORT virtual opentxs::SharedPimpl<opentxs::ui::ActivityThreadItem>
    Next() const noexcept = 0;
    OPENTXS_EXPORT virtual std::string GetDraft() const noexcept = 0;
    OPENTXS_EXPORT virtual std::string Participants() const noexcept = 0;
    OPENTXS_EXPORT virtual bool Pay(
        const std::string& amount,
        const Identifier& sourceAccount,
        const std::string& memo = "",
        const PaymentType type = PaymentType::Cheque) const noexcept = 0;
    OPENTXS_EXPORT virtual bool Pay(
        const Amount& amount,
        const Identifier& sourceAccount,
        const std::string& memo = "",
        const PaymentType type = PaymentType::Cheque) const noexcept = 0;
    OPENTXS_EXPORT virtual std::string PaymentCode(
        const proto::ContactItemType currency) const noexcept = 0;
    OPENTXS_EXPORT virtual bool SendDraft() const noexcept = 0;
    OPENTXS_EXPORT virtual bool SetDraft(
        const std::string& draft) const noexcept = 0;
    OPENTXS_EXPORT virtual std::string ThreadID() const noexcept = 0;

    OPENTXS_EXPORT ~ActivityThread() override = default;

protected:
    ActivityThread() noexcept = default;

private:
    ActivityThread(const ActivityThread&) = delete;
    ActivityThread(ActivityThread&&) = delete;
    ActivityThread& operator=(const ActivityThread&) = delete;
    ActivityThread& operator=(ActivityThread&&) = delete;
};
}  // namespace ui
}  // namespace opentxs
#endif

#if OT_QT || defined(Q_MOC_RUN)
class OPENTXS_EXPORT opentxs::ui::ActivityThreadQt final
    : public QIdentityProxyModel
{
    Q_OBJECT
    Q_PROPERTY(QString displayName READ displayName NOTIFY updated)
    Q_PROPERTY(QString draft READ getDraft NOTIFY updated)
    Q_PROPERTY(QString participants READ participants NOTIFY updated)
    Q_PROPERTY(QString threadID READ threadID NOTIFY updated)

signals:
    void updated() const;

public:
    // Table layout
    enum Roles {
        PolarityRole = Qt::UserRole + 0,
        TypeRole = Qt::UserRole + 1,
    };
    enum Columns {
        TextColumn = 0,
        AmountColumn = 1,
        MemoColumn = 2,
        TimeColumn = 3,
        LoadingColumn = 4,
        PendingColumn = 5,
    };

    QString displayName() const noexcept;
    QString getDraft() const noexcept;
    QString participants() const noexcept;
    QString threadID() const noexcept;
    Q_INVOKABLE bool pay(
        const QString& amount,
        const QString& sourceAccount,
        const QString& memo = "") const noexcept;
    Q_INVOKABLE QString paymentCode(const int currency) const noexcept;
    Q_INVOKABLE bool sendDraft() const noexcept;
    Q_INVOKABLE bool setDraft(const QString& draft) const noexcept;

    ActivityThreadQt(implementation::ActivityThread& parent) noexcept;

    ~ActivityThreadQt() final = default;

private:
    friend opentxs::Factory;

    implementation::ActivityThread& parent_;

    void notify() const noexcept;

    ActivityThreadQt() = delete;
    ActivityThreadQt(const ActivityThreadQt&) = delete;
    ActivityThreadQt(ActivityThreadQt&&) = delete;
    ActivityThreadQt& operator=(const ActivityThreadQt&) = delete;
    ActivityThreadQt& operator=(ActivityThreadQt&&) = delete;
};
#endif
#endif
