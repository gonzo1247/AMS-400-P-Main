#pragma once

#include <QDialog>
#include <QString>
#include <QVector>

#include <atomic>

#include "FileStorageService.h"

class QTableWidget;
class QProgressBar;
class QPushButton;
class QLabel;

class UploadAttachmentDialog : public QDialog
{
    Q_OBJECT

public:
    explicit UploadAttachmentDialog(quint64 ticketId, QWidget* parent = nullptr);
    ~UploadAttachmentDialog() override = default;

private:
    struct PendingFile
    {
        QString localPath;
        QString fileName;
        quint64 fileSize{0};
        QString description;
        QString status;
        quint8  internalID{0};
    };

    void setupUi();
    void addFileToTable(PendingFile& file);
    void refreshRow(int row, const PendingFile& file);
    void updateProgress(int currentIndex, int total);

    bool insertAttachmentToDatabase(const FileStorageService::StoredFileInfo& info, const PendingFile& file);

    bool isForbiddenExtension(const QString& path) const;

private:
    quint64 _ticketId{0};
    QTableWidget* _table{nullptr};
    QProgressBar* _progressBar{nullptr};
    QLabel* _statusLabel{nullptr};
    QPushButton* _btnAdd{nullptr};
    QPushButton* _btnRemove{nullptr};
    QPushButton* _btnUpload{nullptr};
    QPushButton* _btnClose{nullptr};
    QPushButton* _btnCancel{nullptr};

    QVector<PendingFile> _files;

    std::atomic_bool _cancelRequested{false};
    bool _uploadRunning{false};

signals:
    void uploadProgressChanged(int current, int total);
    void uploadRowStatusChanged(int row, const QString& status);
    void uploadFinished();

    void singleFileProgressChanged(qint64 done, qint64 total);

private slots:
    void onAddFiles();
    void onRemoveSelected();
    void onStartUpload();
    void handleUploadProgress(int current, int total);
    void handleUploadRowStatus(int row, const QString& status);
    void handleUploadFinished();
    void handleSingleFileProgress(qint64 done, qint64 total);
    void onCancelUpload();
    void onChangeDescription(QTableWidgetItem* item);

protected:
    void closeEvent(QCloseEvent* event) override;
};
