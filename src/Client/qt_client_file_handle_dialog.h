//
// Created by Kinddle Lee on 2022/1/12.
//

#ifndef WEB_SOFTWARE_DESIGN_FIVE_QT_CLIENT_FILE_HANDLE_DIALOG_H
#define WEB_SOFTWARE_DESIGN_FIVE_QT_CLIENT_FILE_HANDLE_DIALOG_H

#include <QDialog>
#include <QItemSelection>
#include <QStandardItem>
#include "qt_client_dialog.h"
//using QtClientDialog::QtClientDialog;
namespace ClientFileHandleDialog {
    QT_BEGIN_NAMESPACE
    namespace Ui { class ClientFileHandleDialog; }
    QT_END_NAMESPACE

    class ClientFileHandleDialog : public QDialog {
    Q_OBJECT

    public:
        explicit ClientFileHandleDialog(QWidget *parent = nullptr);

        ~ClientFileHandleDialog() override;
        void ClientSignalSlotConnect(QtClientDialog::QtClientDialog *client_dialog);
        static QStandardItemModel* setTreeView(QStandardItemModel* mModel,const QString & str);
    signals:
        void sig_Mission_File(const QString& server_path, const QString& client_path,bool upload,bool download);
    private:
        Ui::ClientFileHandleDialog *ui;
        QStandardItemModel* Server_model;
        QStandardItemModel* Client_model;
    public slots:
        //界面按钮相关
        void slot_ButtonUpload_clicked();
        void slot_ButtonDownload_clicked();
        void slot_ButtonCancel_clicked();
        //树
        void slot_ServerCurrentChanged(const QModelIndex &selected, const QModelIndex &deselected);
        void slot_ClientCurrentChanged(const QModelIndex &selected, const QModelIndex &deselected);

        void slot_MenuCacheServer(const QString&);
        void slot_MenuCacheClient(const QString&);

    };
} // ClientFileHandleDialog

#endif //WEB_SOFTWARE_DESIGN_FIVE_QT_CLIENT_FILE_HANDLE_DIALOG_H
