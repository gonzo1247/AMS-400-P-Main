#include "pch.h"
#include "UploadAttachmentDialog.h"

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

#include "ConnectionGuard.h"
#include "FileStorageService.h"
#include "GlobalSignals.h"
#include "MimeTypeHelper.h"
#include "UserManagement.h"

UploadAttachmentDialog::UploadAttachmentDialog(quint64 ticketId, QWidget* parent) : QDialog(parent), _ticketId(ticketId)
{
    setWindowTitle(tr("Upload Attachments"));
    setModal(true);
    resize(700, 400);

    setupUi();

    connect(this, &UploadAttachmentDialog::uploadProgressChanged, this, &UploadAttachmentDialog::handleUploadProgress, Qt::QueuedConnection);
    connect(this, &UploadAttachmentDialog::uploadRowStatusChanged, this, &UploadAttachmentDialog::handleUploadRowStatus, Qt::QueuedConnection);
    connect(this, &UploadAttachmentDialog::uploadFinished, this, &UploadAttachmentDialog::handleUploadFinished, Qt::QueuedConnection);
    connect(this, &UploadAttachmentDialog::singleFileProgressChanged, this, &UploadAttachmentDialog::handleSingleFileProgress, Qt::QueuedConnection);
}

void UploadAttachmentDialog::setupUi()
{
    auto* mainLayout = new QVBoxLayout(this);

    _table = new QTableWidget(this);
    _table->setColumnCount(5);
    QStringList headers;
    headers << tr("File Name") << tr("Size") << tr("Description") << tr("Status") << tr("Full Path") << tr("Internal ID");
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
    _table->setColumnHidden(4, true);  // hide full path column
    _table->setColumnHidden(5, true);  // hide internal ID column

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

    _btnAdd = new QPushButton(tr("Add Files"), this);
    _btnRemove = new QPushButton(tr("Remove Selected"), this);
    _btnUpload = new QPushButton(tr("Upload"), this);
    _btnClose = new QPushButton(tr("Close"), this);
    _btnCancel = new QPushButton(tr("Cancel"), this);

    btnLayout->addWidget(_btnAdd);
    btnLayout->addWidget(_btnRemove);
    btnLayout->addWidget(_btnUpload);
    btnLayout->addWidget(_btnClose);
    btnLayout->addWidget(_btnCancel);

    mainLayout->addLayout(btnLayout);

    connect(_btnAdd, &QPushButton::clicked, this, &UploadAttachmentDialog::onAddFiles);
    connect(_btnRemove, &QPushButton::clicked, this, &UploadAttachmentDialog::onRemoveSelected);
    connect(_btnUpload, &QPushButton::clicked, this, &UploadAttachmentDialog::onStartUpload);
    connect(_btnClose, &QPushButton::clicked, this, &UploadAttachmentDialog::reject);
    connect(_btnCancel, &QPushButton::clicked, this, &UploadAttachmentDialog::onCancelUpload);
    connect(_table, &QTableWidget::itemChanged, this, &UploadAttachmentDialog::onChangeDescription);
}

void UploadAttachmentDialog::addFileToTable(PendingFile& file)
{
    int row = _table->rowCount();
    _table->insertRow(row);

    auto* itemName = new QTableWidgetItem(file.fileName);
    auto* itemSize = new QTableWidgetItem(QString::number(file.fileSize));
    auto* itemDesc = new QTableWidgetItem(file.description);
    auto* itemStatus = new QTableWidgetItem(file.status);
    auto* itemPath = new QTableWidgetItem(file.localPath);
    auto* itemID = new QTableWidgetItem(QString::number(row));

    _table->setItem(row, 0, itemName);
    _table->setItem(row, 1, itemSize);
    _table->setItem(row, 2, itemDesc);
    _table->setItem(row, 3, itemStatus);
    _table->setItem(row, 4, itemPath);
    _table->setItem(row, 5, itemID);
    file.internalID = static_cast<quint8>(row);
}

void UploadAttachmentDialog::refreshRow(int row, const PendingFile& file)
{
    if (row < 0 || row >= _table->rowCount())
        return;

    _table->item(row, 0)->setText(file.fileName);
    _table->item(row, 1)->setText(QString::number(file.fileSize));
    _table->item(row, 2)->setText(file.description);
    _table->item(row, 3)->setText(file.status);
    _table->item(row, 4)->setText(file.localPath);
}

void UploadAttachmentDialog::onAddFiles()
{
    const QStringList files =
        QFileDialog::getOpenFileNames(this, tr("Select files to upload"), QString(), tr("All Files (*.*)"));
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

void UploadAttachmentDialog::onRemoveSelected()
{
    const int row = _table->currentRow();
    if (row < 0 || row >= _table->rowCount())
        return;

    _table->removeRow(row);

    if (row >= 0 && row < _files.size())
    {
        _files.removeAt(row);
    }
}

void UploadAttachmentDialog::updateProgress(int currentIndex, int total)
{
    if (total <= 0)
    {
        _progressBar->setRange(0, 1);
        _progressBar->setValue(0);
        return;
    }

    // Progress bar shows 0..total, Qt will draw it as percentage
    _progressBar->setRange(0, total);

    // clamp for safety
    if (currentIndex < 0)
        currentIndex = 0;
    else if (currentIndex > total)
        currentIndex = total;

    _progressBar->setValue(currentIndex);

    LOG_DEBUG("UpdateProgress: currIndex: {}, total: {}", currentIndex, total);
}

bool UploadAttachmentDialog::insertAttachmentToDatabase(const FileStorageService::StoredFileInfo& info, const PendingFile& file)
{
    LOG_DEBUG("Calling insertAttachmentToDatabase for file: {}", file.fileName.toStdString());
    try
    {
        ConnectionGuardAMS connection(ConnectionType::Sync);
        auto stmt = connection->GetPreparedStatement(AMSPreparedStatement::DB_TATT_INSERT_NEW_TICKET_ATTACHMENT);

        const QString mime = MimeTypeHelper::detectMimeType(file.localPath);
        LOG_DEBUG("Inserting attachment to DB: {}, mime: {} | detect mime with localPath {}", file.fileName.toStdString(), mime.toStdString(), file.localPath.toStdString());

        stmt->SetUInt64(0, _ticketId);
        stmt->SetUInt(1, GetUser().GetUserID());
        stmt->SetCurrentDate(2);               // uploaded_at
        stmt->SetString(3, file.fileName.toStdString());
        stmt->SetString(4, info.storedFileName.toStdString());
        stmt->SetString(5, info.relativePath.toStdString());
        stmt->SetString(6, mime.toStdString());
        stmt->SetUInt64(7, info.fileSize);
        stmt->SetString(8, file.description.toStdString());
        stmt->SetBool(9, false);

        connection->ExecutePreparedInsert(*stmt);
    }
    catch (const std::exception& e)
    {
        // You can replace with your logging system
        LOG_ERROR("insertAttachmentToDatabase failed: {}", e.what());
        return false;
    }

    return true;
}

bool UploadAttachmentDialog::isForbiddenExtension(const QString& path) const
{
    static const QSet<QString> forbiddenExtensions
    {
        QStringLiteral("exe"),
        QStringLiteral("bat"),
        QStringLiteral("cmd"),
        QStringLiteral("ps1"),
        QStringLiteral("psm1"),
        QStringLiteral("sh")
        // extend here if needed
    };

    QFileInfo fi(path);
    const QString ext = fi.suffix().toLower();
    return forbiddenExtensions.contains(ext);
}

void UploadAttachmentDialog::onStartUpload()
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

    // Read descriptions from the table (if user edited them)
    for (int i = 0; i < _files.size() && i < _table->rowCount(); ++i)
    {
        if (auto* itemDesc = _table->item(i, 2))
            _files[i].description = itemDesc->text();
    }

    // Quick check in UI thread
    {
        FileStorageService storage;
        if (!storage.ensureConnected())
        {
            QMessageBox::warning(this, tr("Network share"),
                                 tr("Could not connect to the network share. Please check settings."));
            return;
        }
    }

    _uploadRunning = true;
    _btnAdd->setEnabled(false);
    _btnRemove->setEnabled(false);
    _btnUpload->setEnabled(false);
    _btnClose->setEnabled(false);
    _statusLabel->setText(tr("Uploading..."));
    updateProgress(0, _files.size());

    // Work data copy for worker thread (no UI access from there)
    const auto files = _files;
    const int total = files.size();
    const quint64 ticketId = _ticketId;

    Util::RunInThread(
        [this, files, total, ticketId]()
        {
            FileStorageService storage;
            storage.ensureConnected();  // should be cheap if already connected

            int currentIndex = 0;

            // initial
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

                emit uploadRowStatusChanged(row, tr("Uploading..."));

                auto progressCallback = [this](qint64 done, qint64 totalBytes)
                {
                    emit singleFileProgressChanged(done, totalBytes);
                };

                auto cancelCallback = [this]() -> bool { return _cancelRequested.load(); };

                auto storedInfo = storage.saveAttachment(ticketId, file.localPath, progressCallback, cancelCallback);
                if (!storedInfo.has_value())
                {
                    emit uploadRowStatusChanged(row, tr("Upload failed"));
                    ++currentIndex;
                    emit uploadProgressChanged(currentIndex, total);
                    continue;
                }

                LOG_DEBUG("File uploaded, stored path: {}", storedInfo->relativePath.toStdString());
                // DB insert in worker thread (no UI access here)
                if (!insertAttachmentToDatabase(*storedInfo, file))
                {
                    emit uploadRowStatusChanged(row, tr("DB error"));
                    ++currentIndex;
                    emit uploadProgressChanged(currentIndex, total);
                    continue;
                }

                emit uploadRowStatusChanged(row, tr("Uploaded"));

                ++currentIndex;
                emit uploadProgressChanged(currentIndex, total);
            }

            emit uploadFinished();
            emit GlobalSignals::instance()->FileUploadFinished();
        });
}

void UploadAttachmentDialog::handleUploadProgress(int current, int total)
{
    _statusLabel->setText(tr("Uploading file %1 of %2").arg(current + 1).arg(total));
}

void UploadAttachmentDialog::handleUploadRowStatus(int row, const QString& status)
{
    if (row < 0 || row >= _files.size())
        return;

    _files[row].status = status;
    refreshRow(row, _files[row]);
}

void UploadAttachmentDialog::handleUploadFinished()
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

void UploadAttachmentDialog::handleSingleFileProgress(qint64 done, qint64 total)
{
    if (total <= 0)
    {
        _progressBar->setRange(0, 1);
        _progressBar->setValue(0);
        return;
    }

    _progressBar->setRange(0, static_cast<int>(total));

    if (done < 0)
        done = 0;
    if (done > total)
        done = total;

    _progressBar->setValue(static_cast<int>(done));
}

void UploadAttachmentDialog::onCancelUpload()
{
    if (!_uploadRunning)
    {
        reject();
        return;
    }

    _cancelRequested.store(true);

    _statusLabel->setText(tr("Cancelling..."));

    // UI disable
    _btnAdd->setEnabled(false);
    _btnRemove->setEnabled(false);
    _btnUpload->setEnabled(false);
    _btnCancel->setEnabled(false);
}

void UploadAttachmentDialog::onChangeDescription(QTableWidgetItem* item)
{
    if (!item)
        return;

    const int row = item->row();
    const int col = item->column();

    if (col != 2) // Description column
        return;

    QString newText = item->text();
    auto* internalID = _table->item(row, 5);
    if (internalID)
    {
        const quint8 id = static_cast<quint8>(internalID->text().toUInt());
        for (auto& file : _files)
        {
            if (file.internalID == id)
            {
                file.description = newText;
                break;
            }
        }
    }
}

void UploadAttachmentDialog::closeEvent(QCloseEvent* event)
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
