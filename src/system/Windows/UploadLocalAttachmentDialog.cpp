#include <QFileDialog>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QMessageBox>
#include <QProgressBar>
#include <QPushButton>
#include <QTableWidget>
#include <QVBoxLayout>

#include "FileStorageManager.h"
#include "Logger.h"
#include "MimeTypeHelper.h"
#include "UploadLocalAttachmentDialog.h"

#include "GlobalSignals.h"
#include "SettingsManager.h"
#include "UserManagement.h"
#include "Util.h"
#include "pch.h"

UploadLocalAttachmentDialog::UploadLocalAttachmentDialog(quint64 ticketId, QWidget* parent) : QDialog(parent), _ticketId(ticketId)
{
    setWindowTitle(tr("Upload attachments"));
    setModal(true);
    resize(700, 400);

    setupUi();

    connect(this, &UploadLocalAttachmentDialog::uploadProgressChanged, this,
            &UploadLocalAttachmentDialog::handleUploadProgress, Qt::QueuedConnection);
    connect(this, &UploadLocalAttachmentDialog::uploadRowStatusChanged, this,
            &UploadLocalAttachmentDialog::handleUploadRowStatus, Qt::QueuedConnection);
    connect(this, &UploadLocalAttachmentDialog::uploadFinished, this,
            &UploadLocalAttachmentDialog::handleUploadFinished, Qt::QueuedConnection);
}

void UploadLocalAttachmentDialog::setupUi()
{
    auto* mainLayout = new QVBoxLayout(this);

    _table = new QTableWidget(this);
    _table->setColumnCount(5);

    QStringList headers;
    headers << tr("File name") << tr("Size") << tr("Description") << tr("Status") << tr("Full path");

    _table->setHorizontalHeaderLabels(headers);
    _table->horizontalHeader()->setStretchLastSection(false);
    _table->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    _table->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    _table->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Stretch);
    _table->horizontalHeader()->setSectionResizeMode(3, QHeaderView::ResizeToContents);
    _table->horizontalHeader()->setSectionResizeMode(4, QHeaderView::ResizeToContents);

    _table->setSelectionBehavior(QAbstractItemView::SelectRows);
    _table->setSelectionMode(QAbstractItemView::SingleSelection);
    _table->setEditTriggers(QAbstractItemView::DoubleClicked | QAbstractItemView::SelectedClicked);
    _table->setColumnHidden(4, true);  // hide full path

    mainLayout->addWidget(_table);

    _progressBar = new QProgressBar(this);
    _progressBar->setRange(0, 100);
    _progressBar->setValue(0);
    _progressBar->setTextVisible(true);
    mainLayout->addWidget(_progressBar);

    _statusLabel = new QLabel(tr("Select files to upload."), this);
    mainLayout->addWidget(_statusLabel);

    auto* btnLayout = new QHBoxLayout();
    btnLayout->addStretch();

    _btnAdd = new QPushButton(tr("Add files"), this);
    _btnRemove = new QPushButton(tr("Remove selected"), this);
    _btnUpload = new QPushButton(tr("Upload"), this);
    _btnClose = new QPushButton(tr("Close"), this);
    _btnCancel = new QPushButton(tr("Cancel"), this);

    btnLayout->addWidget(_btnAdd);
    btnLayout->addWidget(_btnRemove);
    btnLayout->addWidget(_btnUpload);
    btnLayout->addWidget(_btnClose);
    btnLayout->addWidget(_btnCancel);

    mainLayout->addLayout(btnLayout);

    connect(_btnAdd, &QPushButton::clicked, this, &UploadLocalAttachmentDialog::onAddFiles);
    connect(_btnRemove, &QPushButton::clicked, this, &UploadLocalAttachmentDialog::onRemoveSelected);
    connect(_btnUpload, &QPushButton::clicked, this, &UploadLocalAttachmentDialog::onStartUpload);
    connect(_btnClose, &QPushButton::clicked, this, &UploadLocalAttachmentDialog::reject);
    connect(_btnCancel, &QPushButton::clicked, this, &UploadLocalAttachmentDialog::onCancelUpload);
}

void UploadLocalAttachmentDialog::addFileToTable(const PendingFile& file)
{
    int row = _table->rowCount();
    _table->insertRow(row);

    auto* itemName = new QTableWidgetItem(file.fileName);
    auto* itemSize = new QTableWidgetItem(QString::number(file.fileSize));
    auto* itemDesc = new QTableWidgetItem(file.description);
    auto* itemStatus = new QTableWidgetItem(file.status);
    auto* itemPath = new QTableWidgetItem(file.localPath);

    _table->setItem(row, 0, itemName);
    _table->setItem(row, 1, itemSize);
    _table->setItem(row, 2, itemDesc);
    _table->setItem(row, 3, itemStatus);
    _table->setItem(row, 4, itemPath);
}

void UploadLocalAttachmentDialog::refreshRow(int row, const PendingFile& file)
{
    if (row < 0 || row >= _table->rowCount())
        return;

    _table->item(row, 0)->setText(file.fileName);
    _table->item(row, 1)->setText(QString::number(file.fileSize));
    _table->item(row, 2)->setText(file.description);
    _table->item(row, 3)->setText(file.status);
    _table->item(row, 4)->setText(file.localPath);
}

void UploadLocalAttachmentDialog::onAddFiles()
{
    const QStringList files =
        QFileDialog::getOpenFileNames(this, tr("Select files to upload"), QString(), tr("All files (*.*)"));

    if (files.isEmpty())
        return;

    QStringList rejected;

    for (const auto& path : files)
    {
        QFileInfo fi(path);
        if (!fi.exists() || !fi.isFile())
            continue;

        if (isForbiddenExtension(path))
        {
            rejected << fi.fileName();
            continue;
        }

        PendingFile pf;
        pf.localPath = path;
        pf.fileName = fi.fileName();
        pf.fileSize = static_cast<quint64>(fi.size());
        pf.description.clear();
        pf.status = tr("Pending");

        _files.push_back(pf);
        addFileToTable(pf);
    }

    if (!rejected.isEmpty())
    {
        QMessageBox::warning(this, tr("Forbidden file types"),
                             tr("The following files were not added because their type is not allowed:\n%1")
                                 .arg(rejected.join(QStringLiteral("\n"))));
    }

    _statusLabel->setText(tr("Files ready for upload."));
}

void UploadLocalAttachmentDialog::onRemoveSelected()
{
    const int row = _table->currentRow();
    if (row < 0 || row >= _table->rowCount())
        return;

    _table->removeRow(row);

    if (row >= 0 && row < _files.size())
        _files.removeAt(row);
}

void UploadLocalAttachmentDialog::updateProgress(int currentIndex, int total)
{
    if (total <= 0)
    {
        _progressBar->setRange(0, 1);
        _progressBar->setValue(0);
        return;
    }

    _progressBar->setRange(0, total);

    if (currentIndex < 0)
        currentIndex = 0;
    else if (currentIndex > total)
        currentIndex = total;

    _progressBar->setValue(currentIndex);

    LOG_DEBUG("UploadLocalAttachmentDialog::updateProgress: currIndex {}, total {}", currentIndex, total);
}

bool UploadLocalAttachmentDialog::isForbiddenExtension(const QString& path) const
{
    static const QSet<QString> forbiddenExtensions
    {
        QStringLiteral("exe"),
        QStringLiteral("bat"),
        QStringLiteral("cmd"),
        QStringLiteral("ps1"),
        QStringLiteral("psm1"),
        QStringLiteral("sh")
    };

    QFileInfo fi(path);
    const QString ext = fi.suffix().toLower();
    return forbiddenExtensions.contains(ext);
}

void UploadLocalAttachmentDialog::CommitTableEdit()
{
    if (!_table)
        return;

    QWidget* editor = QApplication::focusWidget();
    if (!editor)
        return;

    const QModelIndex idx = _table->currentIndex();
    if (!idx.isValid())
        return;

    if (auto* delegate = _table->itemDelegate(idx))
    {
        emit delegate->commitData(editor);
        emit delegate->closeEditor(editor, QAbstractItemDelegate::SubmitModelCache);
    }
}

void UploadLocalAttachmentDialog::onStartUpload()
{
    if (_uploadRunning)
        return;

    if (_files.isEmpty())
    {
        QMessageBox::information(this, tr("No files"), tr("Please add at least one file."));
        return;
    }

    for (const auto& file : std::as_const(_files))
    {
        if (isForbiddenExtension(file.localPath))
        {
            QMessageBox::warning(
                this, tr("Forbidden file types"),
                tr("File \"%1\" has a forbidden file type and cannot be uploaded.").arg(file.fileName));
            return;
        }
    }

    CommitTableEdit();

    // Read descriptions from table
    for (int i = 0; i < _files.size() && i < _table->rowCount(); ++i)
    {
        if (auto* itemDesc = _table->item(i, 2))
            _files[i].description = itemDesc->text();
    }

    _uploadRunning = true;
    _btnAdd->setEnabled(false);
    _btnRemove->setEnabled(false);
    _btnUpload->setEnabled(false);
    _btnClose->setEnabled(false);
    _btnCancel->setEnabled(true);
    _statusLabel->setText(tr("Uploading..."));

    updateProgress(0, _files.size());

    const auto files = _files;
    const int total = files.size();
    const quint64 ticketId = _ticketId;
    const std::uint32_t uploaderId = GetUser().GetUserID();

    // Root path for attachments
    auto localFileData = GetSettings().getLocalFileData();
    const QString rootPath = localFileData.rootPath;

    Util::RunInThread(
        [this, files, total, ticketId, uploaderId, rootPath]()
        {
            FileStorageManager storage(rootPath);

            int currentIndex = 0;
            emit uploadProgressChanged(currentIndex, total);

            for (int row = 0; row < total; ++row)
            {
                if (_cancelRequested.load())
                {
                    emit uploadRowStatusChanged(row, tr("Cancelled"));
                    emit uploadFinished();
                    emit GlobalSignals::instance()->FileUploadFinished();
                    return;
                }

                const auto& file = files[row];
                emit uploadRowStatusChanged(row, tr("Encrypting and saving..."));

                try
                {
                    auto idOpt = storage.CreateTicketAttachment(file.localPath, ticketId, uploaderId, file.description);

                    if (!idOpt)
                    {
                        emit uploadRowStatusChanged(row, tr("Failed"));
                    }
                    else
                    {
                        emit uploadRowStatusChanged(row, tr("Saved"));
                    }
                }
                catch (const std::exception& e)
                {
                    LOG_ERROR("UploadLocalAttachmentDialog::onStartUpload: exception: {}", e.what());
                    emit uploadRowStatusChanged(row, tr("Error"));
                }

                ++currentIndex;
                emit uploadProgressChanged(currentIndex, total);
            }

            emit uploadFinished();
            emit GlobalSignals::instance()->FileUploadFinished();
        });
}

void UploadLocalAttachmentDialog::handleUploadProgress(int current, int total)
{
    _statusLabel->setText(tr("Processing file %1 of %2").arg(current).arg(total));
    updateProgress(current, total);
}

void UploadLocalAttachmentDialog::handleUploadRowStatus(int row, const QString& status)
{
    if (row < 0 || row >= _files.size())
        return;

    _files[row].status = status;
    refreshRow(row, _files[row]);
}

void UploadLocalAttachmentDialog::handleUploadFinished()
{
    _uploadRunning = false;
    _cancelRequested.store(false);

    _btnAdd->setEnabled(true);
    _btnRemove->setEnabled(true);
    _btnUpload->setEnabled(true);
    _btnCancel->setEnabled(true);
    _btnClose->setEnabled(true);

    if (_statusLabel->text() != tr("Cancelling..."))
        _statusLabel->setText(tr("Upload finished."));
}

void UploadLocalAttachmentDialog::onCancelUpload()
{
    if (!_uploadRunning)
    {
        reject();
        return;
    }

    _cancelRequested.store(true);
    _statusLabel->setText(tr("Cancelling..."));

    _btnAdd->setEnabled(false);
    _btnRemove->setEnabled(false);
    _btnUpload->setEnabled(false);
    _btnCancel->setEnabled(false);
}

void UploadLocalAttachmentDialog::closeEvent(QCloseEvent* event)
{
    if (_uploadRunning)
    {
        _cancelRequested.store(true);
        _statusLabel->setText(tr("Cancelling..."));
        event->ignore();
        return;
    }

    event->accept();
}
