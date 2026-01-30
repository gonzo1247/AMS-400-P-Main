#pragma once

#include <QAction>
#include <QFont>
#include <QIcon>
#include <QKeySequence>
#include <QTextBlockFormat>
#include <QTextCharFormat>
#include <QTextCursor>
#include <QTextDocument>
#include <QTextEdit>
#include <QTextList>
#include <QTextListFormat>
#include <QToolBar>
#include <QToolButton>
#include <QVBoxLayout>
#include <QWidget>
#include <algorithm>

class RichTextEditWithToolbar : public QWidget
{
    Q_OBJECT

   public:
    explicit RichTextEditWithToolbar(QWidget* parent = nullptr) : QWidget(parent)
    {
        _edit = new QTextEdit(this);
        _toolbar = new QToolBar(this);
        _toolbar->setIconSize(QSize(16, 16));

        _actBold = new QAction(tr("Bold"), this);
        _actBold->setCheckable(true);
        _actBold->setShortcut(QKeySequence::Bold);
        _actBold->setToolTip(tr("Bold"));

        _actItalic = new QAction(tr("Italic"), this);
        _actItalic->setCheckable(true);
        _actItalic->setShortcut(QKeySequence::Italic);
        _actItalic->setToolTip(tr("Italic"));

        _actUnderline = new QAction(tr("Underline"), this);
        _actUnderline->setCheckable(true);
        _actUnderline->setShortcut(QKeySequence::Underline);
        _actUnderline->setToolTip(tr("Underline"));

        _actUndo = new QAction(tr("Undo"), this);
        _actUndo->setShortcut(QKeySequence::Undo);
        _actUndo->setToolTip(tr("Undo"));

        _actRedo = new QAction(tr("Redo"), this);
        _actRedo->setShortcut(QKeySequence::Redo);
        _actRedo->setToolTip(tr("Redo"));

        _actBulletList = new QAction(tr("Bullet List"), this);
        _actBulletList->setCheckable(true);
        _actBulletList->setToolTip(tr("Bullet list"));

        _actNumberedList = new QAction(tr("Numbered List"), this);
        _actNumberedList->setCheckable(true);
        _actNumberedList->setToolTip(tr("Numbered list"));

        _actIndent = new QAction(tr("Indent"), this);
        _actIndent->setToolTip(tr("Increase indent"));

        _actOutdent = new QAction(tr("Outdent"), this);
        _actOutdent->setToolTip(tr("Decrease indent"));

        _actReset = new QAction(tr("Reset"), this);
        _actReset->setToolTip(tr("Reset text to default template"));

        applyIcons();

        addToolButton(_actBold);
        addToolButton(_actItalic);
        addToolButton(_actUnderline);

        _toolbar->addSeparator();

        addToolButton(_actUndo);
        addToolButton(_actRedo);

        _toolbar->addSeparator();
        addToolButton(_actReset);

        _toolbar->addSeparator();

        addToolButton(_actBulletList);
        addToolButton(_actNumberedList);

        _toolbar->addSeparator();

        addToolButton(_actIndent);
        addToolButton(_actOutdent);

        connect(_actBold, &QAction::toggled, this, &RichTextEditWithToolbar::onBoldToggled);
        connect(_actItalic, &QAction::toggled, this, &RichTextEditWithToolbar::onItalicToggled);
        connect(_actUnderline, &QAction::toggled, this, &RichTextEditWithToolbar::onUnderlineToggled);

        connect(_actUndo, &QAction::triggered, _edit, &QTextEdit::undo);
        connect(_actRedo, &QAction::triggered, _edit, &QTextEdit::redo);

        connect(_actBulletList, &QAction::triggered, this, &RichTextEditWithToolbar::onBulletListTriggered);
        connect(_actNumberedList, &QAction::triggered, this, &RichTextEditWithToolbar::onNumberedListTriggered);

        connect(_actIndent, &QAction::triggered, this, &RichTextEditWithToolbar::indent);
        connect(_actOutdent, &QAction::triggered, this, &RichTextEditWithToolbar::outdent);

        connect(_edit->document(), &QTextDocument::undoAvailable, _actUndo, &QAction::setEnabled);
        connect(_edit->document(), &QTextDocument::redoAvailable, _actRedo, &QAction::setEnabled);

        connect(_edit, &QTextEdit::cursorPositionChanged, this, &RichTextEditWithToolbar::syncButtonStates);

        connect(_actReset, &QAction::triggered, this, &RichTextEditWithToolbar::resetToDefaultTemplate);


        _actUndo->setEnabled(false);
        _actRedo->setEnabled(false);

        auto* layout = new QVBoxLayout(this);
        layout->setContentsMargins(0, 0, 0, 0);
        layout->setSpacing(4);
        layout->addWidget(_toolbar);
        layout->addWidget(_edit);
        setLayout(layout);

        _edit->setAcceptRichText(true);

_edit->setHtml(R"(
                <p><strong>Problem</strong></p>
                <ul>
                    <li>What was reported?</li>
                </ul>

                <p><strong>Root Cause</strong></p>
                <ul>
                    <li>What caused the issue?</li>
                </ul>

                <p><strong>Solution</strong></p>
                <ul>
                    <li>What was done to fix it?</li>
                </ul>

                <p><strong>Notes</strong></p>
                <ul>
                    <li>Optional</li>
                </ul>)");


        syncButtonStates();
    }

    QTextEdit* editor() const { return _edit; }

    QString toHtml() const { return _edit->toHtml(); }
    QString toPlain() const { return _edit->toPlainText(); }
    void setHtml(const QString& html) { _edit->setHtml(html); }

    bool hasUnsavedChanges() const { return _edit && _edit->document()->isModified(); }

    void setMarkAsSaved() { _edit->document()->setModified(false); }

   private slots:
    void onBoldToggled(bool enabled)
    {
        QTextCharFormat fmt;
        fmt.setFontWeight(enabled ? QFont::Bold : QFont::Normal);
        mergeFormatOnSelection(fmt);
    }

    void onItalicToggled(bool enabled)
    {
        QTextCharFormat fmt;
        fmt.setFontItalic(enabled);
        mergeFormatOnSelection(fmt);
    }

    void onUnderlineToggled(bool enabled)
    {
        QTextCharFormat fmt;
        fmt.setFontUnderline(enabled);
        mergeFormatOnSelection(fmt);
    }

    void onBulletListTriggered() { setListStyle(QTextListFormat::ListDisc); }

    void onNumberedListTriggered() { setListStyle(QTextListFormat::ListDecimal); }

    void indent()
    {
        QTextCursor cursor = _edit->textCursor();
        cursor.beginEditBlock();

        if (QTextList* list = cursor.currentList())
        {
            QTextListFormat lf = list->format();
            lf.setIndent(lf.indent() + 1);
            list->setFormat(lf);
            cursor.endEditBlock();
            return;
        }

        QTextBlockFormat bf = cursor.blockFormat();
        bf.setIndent(bf.indent() + 1);
        cursor.setBlockFormat(bf);

        cursor.endEditBlock();
    }

    void outdent()
    {
        QTextCursor cursor = _edit->textCursor();
        cursor.beginEditBlock();

        if (QTextList* list = cursor.currentList())
        {
            QTextListFormat lf = list->format();
            lf.setIndent(std::max(1, lf.indent() - 1));
            list->setFormat(lf);
            cursor.endEditBlock();
            return;
        }

        QTextBlockFormat bf = cursor.blockFormat();
        bf.setIndent(std::max(0, bf.indent() - 1));
        cursor.setBlockFormat(bf);

        cursor.endEditBlock();
    }

    void syncButtonStates()
    {
        const QTextCharFormat cf = _edit->currentCharFormat();

        _actBold->blockSignals(true);
        _actItalic->blockSignals(true);
        _actUnderline->blockSignals(true);

        _actBold->setChecked(cf.fontWeight() == QFont::Bold);
        _actItalic->setChecked(cf.fontItalic());
        _actUnderline->setChecked(cf.fontUnderline());

        _actBold->blockSignals(false);
        _actItalic->blockSignals(false);
        _actUnderline->blockSignals(false);

        syncListButtonStates();
    }

   private:
    void addToolButton(QAction* action)
    {
        auto* btn = new QToolButton(_toolbar);
        btn->setDefaultAction(action);
        btn->setToolButtonStyle(Qt::ToolButtonIconOnly);
        btn->setAutoRaise(true);
        _toolbar->addWidget(btn);
    }

    void applyIcons()
    {
        using oclero::qlementine::icons::iconPath;

        _actBold->setIcon(QIcon(iconPath(Util::Icons16::Text_FormatBold)));
        _actItalic->setIcon(QIcon(iconPath(Util::Icons16::Text_FormatItalic)));
        _actUnderline->setIcon(QIcon(iconPath(Util::Icons16::Text_FormatUnderline)));

        _actUndo->setIcon(QIcon(iconPath(Util::Icons16::Action_Undo)));
        _actRedo->setIcon(QIcon(iconPath(Util::Icons16::Action_Redo)));

        _actBulletList->setIcon(QIcon(iconPath(Util::Icons16::Text_Paragraph)));
        _actNumberedList->setIcon(QIcon(iconPath(Util::Icons16::Text_Paragraph)));

        _actIndent->setIcon(QIcon(iconPath(Util::Icons16::Text_IndentMore)));
        _actOutdent->setIcon(QIcon(iconPath(Util::Icons16::Text_IndentLess)));

        _actReset->setIcon(QIcon(iconPath(Util::Icons16::Action_Reset)));
    }

    void mergeFormatOnSelection(const QTextCharFormat& fmt)
    {
        QTextCursor cursor = _edit->textCursor();
        if (!cursor.hasSelection())
        {
            cursor.select(QTextCursor::WordUnderCursor);
        }

        cursor.mergeCharFormat(fmt);
        _edit->mergeCurrentCharFormat(fmt);
        _edit->setTextCursor(cursor);
    }

    void setListStyle(QTextListFormat::Style style)
    {
        QTextCursor cursor = _edit->textCursor();
        cursor.beginEditBlock();

        if (QTextList* list = cursor.currentList())
        {
            QTextListFormat lf = list->format();
            if (lf.style() == style)
            {
                QTextBlockFormat bf = cursor.blockFormat();
                bf.setObjectIndex(-1);
                cursor.setBlockFormat(bf);
                cursor.endEditBlock();
                syncListButtonStates();
                return;
            }
        }

        QTextListFormat lf;
        lf.setStyle(style);
        lf.setIndent(1);
        cursor.createList(lf);

        cursor.endEditBlock();
        syncListButtonStates();
    }

    void syncListButtonStates()
    {
        QTextCursor cursor = _edit->textCursor();
        QTextList* list = cursor.currentList();

        _actBulletList->blockSignals(true);
        _actNumberedList->blockSignals(true);

        if (!list)
        {
            _actBulletList->setChecked(false);
            _actNumberedList->setChecked(false);
        }
        else
        {
            const QTextListFormat lf = list->format();
            _actBulletList->setChecked(lf.style() == QTextListFormat::ListDisc);
            _actNumberedList->setChecked(lf.style() == QTextListFormat::ListDecimal);
        }

        _actBulletList->blockSignals(false);
        _actNumberedList->blockSignals(false);
    }

    void setDefaultHtml()
    {
        _edit->blockSignals(true);
        _edit->setHtml(R"(
                            <p><strong>Problem</strong></p>
                            <ul>
                                <li><em>What was reported?</em></li>
                            </ul>

                            <p><strong>Root Cause</strong></p>
                            <ul>
                                <li><em>What caused the issue?</em></li>
                            </ul>

                            <p><strong>Solution</strong></p>
                            <ul>
                                <li><em>What was done to fix it?</em></li>
                            </ul>

                            <p><strong>Notes</strong></p>
                            <ul>
                                <li><em>Optional</em></li>
                            </ul>
                            )"
                            );

        _edit->document()->setModified(false);
        _edit->blockSignals(false);
    }

    void resetToDefaultTemplate()
    {
        if (_edit->document()->isModified())
        {
            setDefaultHtml();
        }
    }

   private:
    QTextEdit* _edit = nullptr;
    QToolBar* _toolbar = nullptr;

    QAction* _actBold = nullptr;
    QAction* _actItalic = nullptr;
    QAction* _actUnderline = nullptr;

    QAction* _actUndo = nullptr;
    QAction* _actRedo = nullptr;

    QAction* _actBulletList = nullptr;
    QAction* _actNumberedList = nullptr;

    QAction* _actIndent = nullptr;
    QAction* _actOutdent = nullptr;

    QAction* _actReset = nullptr;
};
