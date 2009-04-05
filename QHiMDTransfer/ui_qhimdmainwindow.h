/********************************************************************************
** Form generated from reading ui file 'qhimdmainwindow.ui'
**
** Created: Sat Apr 4 18:18:32 2009
**      by: Qt User Interface Compiler version 4.5.0
**
** WARNING! All changes made in this file will be lost when recompiling ui file!
********************************************************************************/

#ifndef UI_QHIMDMAINWINDOW_H
#define UI_QHIMDMAINWINDOW_H

#include <QtCore/QVariant>
#include <QtGui/QAction>
#include <QtGui/QApplication>
#include <QtGui/QButtonGroup>
#include <QtGui/QHBoxLayout>
#include <QtGui/QHeaderView>
#include <QtGui/QListWidget>
#include <QtGui/QMainWindow>
#include <QtGui/QMenu>
#include <QtGui/QMenuBar>
#include <QtGui/QPushButton>
#include <QtGui/QStatusBar>
#include <QtGui/QToolBar>
#include <QtGui/QVBoxLayout>
#include <QtGui/QWidget>

QT_BEGIN_NAMESPACE

class Ui_QHiMDMainWindowClass
{
public:
    QAction *action_Download_to_device;
    QAction *action_Upload_to_PC;
    QAction *actionRe_name_selected;
    QAction *actionD_elete_selected;
    QAction *action_Help;
    QAction *action_About;
    QAction *action_Format_disk;
    QAction *action_Quit_2;
    QWidget *centralWidget;
    QVBoxLayout *verticalLayout;
    QHBoxLayout *horizontalLayout;
    QPushButton *pushButton_3;
    QPushButton *pushButton_2;
    QPushButton *pushButton_4;
    QPushButton *pushButton_5;
    QPushButton *pushButton_6;
    QPushButton *pushButton_7;
    QPushButton *pushButton_8;
    QHBoxLayout *horizontalLayout_2;
    QListWidget *listWidget;
    QListWidget *TrackList;
    QMenuBar *menuBar;
    QMenu *menu_Action;
    QMenu *menu;
    QStatusBar *statusBar;
    QToolBar *mainToolBar;

    void setupUi(QMainWindow *QHiMDMainWindowClass)
    {
        if (QHiMDMainWindowClass->objectName().isEmpty())
            QHiMDMainWindowClass->setObjectName(QString::fromUtf8("QHiMDMainWindowClass"));
        QHiMDMainWindowClass->resize(652, 494);
        QSizePolicy sizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
        sizePolicy.setHorizontalStretch(0);
        sizePolicy.setVerticalStretch(0);
        sizePolicy.setHeightForWidth(QHiMDMainWindowClass->sizePolicy().hasHeightForWidth());
        QHiMDMainWindowClass->setSizePolicy(sizePolicy);
        action_Download_to_device = new QAction(QHiMDMainWindowClass);
        action_Download_to_device->setObjectName(QString::fromUtf8("action_Download_to_device"));
        action_Upload_to_PC = new QAction(QHiMDMainWindowClass);
        action_Upload_to_PC->setObjectName(QString::fromUtf8("action_Upload_to_PC"));
        actionRe_name_selected = new QAction(QHiMDMainWindowClass);
        actionRe_name_selected->setObjectName(QString::fromUtf8("actionRe_name_selected"));
        actionD_elete_selected = new QAction(QHiMDMainWindowClass);
        actionD_elete_selected->setObjectName(QString::fromUtf8("actionD_elete_selected"));
        action_Help = new QAction(QHiMDMainWindowClass);
        action_Help->setObjectName(QString::fromUtf8("action_Help"));
        action_About = new QAction(QHiMDMainWindowClass);
        action_About->setObjectName(QString::fromUtf8("action_About"));
        action_Format_disk = new QAction(QHiMDMainWindowClass);
        action_Format_disk->setObjectName(QString::fromUtf8("action_Format_disk"));
        action_Quit_2 = new QAction(QHiMDMainWindowClass);
        action_Quit_2->setObjectName(QString::fromUtf8("action_Quit_2"));
        centralWidget = new QWidget(QHiMDMainWindowClass);
        centralWidget->setObjectName(QString::fromUtf8("centralWidget"));
        verticalLayout = new QVBoxLayout(centralWidget);
        verticalLayout->setSpacing(6);
        verticalLayout->setMargin(11);
        verticalLayout->setObjectName(QString::fromUtf8("verticalLayout"));
        horizontalLayout = new QHBoxLayout();
        horizontalLayout->setSpacing(6);
        horizontalLayout->setObjectName(QString::fromUtf8("horizontalLayout"));
        pushButton_3 = new QPushButton(centralWidget);
        pushButton_3->setObjectName(QString::fromUtf8("pushButton_3"));
        QIcon icon;
        icon.addPixmap(QPixmap(QString::fromUtf8("icons/download_to_md.png")), QIcon::Normal, QIcon::Off);
        pushButton_3->setIcon(icon);

        horizontalLayout->addWidget(pushButton_3);

        pushButton_2 = new QPushButton(centralWidget);
        pushButton_2->setObjectName(QString::fromUtf8("pushButton_2"));
        QIcon icon1;
        icon1.addPixmap(QPixmap(QString::fromUtf8("icons/upload_from_md.png")), QIcon::Normal, QIcon::Off);
        pushButton_2->setIcon(icon1);

        horizontalLayout->addWidget(pushButton_2);

        pushButton_4 = new QPushButton(centralWidget);
        pushButton_4->setObjectName(QString::fromUtf8("pushButton_4"));
        QIcon icon2;
        icon2.addPixmap(QPixmap(QString::fromUtf8("icons/delete.png")), QIcon::Normal, QIcon::Off);
        pushButton_4->setIcon(icon2);

        horizontalLayout->addWidget(pushButton_4);

        pushButton_5 = new QPushButton(centralWidget);
        pushButton_5->setObjectName(QString::fromUtf8("pushButton_5"));
        QIcon icon3;
        icon3.addPixmap(QPixmap(QString::fromUtf8("icons/rename.png")), QIcon::Normal, QIcon::Off);
        pushButton_5->setIcon(icon3);

        horizontalLayout->addWidget(pushButton_5);

        pushButton_6 = new QPushButton(centralWidget);
        pushButton_6->setObjectName(QString::fromUtf8("pushButton_6"));
        QIcon icon4;
        icon4.addPixmap(QPixmap(QString::fromUtf8("icons/format.png")), QIcon::Normal, QIcon::Off);
        pushButton_6->setIcon(icon4);

        horizontalLayout->addWidget(pushButton_6);

        pushButton_7 = new QPushButton(centralWidget);
        pushButton_7->setObjectName(QString::fromUtf8("pushButton_7"));
        QIcon icon5;
        icon5.addPixmap(QPixmap(QString::fromUtf8("icons/add_group.png")), QIcon::Normal, QIcon::Off);
        pushButton_7->setIcon(icon5);

        horizontalLayout->addWidget(pushButton_7);

        pushButton_8 = new QPushButton(centralWidget);
        pushButton_8->setObjectName(QString::fromUtf8("pushButton_8"));
        QIcon icon6;
        icon6.addPixmap(QPixmap(QString::fromUtf8("icons/quit.png")), QIcon::Normal, QIcon::Off);
        pushButton_8->setIcon(icon6);

        horizontalLayout->addWidget(pushButton_8);


        verticalLayout->addLayout(horizontalLayout);

        horizontalLayout_2 = new QHBoxLayout();
        horizontalLayout_2->setSpacing(6);
        horizontalLayout_2->setObjectName(QString::fromUtf8("horizontalLayout_2"));
        listWidget = new QListWidget(centralWidget);
        listWidget->setObjectName(QString::fromUtf8("listWidget"));

        horizontalLayout_2->addWidget(listWidget);

        TrackList = new QListWidget(centralWidget);
        TrackList->setObjectName(QString::fromUtf8("TrackList"));

        horizontalLayout_2->addWidget(TrackList);


        verticalLayout->addLayout(horizontalLayout_2);

        QHiMDMainWindowClass->setCentralWidget(centralWidget);
        menuBar = new QMenuBar(QHiMDMainWindowClass);
        menuBar->setObjectName(QString::fromUtf8("menuBar"));
        menuBar->setGeometry(QRect(0, 0, 652, 25));
        menu_Action = new QMenu(menuBar);
        menu_Action->setObjectName(QString::fromUtf8("menu_Action"));
        menu = new QMenu(menuBar);
        menu->setObjectName(QString::fromUtf8("menu"));
        QHiMDMainWindowClass->setMenuBar(menuBar);
        statusBar = new QStatusBar(QHiMDMainWindowClass);
        statusBar->setObjectName(QString::fromUtf8("statusBar"));
        QHiMDMainWindowClass->setStatusBar(statusBar);
        mainToolBar = new QToolBar(QHiMDMainWindowClass);
        mainToolBar->setObjectName(QString::fromUtf8("mainToolBar"));
        QHiMDMainWindowClass->addToolBar(Qt::TopToolBarArea, mainToolBar);

        menuBar->addAction(menu_Action->menuAction());
        menuBar->addAction(menu->menuAction());
        menu_Action->addAction(action_Download_to_device);
        menu_Action->addAction(action_Upload_to_PC);
        menu_Action->addSeparator();
        menu_Action->addAction(actionD_elete_selected);
        menu_Action->addAction(actionRe_name_selected);
        menu_Action->addSeparator();
        menu_Action->addAction(action_Format_disk);
        menu_Action->addSeparator();
        menu_Action->addAction(action_Quit_2);
        menu->addAction(action_Help);
        menu->addAction(action_About);

        retranslateUi(QHiMDMainWindowClass);

        QMetaObject::connectSlotsByName(QHiMDMainWindowClass);
    } // setupUi

    void retranslateUi(QMainWindow *QHiMDMainWindowClass)
    {
        QHiMDMainWindowClass->setWindowTitle(QApplication::translate("QHiMDMainWindowClass", "QHiMDTransfer", 0, QApplication::UnicodeUTF8));
        action_Download_to_device->setText(QApplication::translate("QHiMDMainWindowClass", "&Download to device", 0, QApplication::UnicodeUTF8));
        action_Upload_to_PC->setText(QApplication::translate("QHiMDMainWindowClass", "&Upload to PC", 0, QApplication::UnicodeUTF8));
        actionRe_name_selected->setText(QApplication::translate("QHiMDMainWindowClass", "Re&name selected", 0, QApplication::UnicodeUTF8));
        actionD_elete_selected->setText(QApplication::translate("QHiMDMainWindowClass", "D&elete selected", 0, QApplication::UnicodeUTF8));
        action_Help->setText(QApplication::translate("QHiMDMainWindowClass", "&Help", 0, QApplication::UnicodeUTF8));
        action_About->setText(QApplication::translate("QHiMDMainWindowClass", "&About", 0, QApplication::UnicodeUTF8));
        action_Format_disk->setText(QApplication::translate("QHiMDMainWindowClass", "&Format disk", 0, QApplication::UnicodeUTF8));
        action_Quit_2->setText(QApplication::translate("QHiMDMainWindowClass", "&Quit", 0, QApplication::UnicodeUTF8));
        pushButton_3->setText(QApplication::translate("QHiMDMainWindowClass", "&Download", 0, QApplication::UnicodeUTF8));
        pushButton_2->setText(QApplication::translate("QHiMDMainWindowClass", "&Upload", 0, QApplication::UnicodeUTF8));
        pushButton_4->setText(QApplication::translate("QHiMDMainWindowClass", "D&elete", 0, QApplication::UnicodeUTF8));
        pushButton_5->setText(QApplication::translate("QHiMDMainWindowClass", "&Rename", 0, QApplication::UnicodeUTF8));
        pushButton_6->setText(QApplication::translate("QHiMDMainWindowClass", "&Format", 0, QApplication::UnicodeUTF8));
        pushButton_7->setText(QApplication::translate("QHiMDMainWindowClass", "Add &Group", 0, QApplication::UnicodeUTF8));
        pushButton_8->setText(QApplication::translate("QHiMDMainWindowClass", "&Quit", 0, QApplication::UnicodeUTF8));
        menu_Action->setTitle(QApplication::translate("QHiMDMainWindowClass", "&File", 0, QApplication::UnicodeUTF8));
        menu->setTitle(QApplication::translate("QHiMDMainWindowClass", "&?", 0, QApplication::UnicodeUTF8));
    } // retranslateUi

};

namespace Ui {
    class QHiMDMainWindowClass: public Ui_QHiMDMainWindowClass {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_QHIMDMAINWINDOW_H
