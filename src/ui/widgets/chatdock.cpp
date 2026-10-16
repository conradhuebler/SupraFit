// chatdock.cpp - see chatdock.h
// Copyright (C) 2016 - 2026 Conrad Hübler <Conrad.Huebler@gmx.net>
//
// Claude Generated 2026 (chat UI: UI redesign WP9)
//
// Transcript design: one widget per entry in a QScrollArea instead of a single QTextBrowser.
// Each bubble is a QLabel with Qt::MarkdownText (QTextDocument underneath, GitHub dialect, so
// pipe tables, lists, headings and code fences render) and can be aligned, sized (85 % of the
// width) and coloured on its own; the colours are mixed from the palette at paint time, so the
// transcript follows the light/dark switch without style sheets. Streaming only replaces the
// text of the last bubble (throttled), so nothing above it moves.
#include "chatdock.h"

#include "src/agent/agentrunner.h"
#include "src/agent/chatrecorder.h"
#include "src/agent/llmclient.h"
#include "src/agent/llmconfig.h"
#include "src/agent/llmsession.h"
#include "src/agent/tooldispatcher.h"
#include "src/agent/toolregistry.h"
#include "src/agent/tools_analysis.h"
#include "src/agent/tools_inspect.h"
#include "src/agent/tools_project.h"
#include "src/core/projectmanager.h"
#include "src/version.h"
#include "src/ui/dialogs/endpointdialog.h"
#include "src/ui/guitools/flowlayout.h"

#include <QApplication>
#include <QClipboard>
#include <QComboBox>
#include <QContextMenuEvent>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QJsonDocument>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QLabel>
#include <QLineEdit>
#include <QMainWindow>
#include <QMenu>
#include <QMessageBox>
#include <QPainter>
#include <QResizeEvent>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QScrollArea>
#include <QScrollBar>
#include <QSettings>
#include <QSet>
#include <QShortcut>
#include <QStandardPaths>
#include <QTextBrowser>
#include <QTextDocument>
#include <QTimer>
#include <QUrl>
#include <QToolButton>
#include <QVBoxLayout>

#include <cmath>
#include <algorithm>
#include <functional>

namespace {

QColor mix(const QColor& a, const QColor& b, double t)
{
    return QColor::fromRgbF(a.redF() * (1 - t) + b.redF() * t, a.greenF() * (1 - t) + b.greenF() * t,
        a.blueF() * (1 - t) + b.blueF() * t);
}

QString compactJson(const QJsonObject& object, int limit = 120)
{
    QString text = QString::fromUtf8(QJsonDocument(object).toJson(QJsonDocument::Compact));
    if (text.size() > limit)
        text = text.left(limit - 3) + QStringLiteral("...");
    return text;
}

} // namespace

// ---------------------------------------------------------------------------------------------
/// One question or answer: rounded bubble with the Markdown text and a small time stamp.
/// The text is a frameless, transparent QTextBrowser whose height is set from its document,
/// which keeps wrapping, tables, selection and links consistent at any width.
class MessageBubble : public QFrame {
public:
    MessageBubble(const ChatEntry& entry, QWidget* parent = nullptr)
        : QFrame(parent)
        , m_user(entry.kind == ChatEntry::Kind::User)
    {
        m_text = new QTextBrowser;
        m_text->setFrameShape(QFrame::NoFrame);
        m_text->setOpenExternalLinks(true);
        m_text->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        m_text->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        m_text->viewport()->setAutoFillBackground(false);
        m_text->setAutoFillBackground(false);
        m_text->setContextMenuPolicy(Qt::NoContextMenu); // the bubble offers copy actions
        m_text->document()->setDocumentMargin(0);
        m_time = new QLabel(entry.time.toString(QStringLiteral("HH:mm")));
        m_time->setForegroundRole(QPalette::PlaceholderText);
        QFont small = m_time->font();
        small.setPointSizeF(small.pointSizeF() * 0.85);
        m_time->setFont(small);
        m_time->setAlignment(m_user ? Qt::AlignRight : Qt::AlignLeft);

        auto* layout = new QVBoxLayout;
        layout->setContentsMargins(12, 8, 12, 6);
        layout->setSpacing(2);
        layout->addWidget(m_text);
        layout->addWidget(m_time);
        setLayout(layout);
        setText(entry.text);
    }

    bool isUser() const { return m_user; }
    QString text() const { return m_raw; }

    void setText(const QString& text)
    {
        m_raw = text;
        if (m_user)
            m_text->setPlainText(text);
        else
            m_text->setMarkdown(text);
    }

    /// Height for the current width: the document wrapped at the text width plus the time stamp.
    void fitHeight()
    {
        const QMargins m = layout()->contentsMargins();
        const int textWidth = width() - m.left() - m.right();
        if (textWidth <= 0)
            return;
        m_text->document()->setTextWidth(textWidth);
        const int text = int(std::ceil(m_text->document()->size().height())) + 2;
        m_text->setFixedHeight(text);
        setFixedHeight(text + m_time->sizeHint().height() + layout()->spacing() + m.top() + m.bottom());
    }

    /// Width the text would like without wrapping (plus padding), for the 85 % rule.
    int naturalWidth() const
    {
        QTextDocument doc;
        doc.setDefaultFont(m_text->font());
        doc.setDocumentMargin(0);
        if (m_user)
            doc.setPlainText(m_raw);
        else
            doc.setMarkdown(m_raw);
        return int(std::ceil(doc.idealWidth())) + 24 + 6;
    }

protected:
    /// The width is final only once the bubble is laid out: fit the height again then (a new
    /// question showed only its first line otherwise).
    void resizeEvent(QResizeEvent* event) override
    {
        QFrame::resizeEvent(event);
        if (event->size().width() != event->oldSize().width())
            fitHeight();
    }

    void paintEvent(QPaintEvent*) override
    {
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing);
        const QPalette p = palette();
        const QColor fill = m_user ? mix(p.color(QPalette::Base), p.color(QPalette::Highlight), 0.25)
                                   : mix(p.color(QPalette::Base), p.color(QPalette::Text), 0.06);
        painter.setPen(QPen(mix(fill, p.color(QPalette::Text), 0.12), 1));
        painter.setBrush(fill);
        painter.drawRoundedRect(QRectF(rect()).adjusted(0.5, 0.5, -0.5, -0.5), 10, 10);
    }

    void contextMenuEvent(QContextMenuEvent* event) override
    {
        QMenu menu(this);
        menu.addAction(QObject::tr("Copy message"), [this]() { QApplication::clipboard()->setText(m_raw); });
        const QString selection = m_text->textCursor().selectedText();
        if (!selection.isEmpty())
            menu.addAction(QObject::tr("Copy selection"), [selection]() { QApplication::clipboard()->setText(selection); });
        menu.exec(event->globalPos());
    }

private:
    bool m_user;
    QString m_raw;
    QTextBrowser* m_text;
    QLabel* m_time;
};

/// A tool call: one muted line (name, short arguments, state); a click shows the details.
class ToolLine : public QWidget {
public:
    explicit ToolLine(const ChatEntry& entry, bool pending, QWidget* parent = nullptr)
        : QWidget(parent)
        , m_entry(entry)
    {
        m_head = new QLabel;
        m_head->setForegroundRole(QPalette::PlaceholderText);
        m_head->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Fixed);
        m_details = new QLabel;
        m_details->setWordWrap(true);
        m_details->setTextInteractionFlags(Qt::TextSelectableByMouse);
        m_details->setForegroundRole(QPalette::PlaceholderText);
        m_details->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);
        m_details->setVisible(false);
        m_details->setIndent(18);
        auto* layout = new QVBoxLayout;
        layout->setContentsMargins(6, 0, 6, 0);
        layout->setSpacing(0);
        layout->addWidget(m_head);
        layout->addWidget(m_details);
        setLayout(layout);
        setCursor(Qt::PointingHandCursor);
        setToolTip(QObject::tr("Click to show the arguments and the result of this tool call"));
        update(entry, pending);
    }

    void update(const ChatEntry& entry, bool pending)
    {
        m_entry = entry;
        const QString state = pending ? QStringLiteral("…") : (entry.toolOk ? QStringLiteral("✓") : QStringLiteral("✗"));
        m_line = QStringLiteral("⚙ %1  %2  %3").arg(entry.toolName, compactJson(entry.toolArgs, 200), state);
        m_details->setText(QObject::tr("Arguments: %1\nResult: %2")
                               .arg(QString::fromUtf8(QJsonDocument(entry.toolArgs).toJson(QJsonDocument::Compact)),
                                   pending ? QObject::tr("running") : entry.text));
        elide();
    }

protected:
    void mousePressEvent(QMouseEvent* event) override
    {
        m_details->setVisible(!m_details->isVisible());
        fitDetails();
        QWidget::mousePressEvent(event);
    }
    void resizeEvent(QResizeEvent* event) override
    {
        QWidget::resizeEvent(event);
        elide();
        fitDetails();
    }
    void fitDetails()
    {
        if (m_details->isVisible())
            m_details->setFixedHeight(m_details->heightForWidth(qMax(60, width() - 12)));
    }

private:
    void elide()
    {
        const QString state = m_line.right(1);
        const QString body = m_line.left(m_line.size() - 1);
        const int width = qMax(40, m_head->width() - m_head->fontMetrics().horizontalAdvance(state + QStringLiteral("  ")));
        m_head->setText(m_head->fontMetrics().elidedText(body, Qt::ElideRight, width) + QStringLiteral(" ") + state);
    }

    ChatEntry m_entry;
    QString m_line;
    QLabel *m_head, *m_details;
};

/// Transcript: scroll area with one widget per entry; follows the end unless the user scrolled up.
class ChatView : public QScrollArea {
public:
    explicit ChatView(QWidget* parent = nullptr)
        : QScrollArea(parent)
    {
        setWidgetResizable(true);
        setFrameShape(QFrame::NoFrame);
        setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        m_content = new QWidget;
        m_content->setBackgroundRole(QPalette::Base);
        m_content->setAutoFillBackground(true);
        m_layout = new QVBoxLayout;
        m_layout->setContentsMargins(8, 8, 8, 8);
        m_layout->setSpacing(8);
        m_layout->addStretch();
        m_content->setLayout(m_layout);
        setWidget(m_content);

        connect(verticalScrollBar(), &QScrollBar::rangeChanged, this, [this](int, int max) {
            if (m_follow)
                verticalScrollBar()->setValue(max);
        });
        // only a user action (wheel, drag, click) decides whether to keep following the end;
        // layout changes that move the value do not
        connect(verticalScrollBar(), &QScrollBar::actionTriggered, this, [this](int) {
            QTimer::singleShot(0, this, [this]() {
                m_follow = verticalScrollBar()->value() >= verticalScrollBar()->maximum() - 8;
            });
        });
    }

    void clear()
    {
        while (m_layout->count() > 1) {
            QLayoutItem* item = m_layout->takeAt(0);
            if (item->widget())
                item->widget()->deleteLater();
            delete item;
        }
        m_bubbles.clear();
        m_tools.clear();
        m_notices.clear();
        m_follow = true;
    }

    MessageBubble* addMessage(const ChatEntry& entry)
    {
        auto* bubble = new MessageBubble(entry);
        auto* row = new QHBoxLayout;
        row->setContentsMargins(0, 0, 0, 0);
        if (bubble->isUser())
            row->addStretch();
        row->addWidget(bubble);
        if (!bubble->isUser())
            row->addStretch();
        auto* holder = new QWidget;
        holder->setLayout(row);
        insert(holder);
        m_bubbles.append(bubble);
        sizeBubble(bubble);
        return bubble;
    }

    ToolLine* addTool(const ChatEntry& entry, bool pending)
    {
        auto* line = new ToolLine(entry, pending);
        insert(line);
        m_tools.append(line);
        return line;
    }

    void addNotice(const ChatEntry& entry, bool error)
    {
        auto* label = new QLabel(entry.text);
        label->setWordWrap(true);
        label->setAlignment(Qt::AlignHCenter);
        label->setTextInteractionFlags(Qt::TextSelectableByMouse);
        QFont font = label->font();
        font.setItalic(true);
        label->setFont(font);
        if (error) {
            QPalette p = label->palette();
            p.setColor(QPalette::WindowText, QColor(198, 40, 40)); // error red, readable on light and dark
            label->setPalette(p);
        } else {
            label->setForegroundRole(QPalette::PlaceholderText);
        }
        insert(label);
        m_notices.append(label);
        sizeNotice(label);
    }

    void add(const ChatEntry& entry)
    {
        switch (entry.kind) {
        case ChatEntry::Kind::User:
        case ChatEntry::Kind::Assistant:
            addMessage(entry);
            break;
        case ChatEntry::Kind::Tool:
            addTool(entry, false);
            break;
        case ChatEntry::Kind::Notice:
            addNotice(entry, false);
            break;
        }
    }

    MessageBubble* lastBubble() const { return m_bubbles.isEmpty() ? nullptr : m_bubbles.last(); }
    ToolLine* lastTool() const { return m_tools.isEmpty() ? nullptr : m_tools.last(); }
    /// Word-wrapped labels need their height set: the scroll area sizes its content by size hints,
    /// not by height-for-width.
    void sizeNotice(QLabel* label)
    {
        const int width = viewport()->width() - 16;
        label->setFixedHeight(label->heightForWidth(width));
    }
    void sizeBubble(MessageBubble* bubble)
    {
        const int max = int(viewport()->width() * 0.85) - 16;
        bubble->setFixedWidth(qMax(120, qMin(max, bubble->naturalWidth())));
        bubble->fitHeight();
    }
    void scrollToEnd()
    {
        m_follow = true;
        verticalScrollBar()->setValue(verticalScrollBar()->maximum());
        // once more after the pending layout pass has settled the heights
        QTimer::singleShot(0, this, [this]() { verticalScrollBar()->setValue(verticalScrollBar()->maximum()); });
    }

protected:
    void resizeEvent(QResizeEvent* event) override
    {
        QScrollArea::resizeEvent(event);
        for (MessageBubble* bubble : std::as_const(m_bubbles))
            sizeBubble(bubble);
        for (QLabel* label : std::as_const(m_notices))
            sizeNotice(label);
    }

private:
    void insert(QWidget* widget)
    {
        // never wider than the view: long lines wrap, elide or clip instead of scrolling sideways
        // and never taller than needed: spare height goes to the stretch above the first entry
        widget->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Maximum);
        m_layout->insertWidget(m_layout->count() - 1, widget);
    }

    QWidget* m_content;
    QVBoxLayout* m_layout;
    QList<MessageBubble*> m_bubbles;
    QList<ToolLine*> m_tools;
    QList<QLabel*> m_notices;
    bool m_follow = true;
};

/// Input field: grows from one to six lines; Enter sends, Shift+Enter breaks the line,
/// Up/Down on the first/last line walk through earlier inputs, Esc clears.
class ChatInput : public QPlainTextEdit {
public:
    ChatInput(InputHistory* history, QWidget* parent = nullptr)
        : QPlainTextEdit(parent)
        , m_history(history)
    {
        setPlaceholderText(QObject::tr("Ask about the open projects…"));
        setToolTip(QObject::tr("Enter sends, Shift+Enter starts a new line, Up/Down recall earlier questions, Esc clears"));
        setTabChangesFocus(true);
        connect(document(), &QTextDocument::contentsChanged, this, [this]() { adjustHeight(); });
        adjustHeight();
    }

    std::function<void()> onSend;
    std::function<void()> onEscape;

protected:
    void keyPressEvent(QKeyEvent* event) override
    {
        const bool enter = event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter;
        if (enter && !(event->modifiers() & Qt::ShiftModifier)) {
            if (onSend)
                onSend();
            return;
        }
        if (event->key() == Qt::Key_Up && textCursor().blockNumber() == 0 && !(event->modifiers() & Qt::ShiftModifier)) {
            replace(m_history->previous(toPlainText()));
            return;
        }
        if (event->key() == Qt::Key_Down && m_history->isBrowsing()
            && textCursor().blockNumber() == document()->blockCount() - 1) {
            replace(m_history->next());
            return;
        }
        if (event->key() == Qt::Key_Escape) {
            if (!toPlainText().isEmpty()) {
                clear();
                m_history->resetBrowsing();
            } else if (onEscape) {
                onEscape();
            }
            return;
        }
        QPlainTextEdit::keyPressEvent(event);
    }

    void resizeEvent(QResizeEvent* event) override
    {
        QPlainTextEdit::resizeEvent(event);
        adjustHeight();
    }

private:
    void replace(const QString& text)
    {
        setPlainText(text);
        moveCursor(QTextCursor::End);
    }

    void adjustHeight()
    {
        const int lines = qBound(1, int(document()->size().height()), 6);
        const int margins = int(document()->documentMargin() * 2) + frameWidth() * 2 + contentsMargins().top() + contentsMargins().bottom();
        setFixedHeight(lines * fontMetrics().lineSpacing() + margins + 2);
    }

    InputHistory* m_history;
};

// ---------------------------------------------------------------------------------------------

ChatDock::ChatDock(QWidget* parent)
    : QDockWidget(tr("Agent"), parent)
{
    setObjectName(QStringLiteral("chat_dock"));
    setMinimumWidth(300);

    // ---- header: selectors flow (and wrap when narrow), actions stay on the right ----
    m_profile = new QComboBox;
    m_profile->setToolTip(tr("LLM endpoint profile (llm.json)"));
    m_model = new QComboBox;
    m_model->setEditable(true);
    m_model->setMinimumContentsLength(14);
    m_model->setSizeAdjustPolicy(QComboBox::AdjustToMinimumContentsLengthWithIcon);
    m_model->setToolTip(tr("Model served by the endpoint (empty = profile default)"));
    m_refresh = new QPushButton(tr("Models"));
    m_refresh->setToolTip(tr("Ask the endpoint which models it offers"));
    m_autonomy = new QComboBox;
    m_autonomy->addItem(tr("Ask for fits"), int(ToolAutonomy::Ask));
    m_autonomy->addItem(tr("Run in program"), int(ToolAutonomy::InProgram));
    m_autonomy->addItem(tr("Full"), int(ToolAutonomy::Full));
    m_autonomy->setToolTip(tr("Which tool calls run without asking"));

    m_maxButton = new QToolButton;
    m_maxButton->setText(QStringLiteral("⛶"));
    m_maxButton->setAutoRaise(true);
    m_maxButton->setCheckable(true);
    m_maxButton->setToolTip(tr("Enlarge the chat to a large window (Esc returns)"));

    m_menu = new QMenu(this);
    m_menu->addAction(tr("Endpoints…"), this, &ChatDock::editEndpoints);
    m_menu->addSeparator();
    m_menu->addAction(tr("New chat"), this, &ChatDock::newChat);
    m_historyMenu = m_menu->addMenu(tr("History"));
    connect(m_historyMenu, &QMenu::aboutToShow, this, &ChatDock::fillHistoryMenu);
    m_menu->addSeparator();
    m_menu->addAction(tr("Export as Markdown…"), this, &ChatDock::exportMarkdown);
    m_menu->addAction(tr("Export as JSON…"), this, &ChatDock::exportJson);
    m_menu->addAction(tr("Import JSON…"), this, &ChatDock::importJson);
    m_menu->addSeparator();
    m_menu->addAction(tr("Copy last answer"), this, &ChatDock::copyLastAnswer);
    QAction* removeChats = m_menu->addAction(tr("Remove chats from this project…"), this, &ChatDock::removeProjectChats);
    removeChats->setToolTip(tr("Project files contain the agent conversations; remove them before sharing a project if needed"));
    m_menu->setToolTipsVisible(true);
    m_menu->addAction(tr("Enlarge / restore"), this, &ChatDock::toggleMaximized);
    m_menuButton = new QToolButton;
    m_menuButton->setText(QStringLiteral("☰"));
    m_menuButton->setAutoRaise(true);
    m_menuButton->setPopupMode(QToolButton::InstantPopup);
    m_menuButton->setMenu(m_menu);
    m_menuButton->setToolTip(tr("New chat, history, export and import"));

    auto* selectors = new QWidget;
    auto* flow = new FlowLayout(0, 6, 4);
    flow->addWidget(m_profile);
    flow->addWidget(m_model);
    flow->addWidget(m_refresh);
    flow->addWidget(m_autonomy);
    selectors->setLayout(flow);
    auto* header = new QHBoxLayout;
    header->addWidget(selectors, 1);
    header->addWidget(m_maxButton, 0, Qt::AlignTop);
    header->addWidget(m_menuButton, 0, Qt::AlignTop);

    // ---- transcript and input ----
    m_view = new ChatView;
    m_input = new ChatInput(&m_inputHistory);
    m_send = new QPushButton(tr("Send"));
    m_send->setToolTip(tr("Send the question (Enter)"));
    m_stop = new QPushButton(tr("Stop"));
    m_stop->setToolTip(tr("Stop the running answer"));
    m_stop->setEnabled(false);
    m_status = new QLabel;
    m_status->setForegroundRole(QPalette::PlaceholderText);
    m_status->setWordWrap(true);

    auto* buttons = new QVBoxLayout;
    buttons->addWidget(m_send);
    buttons->addWidget(m_stop);
    buttons->addStretch();
    auto* bottom = new QHBoxLayout;
    bottom->addWidget(m_input, 1);
    bottom->addLayout(buttons);
    auto* layout = new QVBoxLayout;
    layout->setContentsMargins(6, 4, 6, 4);
    m_identity = new QLabel;
    m_identity->setForegroundRole(QPalette::PlaceholderText);
    m_identity->setWordWrap(true);
    m_identity->setToolTip(tr("Endpoint and model the next question goes to"));
    layout->addLayout(header);
    layout->addWidget(m_identity);
    layout->addWidget(m_view, 1);
    layout->addLayout(bottom);
    layout->addWidget(m_status);
    auto* holder = new QWidget;
    holder->setLayout(layout);
    setWidget(holder);

    // ---- agent ----
    m_registry = new ToolRegistry;
    registerProjectTools(*m_registry, &SupraFit::ProjectManager::instance());
    registerAnalysisTools(*m_registry, &SupraFit::ProjectManager::instance());
    registerInspectTools(*m_registry, &SupraFit::ProjectManager::instance());
    m_dispatcher = new ToolDispatcher(m_registry, this);
    m_client = new LlmClient(this);
    m_session = new LlmSession(m_client, m_registry, m_dispatcher, this);
    m_session->setSystemPrompt(agentSystemPrompt());
    m_session->setKeepHistory(true); // a chat: follow-up questions see the earlier turns
    m_streamTimer = new QTimer(this);
    m_streamTimer->setSingleShot(true);
    m_streamTimer->setInterval(90);
    connect(m_streamTimer, &QTimer::timeout, this, &ChatDock::flushStream);

    connect(m_session, &LlmSession::assistantChunk, this, [this](const QString& t) {
        if (!m_streamOpen) {
            m_streamOpen = true;
            m_streamText.clear();
            m_log.append(ChatEntry::Kind::Assistant, QString());
            m_view->addMessage(*m_log.lastEntry());
        }
        m_streamText += t;
        if (!m_streamTimer->isActive())
            m_streamTimer->start();
    });
    connect(m_session, &LlmSession::assistantMessage, this, [this](const QString& t) {
        m_streamTimer->stop();
        if (m_streamOpen) {
            m_streamText = t.isEmpty() ? m_streamText : t;
            flushStream();
            m_streamOpen = false;
        } else if (!t.trimmed().isEmpty()) {
            m_view->addMessage(m_log.append(ChatEntry::Kind::Assistant, t));
        }
    });
    connect(m_session, &LlmSession::toolStarted, this, [this](const QString&, const QJsonObject&) {
        if (m_streamOpen) { // text before a tool call closes that bubble (before the recorder adds the tool)
            m_streamTimer->stop();
            flushStream();
            m_streamOpen = false;
        }
    });
    // The audit trail (tool, arguments, effect, full result, duration, approval) is written by the
    // same recorder as in suprafit_cli --agent; the view only follows it (WP11).
    m_recorder = new ChatRecorder(m_session, m_registry, this);
    m_recorder->setLog(&m_log);
    m_recorder->onToolAdded = [this](int index) {
        if (const ChatEntry* entry = m_log.entry(index))
            m_view->addTool(*entry, entry->toolResult.isEmpty() && entry->approval.isEmpty());
    };
    m_recorder->onToolChanged = [this](int index) {
        if (const ChatEntry* entry = m_log.entry(index))
            if (ToolLine* line = m_view->lastTool())
                line->update(*entry, false);
    };
    m_session->setApprovalPolicy(m_recorder->wrapApproval([this](const ToolSpec& spec, const QJsonObject& args) {
        const QString text = tr("The agent wants to run '%1'.\n\n%2\n\nAllow?")
                                 .arg(spec.name, QString::fromUtf8(QJsonDocument(args).toJson(QJsonDocument::Compact)));
        return QMessageBox::question(this, tr("Agent"), text) == QMessageBox::Yes;
    }, QStringLiteral("user")));
    connect(m_session, &LlmSession::failed, this, [this](const QString& e) {
        addNotice(e, true);
        finishTurn();
    });
    connect(m_session, &LlmSession::finished, this, &ChatDock::finishTurn);

    // ---- endpoint and model selection (WP10) ----
    // Former bug ("only glm can be selected"): the model list was only filled after pressing
    // "Models", and every profile change reset the field to the profile's default model, so the
    // only entry ever offered was the profile's glm. Now each profile switch lists the models of
    // that endpoint at once and shows the model last chosen for it (LlmModelSelection).
    connect(m_client, &LlmClient::modelListFailed, this, [this](const QString& error) {
        m_listError = error;
    });
    connect(m_client, &LlmClient::modelsListed, this, [this](const QStringList& models) {
        m_listed = models;
        const QString keep = m_model->currentText().trimmed();
        m_model->blockSignals(true);
        m_model->clear();
        for (const QString& id : models)
            m_model->addItem(id);
        LlmProfile profile = m_client->profile();
        const QString chosen = keep.isEmpty() ? m_selection.choose(profile, models) : keep;
        // select the item itself: an edit text over another current item would be replaced as
        // soon as that item changes (capability icons arrive later)
        const int index = m_model->findText(chosen);
        m_model->setCurrentIndex(index);
        if (index < 0)
            m_model->setEditText(chosen);
        m_model->blockSignals(false);
        applyModel(chosen, false);
        if (!m_listError.isEmpty())
            m_listStatus = tr("model list failed: %1").arg(m_listError);
        else
            m_listStatus = models.isEmpty() ? tr("no model list (type the model)") : tr("%n model(s)", "", models.size());
        m_listError.clear();
        updateIdentity();
        m_client->queryCapabilities(models);
    });
    connect(m_client, &LlmClient::modelInfo, this, [this](const LlmModelInfo& info) {
        m_infos.insert(info.id, info);
        const int index = m_model->findText(info.id);
        if (index >= 0 && info.detailsKnown) {
            m_model->setItemIcon(index, toolIcon(info.supportsTools));
            m_model->setItemData(index, info.supportsTools ? tr("supports tools") + (info.contextLength ? tr(", context %1 tokens").arg(info.contextLength) : QString())
                                                           : tr("reports no tool support: the agent cannot work with it"),
                Qt::ToolTipRole);
        }
        if (info.id == m_model->currentText().trimmed())
            updateIdentity();
    });
    connect(m_refresh, &QPushButton::clicked, this, [this] { listModelsForProfile(); });
    connect(m_profile, &QComboBox::currentTextChanged, this, [this](const QString&) { listModelsForProfile(); });
    connect(m_model, &QComboBox::activated, this, [this](int) { applyModel(m_model->currentText(), true); });
    connect(m_model->lineEdit(), &QLineEdit::editingFinished, this, [this]() { applyModel(m_model->currentText(), true); });
    connect(m_send, &QPushButton::clicked, this, &ChatDock::send);
    connect(m_stop, &QPushButton::clicked, this, &ChatDock::stop);
    connect(m_maxButton, &QToolButton::clicked, this, &ChatDock::toggleMaximized);
    m_input->onSend = [this]() { send(); };
    m_input->onEscape = [this]() {
        if (m_maximized)
            toggleMaximized();
    };
    auto* escape = new QShortcut(QKeySequence(Qt::Key_Escape), this, nullptr, nullptr, Qt::WidgetWithChildrenShortcut);
    connect(escape, &QShortcut::activated, this, [this]() {
        if (m_maximized)
            toggleMaximized();
    });

    QSettings settings;
    resetLog();
    m_inputHistory.setItems(settings.value(QStringLiteral("chat/input_history")).toStringList());
    {
        QMap<QString, QString> remembered;
        settings.beginGroup(QStringLiteral("chat/models"));
        for (const QString& key : settings.childKeys())
            remembered.insert(QUrl::fromPercentEncoding(key.toUtf8()), settings.value(key).toString());
        settings.endGroup();
        m_selection.setRemembered(remembered);
    }
    reloadProfiles();
}

ChatDock::~ChatDock()
{
    m_session->cancel();
    autosave();
    delete m_registry; // the dispatcher only borrows it; both go with the dock
}

void ChatDock::reloadProfiles()
{
    m_profile->clear();
    LlmConfig config;
    QString error;
    if (!config.load(LlmConfig::defaultPath(), &error)) {
        QString werr;
        LlmConfig::writeExampleIfMissing(LlmConfig::defaultPath(), &werr);
        m_status->setText(tr("No usable LLM config: %1 (example written to %2)")
                              .arg(error, LlmConfig::defaultPath()));
        return;
    }
    const QString keep = m_profile->property("keep").toString();
    m_profile->blockSignals(true);
    for (const LlmProfile& p : config.profiles()) {
        m_profile->addItem(p.name);
        m_profile->setItemData(m_profile->count() - 1, LlmConfig::displayName(p) + QStringLiteral("\n") + p.baseUrl, Qt::ToolTipRole);
    }
    m_profile->setCurrentText(keep.isEmpty() || m_profile->findText(keep) < 0 ? config.activeProfileName() : keep);
    m_profile->blockSignals(false);
    m_status->clear();
    listModelsForProfile();
}

QIcon ChatDock::toolIcon(bool tools)
{
    QPixmap pixmap(12, 12);
    pixmap.fill(Qt::transparent);
    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setPen(Qt::NoPen);
    painter.setBrush(tools ? QColor(46, 160, 67) : QColor(150, 150, 150)); // green: tools, grey: none
    painter.drawEllipse(QRectF(1, 1, 10, 10));
    return QIcon(pixmap);
}

bool ChatDock::currentProfile(LlmProfile& profile) const
{
    LlmConfig config;
    return config.load(LlmConfig::defaultPath()) && config.profile(m_profile->currentText(), profile);
}

void ChatDock::listModelsForProfile()
{
    LlmProfile profile;
    if (!currentProfile(profile)) {
        m_listStatus = tr("no usable profile");
        updateIdentity();
        return;
    }
    m_profile->setProperty("keep", profile.name);
    m_client->setProfile(profile); // drops a model chosen for another profile
    m_client->setApiKey(SessionKeys::effectiveKey(profile));
    m_infos.clear();
    m_listed.clear();
    // the remembered (or default) model at once, so the field never shows the previous profile's
    m_model->blockSignals(true);
    m_model->clear();
    m_model->setEditText(m_selection.choose(profile));
    m_model->blockSignals(false);
    m_listStatus = tr("loading models…");
    updateIdentity();
    if (!LlmConfig::missingKeyMessage(profile).isEmpty() && SessionKeys::key(profile.name).isEmpty()) {
        m_listStatus = tr("key missing: set %1").arg(profile.apiKeyEnv);
        updateIdentity();
        return;
    }
    m_client->listModels();
}

void ChatDock::applyModel(const QString& model, bool remember)
{
    const QString id = model.trimmed();
    m_client->setModel(id);
    if (remember) {
        m_selection.remember(m_client->profile().name, id);
        QSettings settings;
        settings.beginGroup(QStringLiteral("chat/models"));
        const QString key = QString::fromUtf8(QUrl::toPercentEncoding(m_client->profile().name));
        if (id.isEmpty())
            settings.remove(key);
        else
            settings.setValue(key, id);
        settings.endGroup();
    }
    updateIdentity();
}

void ChatDock::updateIdentity()
{
    const LlmProfile profile = m_client->profile();
    const QString model = m_model->currentText().trimmed();
    QString text = LlmConfig::displayName(profile) + QStringLiteral(" · ") + (model.isEmpty() ? tr("no model chosen") : model);
    if (!m_listStatus.isEmpty())
        text += QStringLiteral(" · ") + m_listStatus;
    const auto info = m_infos.constFind(model);
    if (info != m_infos.constEnd() && info->detailsKnown && !info->supportsTools)
        text += QStringLiteral("\n") + tr("⚠ %1 reports no tool support; the agent needs tools to read the projects.").arg(model);
    if (!LlmConfig::isLocalEndpoint(profile) && profile.isValid())
        text += QStringLiteral("\n") + tr("Remote endpoint: questions and project data go to %1").arg(QUrl(profile.baseUrl).host());
    m_identity->setText(text);
}

void ChatDock::editEndpoints()
{
    EndpointDialog dialog(LlmConfig::defaultPath(), this);
    if (dialog.exec() == QDialog::Accepted)
        reloadProfiles();
}

bool ChatDock::prepareClient()
{
    LlmConfig config;
    QString error;
    LlmProfile profile;
    if (!config.load(LlmConfig::defaultPath(), &error) || !config.profile(m_profile->currentText(), profile)) {
        addNotice(tr("LLM config unusable: %1").arg(error), true);
        return false;
    }
    const QString keyProblem = LlmConfig::missingKeyMessage(profile);
    if (!keyProblem.isEmpty() && SessionKeys::key(profile.name).isEmpty()) {
        addNotice(keyProblem, true);
        return false;
    }
    if (!LlmConfig::isLocalEndpoint(profile))
        addNotice(tr("Remote endpoint %1: the conversation and the project data the agent reads are sent there.")
                      .arg(profile.baseUrl));
    m_client->setProfile(profile);
    m_client->setApiKey(SessionKeys::effectiveKey(profile));
    m_client->setModel(m_model->currentText().trimmed());
    if (m_client->model().isEmpty()) {
        addNotice(tr("No model chosen: pick one from the list or type its name."), true);
        return false;
    }
    m_session->setMaxIterations(profile.maxToolIterations);
    m_session->setAutonomy(ToolAutonomy(m_autonomy->currentData().toInt()));
    m_log.setProfile(profile.name);
    m_log.setModel(m_client->model());
    m_log.setEndpointHost(QUrl(profile.baseUrl).host());
    m_log.setAutonomy(toolAutonomyName(ToolAutonomy(m_autonomy->currentData().toInt())));
    m_recorder->setAutonomy(ToolAutonomy(m_autonomy->currentData().toInt()));
    return true;
}

void ChatDock::askQuestion(const QString& text)
{
    m_input->setPlainText(text);
    send();
}

void ChatDock::setAutonomyIndex(int index)
{
    if (index >= 0 && index < m_autonomy->count())
        m_autonomy->setCurrentIndex(index);
}

void ChatDock::send()
{
    const QString question = m_input->toPlainText().trimmed();
    if (question.isEmpty() || m_session->isBusy() || !prepareClient())
        return;
    m_inputHistory.push(question);
    QSettings().setValue(QStringLiteral("chat/input_history"), m_inputHistory.items());
    m_input->clear();
    m_view->addMessage(m_log.append(ChatEntry::Kind::User, question));
    m_view->scrollToEnd();
    m_streamOpen = false;
    setRunning(true);
    m_session->ask(question);
}

void ChatDock::stop()
{
    m_session->cancel();
    finishTurn();
}

void ChatDock::finishTurn()
{
    if (m_streamOpen) {
        m_streamTimer->stop();
        flushStream();
        m_streamOpen = false;
    }
    setRunning(false);
    autosave();
}

void ChatDock::flushStream()
{
    ChatEntry* entry = m_log.lastEntry();
    MessageBubble* bubble = m_view->lastBubble();
    if (!entry || !bubble || entry->kind != ChatEntry::Kind::Assistant)
        return;
    entry->text = m_streamText;
    bubble->setText(m_streamText);
    m_view->sizeBubble(bubble);
}

void ChatDock::setRunning(bool running)
{
    m_send->setEnabled(!running);
    m_input->setReadOnly(running);
    m_stop->setEnabled(running);
    m_status->setText(running ? tr("Working...") : QString());
}

void ChatDock::addNotice(const QString& text, bool error, bool store)
{
    ChatEntry entry;
    entry.kind = ChatEntry::Kind::Notice;
    entry.text = text;
    entry.time = QDateTime::currentDateTime();
    if (store)
        entry = m_log.append(ChatEntry::Kind::Notice, text);
    m_view->addNotice(entry, error);
}

void ChatDock::autosave()
{
    if (m_log.isEmpty())
        return;
    m_log.setConversation(m_session->conversation());
    m_log.touch();
    // the project copy is the record (WP11); the AppData copy is a convenience for the History
    if (!m_projectId.isEmpty() && storeInProjects()
        && SupraFit::ProjectManager::instance().getLoadedProjectIds().contains(m_projectId))
        ProjectChats::store(m_projectId, m_log);
    QString error;
    if (!m_store.save(m_log, &error))
        m_status->setText(tr("Chat not saved: %1").arg(error));
}

bool ChatDock::storeInProjects()
{
    return qApp->property("store_agent_chats").isValid() ? qApp->property("store_agent_chats").toBool() : true;
}

void ChatDock::resetLog()
{
    m_log = ChatLog();
    m_log.setAppVersion(git_tag, git_commit_hash);
    m_log.addProjectUuid(m_projectId);
    m_recorder->setLog(&m_log);
}

void ChatDock::showLog(const ChatLog& log)
{
    m_view->clear();
    for (const ChatEntry& entry : log.entries())
        m_view->add(entry);
    m_view->scrollToEnd();
}

void ChatDock::newChat()
{
    if (m_session->isBusy())
        m_session->cancel();
    finishTurn();
    resetLog();
    m_session->reset();
    m_view->clear();
    if (m_projectId.isEmpty())
        addNotice(tr("No project open: this chat is not stored in a project file."), false, false);
    m_status->setText(tr("New chat"));
}

void ChatDock::setProject(const QString& projectId)
{
    if (projectId == m_projectId && !m_log.isEmpty())
        return;
    if (m_session->isBusy()) // a running answer stays with its project; switching waits for it
        return;
    autosave();
    m_log = ChatLog(); // stored above; must not be stored again into the next project
    m_recorder->setLog(&m_log);
    m_projectId = projectId;
    // show the project's most recent chat, or start a new one in it
    QStringList warnings;
    const QVector<ChatLog> chats = projectId.isEmpty() ? QVector<ChatLog>() : ProjectChats::load(projectId, &warnings);
    if (chats.isEmpty()) {
        newChat();
    } else {
        const ChatLog* latest = &chats.first();
        for (const ChatLog& chat : chats)
            if (chat.updated() > latest->updated())
                latest = &chat;
        showChat(*latest, true);
    }
    for (const QString& warning : warnings)
        addNotice(warning, true, false);
    updateIdentity();
}

void ChatDock::showChat(const ChatLog& log, bool fromProject)
{
    if (m_session->isBusy())
        m_session->cancel();
    finishTurn();
    m_log = log;
    m_log.addProjectUuid(m_projectId);
    m_recorder->setLog(&m_log);
    m_session->reset();
    m_session->setConversation(log.conversation());
    showLog(m_log);
    if (fromProject)
        addNotice(tr("Chat of %1, stored with this project. The project state is the one saved with it; the conversation can be continued.")
                      .arg(log.updated().toString(QStringLiteral("yyyy-MM-dd HH:mm"))),
            false, false);
    else
        addNotice(tr("Chat of %1 restored from the history. It may belong to another project; ids mentioned above may refer to projects that are not open.")
                      .arg(log.created().toString(QStringLiteral("yyyy-MM-dd HH:mm"))),
            false, false);
    m_view->scrollToEnd();
}

bool ChatDock::openChat(const QString& path, QString* error)
{
    ChatLog log;
    QString why;
    if (!ChatLog::load(path, &log, &why)) {
        if (error)
            *error = why;
        addNotice(tr("Could not open the chat: %1").arg(why), true, false);
        return false;
    }
    autosave();
    showChat(log, false);
    return true;
}

void ChatDock::removeProjectChats()
{
    if (m_projectId.isEmpty())
        return;
    if (QMessageBox::question(this, tr("Remove chats"),
            tr("Remove all agent chats from this project? They disappear from the project file when it is saved next time; copies stay in the History under \"Other chats\".")) != QMessageBox::Yes)
        return;
    ProjectChats::clear(m_projectId);
    resetLog();
    m_session->reset();
    m_view->clear();
    addNotice(tr("The chats of this project were removed (save the project to make it permanent)."), false, false);
}

void ChatDock::fillHistoryMenu()
{
    m_historyMenu->clear();
    QSet<QString> shown;
    if (!m_projectId.isEmpty()) {
        m_historyMenu->addSection(tr("This project"));
        QVector<ChatLog> chats = ProjectChats::load(m_projectId);
        std::sort(chats.begin(), chats.end(), [](const ChatLog& a, const ChatLog& b) { return a.updated() > b.updated(); });
        if (chats.isEmpty())
            m_historyMenu->addAction(tr("No chats stored in this project"))->setEnabled(false);
        for (const ChatLog& chat : chats) {
            shown.insert(chat.id());
            const QString title = chat.title().isEmpty() ? tr("(no question)") : chat.title();
            QAction* action = m_historyMenu->addAction(chat.updated().toString(QStringLiteral("yyyy-MM-dd HH:mm")) + QStringLiteral("  ") + title);
            action->setToolTip(tr("%1 · %2, %n entries", "", chat.entries().size()).arg(chat.profile(), chat.model()));
            const QString id = chat.id();
            connect(action, &QAction::triggered, this, [this, id]() {
                autosave();
                for (const ChatLog& c : ProjectChats::load(m_projectId))
                    if (c.id() == id)
                        showChat(c, true);
            });
        }
    }
    m_historyMenu->addSection(tr("Other chats"));
    int others = 0;
    for (const auto& item : m_store.list()) {
        if (shown.contains(QFileInfo(item.path).completeBaseName()))
            continue;
        const QString title = item.title.isEmpty() ? tr("(no question)") : item.title;
        QAction* action = m_historyMenu->addAction(item.created.toString(QStringLiteral("yyyy-MM-dd HH:mm")) + QStringLiteral("  ") + title);
        action->setToolTip(item.path);
        const QString path = item.path;
        connect(action, &QAction::triggered, this, [this, path]() { openChat(path); });
        ++others;
    }
    if (others == 0)
        m_historyMenu->addAction(tr("No other saved chats"))->setEnabled(false);
}

QString ChatDock::lastDirectory() const
{
    return QSettings().value(QStringLiteral("chat/lastdir"), QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation)).toString();
}

void ChatDock::rememberDirectory(const QString& file)
{
    QSettings().setValue(QStringLiteral("chat/lastdir"), QFileInfo(file).absolutePath());
}

void ChatDock::exportMarkdown()
{
    const QString file = QFileDialog::getSaveFileName(this, tr("Export chat as Markdown"), lastDirectory(), tr("Markdown (*.md)"));
    if (file.isEmpty())
        return;
    rememberDirectory(file);
    QFile out(file);
    if (!out.open(QIODevice::WriteOnly | QIODevice::Text)) {
        addNotice(tr("Export failed: %1").arg(out.errorString()), true, false);
        return;
    }
    out.write(m_log.toMarkdown().toUtf8());
    m_status->setText(tr("Chat exported to %1").arg(file));
}

void ChatDock::exportJson()
{
    const QString file = QFileDialog::getSaveFileName(this, tr("Export chat as JSON"), lastDirectory(), tr("SupraFit chat (*.json)"));
    if (file.isEmpty())
        return;
    rememberDirectory(file);
    m_log.setConversation(m_session->conversation());
    QString error;
    if (!m_log.save(file, &error))
        addNotice(tr("Export failed: %1").arg(error), true, false);
    else
        m_status->setText(tr("Chat exported to %1").arg(file));
}

void ChatDock::importJson()
{
    const QString file = QFileDialog::getOpenFileName(this, tr("Import chat"), lastDirectory(), tr("SupraFit chat (*.json)"));
    if (file.isEmpty())
        return;
    rememberDirectory(file);
    if (!m_log.isEmpty()
        && QMessageBox::question(this, tr("Import chat"), tr("Replace the current chat with the imported one? The current chat stays in the history.")) != QMessageBox::Yes)
        return;
    autosave();
    openChat(file);
}

void ChatDock::copyLastAnswer()
{
    const QString answer = m_log.lastText(ChatEntry::Kind::Assistant);
    QApplication::clipboard()->setText(answer);
    m_status->setText(answer.isEmpty() ? tr("No answer to copy yet") : tr("Last answer copied"));
}

void ChatDock::toggleMaximized()
{
    QWidget* window = parentWidget() ? parentWidget()->window() : nullptr;
    if (!m_maximized) {
        m_wasFloating = isFloating();
        m_previousGeometry = geometry();
        setFloating(true);
        if (window) {
            const QRect area = window->geometry();
            const QSize size(int(area.width() * 0.8), int(area.height() * 0.8));
            setGeometry(QRect(area.center() - QPoint(size.width() / 2, size.height() / 2), size));
        }
        show();
        raise();
        m_maximized = true;
    } else {
        if (m_wasFloating)
            setGeometry(m_previousGeometry);
        else
            setFloating(false);
        m_maximized = false;
    }
    m_maxButton->setChecked(m_maximized);
}
