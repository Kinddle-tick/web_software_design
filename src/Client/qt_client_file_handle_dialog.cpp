//
// Created by Kinddle Lee on 2022/1/12.
//

// You may need to build the project (run Qt uic code generator) to get "ui_qt_client_file_handle_dialog.h" resolved

#include "qt_client_file_handle_dialog.h"
#include "ui_qt_client_file_handle_dialog.h"
const char * kTreeViewStyleSheet = "QTreeView{background-color: #f3f5ec; color:black; font: bold 14px;outline:none;}"
                                   "QTreeView::branch:open:has-children:has-children{image: url(:/icon/tree_open.png);}"
                                   "QTreeView::branch:closed:has-children:has-children{image: url(:/icon/tree_close.png);} "
                                   "QTreeView::item:selected {background-color:#CCCC99;color:black;}";

namespace ClientFileHandleDialog {
    ClientFileHandleDialog::ClientFileHandleDialog(QWidget *parent) :
            QDialog(parent), ui(new Ui::ClientFileHandleDialog) {
        ui->setupUi(this);
        ui->ServerTree->setStyleSheet(kTreeViewStyleSheet);
        ui->ClientTree->setStyleSheet(kTreeViewStyleSheet);
        ui->ServerTree->setEditTriggers(QAbstractItemView::NoEditTriggers);
        ui->ClientTree->setEditTriggers(QAbstractItemView::NoEditTriggers);
//        ui->ServerTree->setSelectionBehavior(QAbstractItemView::SelectRows);

        Server_model = new QStandardItemModel(ui->ServerTree);
        Client_model = new QStandardItemModel(ui->ClientTree);
        this->setAttribute(Qt::WA_DeleteOnClose); // 保证每次关闭对话框的资源都会被清理 否则每开一次对话框，内存里都会永远有一个对话框等着接受信号

        Server_model->setHorizontalHeaderLabels(QStringList()<<QStringLiteral("目录") << QStringLiteral("路径名"));     //设置列头
        Client_model->setHorizontalHeaderLabels(QStringList()<<QStringLiteral("目录") << QStringLiteral("路径名"));     //设置列头
        ui->ServerTree->setModel(Server_model);
        ui->ClientTree->setModel(Client_model);
    }

    void ClientFileHandleDialog::slot_MenuCacheServer(const QString &str) {
        ui->ServerTree->setModel(setTreeView(Server_model,str));
        ui->ServerTree->expandAll();
    }
    void ClientFileHandleDialog::slot_MenuCacheClient(const QString &str) {
        ui->ClientTree->setModel(setTreeView(Client_model,str));
        ui->ClientTree->expandAll();
    }
    QStandardItemModel *ClientFileHandleDialog::setTreeView(QStandardItemModel *mModel, const QString &str) {
        mModel->clear();
        mModel->setHorizontalHeaderLabels(QStringList()<<QStringLiteral("目录") << QStringLiteral("路径名"));     //设置列头
        QStringList sections  = str.split("\n",QString::KeepEmptyParts);
        QStringList headers;
        QList<QStandardItem*> dir_List;
        QStandardItem* new_index;
        QStandardItem* description;
        int old_count = 0,new_count = 0;
        for(auto &iter:sections){
            if(iter.isEmpty()){
                continue;
            }
            new_count = iter.count("\t");
            if(headers.length()==new_count){
                headers.append("?");
            }
            if(dir_List.length()==new_count){
                dir_List.push_back(nullptr);
            }
            QList<QStandardItem*> items;
            if(iter.count(".")==0){
                new_index = new QStandardItem(QPixmap(":icon/folder.png"),iter.trimmed());
            }
            else{
                new_index = new QStandardItem(QPixmap(":icon/file.png"),iter.trimmed());
            }
            while(new_count<old_count){
                headers.removeLast();
                old_count--;
            }
            if(new_count-old_count>1){
                printf("无法理解的结构,此行不处理");
                continue;
            }
            headers.replace(new_count,iter.trimmed());

            description = new QStandardItem(headers.join("/"));
            items.append(new_index);
            items.append(description);
            if(new_count == 0){
                dir_List.replace(0,new_index);
                mModel->appendRow(items);
            }
            else{
                dir_List.replace(new_count,new_index);
                dir_List[new_count-1]->appendRow(items);
            }
            old_count = new_count;
        }
        return mModel;
    }


    ClientFileHandleDialog::~ClientFileHandleDialog() {
        delete ui;
        delete Server_model;
        delete Client_model;
    }

    void ClientFileHandleDialog::ClientSignalSlotConnect(QtClientDialog::QtClientDialog *client_dialog) {
        connect(this,&ClientFileHandleDialog::sig_Mission_File,client_dialog,&QtClientDialog::QtClientDialog::slot_Mission_File,Qt::QueuedConnection);
        connect(ui->ButtonUpload,&QPushButton::clicked,this,&ClientFileHandleDialog::slot_ButtonUpload_clicked);
        connect(ui->ButtonDownload,&QPushButton::clicked,this,&ClientFileHandleDialog::slot_ButtonDownload_clicked);
        connect(ui->ButtonCancle,&QPushButton::clicked,this,&ClientFileHandleDialog::slot_ButtonCancel_clicked);
        connect(client_dialog,&QtClientDialog::QtClientDialog::sig_MenuCacheServer,this,&ClientFileHandleDialog::slot_MenuCacheServer);
        connect(client_dialog,&QtClientDialog::QtClientDialog::sig_MenuCacheClient,this,&ClientFileHandleDialog::slot_MenuCacheClient);
        connect(ui->ClientTree->selectionModel(),&QItemSelectionModel::currentChanged,this,
                &ClientFileHandleDialog::slot_ClientCurrentChanged);
        connect(ui->ServerTree->selectionModel(),&QItemSelectionModel::currentChanged,this,
                &ClientFileHandleDialog::slot_ServerCurrentChanged);
    }

    void ClientFileHandleDialog::slot_ButtonUpload_clicked() {
        QString server_path = ui->ServerPathEdit->text();
        QString client_path = ui->ClientPathEdit->text();
        sig_Mission_File(server_path,client_path,true,false);
        this->accept();
    }

    void ClientFileHandleDialog::slot_ButtonDownload_clicked() {
        QString server_path = ui->ServerPathEdit->text();
        QString client_path = ui->ClientPathEdit->text();
        sig_Mission_File(server_path,client_path,false,true);
        this->accept();
    }

    void ClientFileHandleDialog::slot_ButtonCancel_clicked() {
        this->accept();
    }

    void ClientFileHandleDialog::slot_ServerCurrentChanged(const QModelIndex &selected,
                                                           const QModelIndex &deselected) {
        QStandardItem * item = Server_model->itemFromIndex(selected.sibling(selected.row(),1));
        if(item){
            QString text = item->text();
            ui->ServerPathEdit->setText(text);
        }
    }

    void ClientFileHandleDialog::slot_ClientCurrentChanged(const QModelIndex &selected,
                                                           const QModelIndex &deselected) {
        QStandardItem * item = Client_model->itemFromIndex(selected.sibling(selected.row(),1));
        if(item){
            QString text = item->text();
            ui->ClientPathEdit->setText(text);
        }
    }




} // ClientFileHandleDialog
