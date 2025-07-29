// SPDX-FileCopyrightText: 2025 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#include <QObject>

#include "passkeyworker.h"
#include "passkeymodel.h"

class PasskeyController : public QObject
{
    Q_OBJECT
    Q_PROPERTY(PasskeyWorker *worker MEMBER m_worker CONSTANT)
    Q_PROPERTY(PasskeyModel *model MEMBER m_model CONSTANT)

public:
    explicit PasskeyController(QObject *parent = nullptr);
    ~PasskeyController();

private:
    PasskeyModel *m_model = nullptr;
    PasskeyWorker *m_worker = nullptr;
};
