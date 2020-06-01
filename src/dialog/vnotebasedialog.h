/*
* Copyright (C) 2019 ~ 2019 UnionTech Software Technology Co.,Ltd.
*
* Author:     liuyanga <liuyanga@uniontech.com>
*
* Maintainer: liuyanga <liuyanga@uniontech.com>
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef VNOTEBASEDIALOG_H
#define VNOTEBASEDIALOG_H

#include <DAbstractDialog>
#include <DLabel>
#include <DWindowCloseButton>

DWIDGET_USE_NAMESPACE

class QVBoxLayout;

class VNoteBaseDialog : public DAbstractDialog
{
    Q_OBJECT
public:
    explicit VNoteBaseDialog(QWidget *parent = nullptr);

    void addContent(QWidget* content);
    void setIconPixmap(const QPixmap &iconPixmap);

    const int DEFAULT_WINDOW_W = 380;
    const int DEFAULT_WINDOW_H = 140;
    const int TITLEBAR_H = 50;

protected:
    void initUI();
    void InitConnections();
    void setLogoVisable(bool visible=true);
    void setTitle(const QString& title);
    QLayout* getContentLayout();

    //Overrides
    void closeEvent(QCloseEvent *event) override;
    void showEvent(QShowEvent *event) override;
signals:
    void closed();
public slots:
private:
    QWidget *m_titleBar {nullptr};
    DLabel  *m_logoIcon {nullptr};
    DLabel  *m_tileText {nullptr};
    DWindowCloseButton* m_closeButton {nullptr};

    QWidget *m_content {nullptr};
    QVBoxLayout *m_contentLayout {nullptr};
};

#endif // VNOTEBASEDIALOG_H
