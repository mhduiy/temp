// SPDX-FileCopyrightText: 2025 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#include <QObject>

class PasskeyController : public QObject
{
    Q_OBJECT

public:
    explicit PasskeyController(QObject *parent = nullptr);
    ~PasskeyController();

private:

};