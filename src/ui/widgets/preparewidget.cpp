/*
 * <one line to give the program's name and a brief idea of what it does.>
 * Copyright (C) 2022 Conrad Hübler <Conrad.Huebler@gmx.net>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include <QtCore/QJsonObject>
#include <QtCore/QPair>
#include <QtCore/QPointer>
#include <QtCore/QVariant>

#include <QtGui/QFont>

#include <QtWidgets/QDoubleSpinBox>
#include <QtWidgets/QGroupBox>
#include <QtWidgets/QHBoxLayout>
#include <QtGui/QResizeEvent>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtGui/QFontMetrics>
#include <QtWidgets/QListWidget>
#include <QtWidgets/QSplitter>
#include <QtWidgets/QSpinBox>
#include <QtWidgets/QTextEdit>
#include <QtWidgets/QWidget>

#include <QtWidgets/QGridLayout>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QScrollArea>
#include "src/core/models/exprtkinterpreter.h"
#include "src/core/reactionparser.h"
#include "src/core/speciationengine.h"
#include "src/ui/widgets/reactioneditorwidget.h"

#include "preparewidget.h"

Highlighter::Highlighter(QTextDocument* parent)
    : QSyntaxHighlighter(parent)
{
    commentStartExpression = QRegularExpression(QStringLiteral("/\\*"));
    commentEndExpression = QRegularExpression(QStringLiteral("\\*/"));
}

void Highlighter::Highlight(const QString& text, const QStringList& list, QTextCharFormat format)
{
    for (QString string : list) {
        QRegularExpression expr = QRegularExpression(QRegularExpression::escape(string));

        expr.setPatternOptions(QRegularExpression::CaseInsensitiveOption);
        QRegularExpressionMatchIterator matchIterator = expr.globalMatch(text);
        while (matchIterator.hasNext()) {
            QRegularExpressionMatch match = matchIterator.next();
            setFormat(match.capturedStart(), match.capturedLength(), format);
        }
    }
}

void Highlighter::highlightBlock(const QString& text)
{
    QTextCharFormat notfound;
    notfound.setFontUnderline(true);
    QRegularExpression expression(QStringLiteral("\\b[a-z]+\\b"));
    expression.setPatternOptions(QRegularExpression::CaseInsensitiveOption);
    QRegularExpressionMatchIterator matchIterator = expression.globalMatch(text);
    while (matchIterator.hasNext()) {
        QRegularExpressionMatch match = matchIterator.next();
        setFormat(match.capturedStart(), match.capturedLength(), notfound);
    }

    QTextCharFormat format;
    format.setFontUnderline(false);
    format.setForeground(Qt::darkGreen);
    format.setFontWeight(QFont::Light);
    format.setToolTip(tr("Mathematical function or expression."));
    Highlight(text, m_math_functions, format);

    format.setForeground(Qt::darkBlue);
    format.setFontWeight(QFont::Bold);
    format.setToolTip(tr("Independent variable."));

    Highlight(text, m_table_header, format);

    format.setForeground(Qt::darkRed);
    format.setFontWeight(QFont::Bold);
    format.setToolTip(tr("Global parameter."));

    Highlight(text, m_global_parameter_names, format);

    format.setForeground(Qt::darkMagenta);
    format.setFontWeight(QFont::Bold);
    format.setToolTip(tr("Local parameter."));

    Highlight(text, m_local_parameter_names, format);

    setCurrentBlockState(0);

    int startIndex = 0;
    if (previousBlockState() != 1)
        startIndex = text.indexOf(commentStartExpression);

    while (startIndex >= 0) {
        QRegularExpressionMatch match = commentEndExpression.match(text, startIndex);
        int endIndex = match.capturedStart();
        int commentLength = 0;
        if (endIndex == -1) {
            setCurrentBlockState(1);
            commentLength = text.length() - startIndex;
        } else {
            commentLength = endIndex - startIndex
                + match.capturedLength();
        }
        setFormat(startIndex, commentLength, multiLineCommentFormat);
        startIndex = text.indexOf(commentStartExpression, startIndex + commentLength);
    }
}

// One grid cell — every simple field gets exactly this, wide fields get two. A free-flowing layout
// produced a ragged form whose fields were all differently sized; a fixed column count keeps them on
// a grid and gives the scroll area a deterministic height to work with. Claude Generated.
static constexpr int kCellWidth = 280;
static constexpr int kCellHeight = 130;
static constexpr int kColumns = 2;
static constexpr int kCellSpacing = 12;
static constexpr int kSpringRow = 1000; ///< far below any real row: absorbs the leftover height

namespace {
/*! \brief Configure a field grid: equal-width columns of one cell each and a trailing spring row, so
 * the fields keep their natural height instead of being smeared over the dialog. Claude Generated. */
void SetupFieldGrid(QGridLayout* grid)
{
    grid->setContentsMargins(0, 0, 0, 0);
    grid->setHorizontalSpacing(kCellSpacing);
    grid->setVerticalSpacing(kCellSpacing);
    for (int c = 0; c < kColumns; ++c) {
        grid->setColumnMinimumWidth(c, kCellWidth);
        grid->setColumnStretch(c, 1); // equal share of the surplus width -> equally wide fields
    }
    grid->setRowStretch(kSpringRow, 1);
}
}

PrepareBox::PrepareBox(const QJsonObject& object, Highlighter* highlighter, QWidget* parent)
    : QGroupBox(parent)
    , m_json(object)
{
    QVBoxLayout* layout = new QVBoxLayout;

    QLabel* description = new QLabel;
    description->setWordWrap(true);
    description->setText(object["description"].toString());
    // Normal weight + muted, so the bold group title stays the thing you read first.
    description->setStyleSheet(QStringLiteral("QLabel { font-weight: normal; color: palette(mid); }"));
    layout->addWidget(description);

    m_title = object["title"].toString();
    setTitle(m_title);
    m_type = object["type"].toInt();
    // 350x150 minimum for what is often a single spin box wasted enormous area — ten of those never
    // fit any dialog. Width is now a range and the height follows the content. Claude Generated.
    // One standard cell for every simple field; wide fields (equation, reactions) span two of them.
    // Uniform metrics keep the form on a grid instead of a ragged flow. Claude Generated.
    setMinimumWidth(kCellWidth);
    setMinimumHeight(kCellHeight);
    ApplyStyle();
    m_initial = object;
    m_name = object["name"].toString();
    if (m_name.isNull() || m_name.isEmpty())
        m_type = -1;
    if (m_type == 1) {
        m_spinbox = new QSpinBox;
        m_spinbox->setValue(object["value"].toInt());
        layout->addWidget(m_spinbox);
        connect(m_spinbox, &QSpinBox::valueChanged, this, [this, object]() {
            m_json["value"] = m_spinbox->value();
            emit this->changed();
        });
    } else if (m_type == 2) {
        m_doublespinbox = new QDoubleSpinBox;
        m_doublespinbox->setValue(object["value"].toDouble());
        layout->addWidget(m_doublespinbox);
        connect(m_doublespinbox, &QDoubleSpinBox::valueChanged, this, [this, object]() {
            m_json["value"] = m_doublespinbox->value();
            emit this->changed();
        });

    } else if (m_type == 3) {
        // Styled, validated string input (Claude Generated): clear button, placeholder, tooltip and
        // a monospace font so the "|"-separated parameter/name lists line up while editing.
        m_lineedit = new QLineEdit;
        m_lineedit->setText(object["value"].toString());
        m_lineedit->setClearButtonEnabled(true);
        m_lineedit->setToolTip(object["description"].toString());
        m_lineedit->setPlaceholderText(object.contains("placeholder")
                ? object["placeholder"].toString()
                : tr("name1 | name2 | …"));
        QFont mono = m_lineedit->font();
        mono.setStyleHint(QFont::Monospace);
        mono.setFamily(QStringLiteral("monospace"));
        m_lineedit->setFont(mono);
        m_lineedit->setStyleSheet(QStringLiteral(
            "QLineEdit { padding: 4px 6px; border: 1px solid palette(mid); border-radius: 4px; }"
            "QLineEdit:focus { border: 1px solid palette(highlight); }"));
        layout->addWidget(m_lineedit);
        m_highlight_match = object["value"].toString().split("|");
        connect(m_lineedit, &QLineEdit::textChanged, this, [this, object]() {
            m_json["value"] = m_lineedit->text();
            m_highlight_match = m_lineedit->text().split("|");
            emit this->changed();
        });
    } else if (m_type == 4) {
        m_textedit = new QTextEdit;
        highlighter->setDocument(m_textedit->document());
        QStringList execute;
        QJsonObject value = object["value"].toObject();
        // Line keys are numbers stored as strings and QJsonObject::keys() sorts LEXICOGRAPHICALLY
        // ("0","1","10",…,"2"), so an equation longer than ten lines came back scrambled. Sort
        // numerically; the write side below zero-pads so both orders agree. Claude Generated.
        QStringList valueKeys = value.keys();
        std::sort(valueKeys.begin(), valueKeys.end(), [](const QString& a, const QString& b) {
            bool okA = false, okB = false;
            const int ia = a.toInt(&okA);
            const int ib = b.toInt(&okB);
            return (okA && okB) ? ia < ib : a < b;
        });
        for (const QString& key : valueKeys)
            execute << value[key].toString();
        m_textedit->setText(execute.join("\n"));
        layout->addWidget(m_textedit);

        connect(m_textedit, &QTextEdit::textChanged, this, [this, object]() {
            QString script = m_textedit->document()->toPlainText();
            QStringList lines = script.split("\n");
            QJsonObject json;
            // Zero-padded keys so the lexicographic order of QJsonObject::keys() matches the line
            // order (older projects with unpadded keys are handled by the numeric sort). CG.
            for (int i = 0; i < lines.size(); ++i)
                json[QStringLiteral("%1").arg(i, 3, 10, QLatin1Char('0'))] = lines[i];
            m_json["value"] = json;
            emit this->changed();
        });
        // Was a HARD 700 px: in a FlowLayout such a box can neither wrap nor shrink, so as soon as
        // anything else claimed width (the preset column) the equation was pushed out of view
        // entirely. Flexible now — it still prefers to be wide, but it yields. Claude Generated.
        setMinimumWidth(2 * kCellWidth + 12); // two cells plus the grid spacing
        setMinimumHeight(2 * kCellHeight);
        setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    } else if (m_type == 6) {
        // Live-parsed reaction-equation editor (Claude Generated): the user types reaction equations
        // (arrow syntax) that ReactionParser turns into the N-component species list; the raw text is
        // stored verbatim and re-parsed in the model's DefineModel().
        m_reaction_editor = new ReactionEditorWidget(object["value"].toString(), this);
        layout->addWidget(m_reaction_editor);
        setMinimumWidth(2 * kCellWidth + 12); // spans two cells, like the equation
        setMinimumHeight(2 * kCellHeight);
        m_json["value"] = m_reaction_editor->reactionText();
        connect(m_reaction_editor, &ReactionEditorWidget::changed, this, [this](const QString& reactions) {
            m_json["value"] = reactions;
            emit this->changed();
        });
    }
    setLayout(layout);
}

void PrepareBox::setDerived(bool derived, const QString& reason)
{
    QWidget* editor = nullptr;
    switch (m_type) {
    case 1: editor = m_spinbox; break;
    case 2: editor = m_doublespinbox; break;
    case 3: editor = m_lineedit; break;
    case 4: editor = m_textedit; break;
    default: break;
    }
    if (!editor)
        return;
    editor->setEnabled(!derived);
    editor->setToolTip(derived ? reason : QString());
    setTitle(derived && !reason.isEmpty() ? tr("%1 (derived)").arg(m_title) : m_title);
}

void PrepareBox::ApplyStyle()
{
    // Bold, full-contrast header. palette(text) rather than a literal black so it stays readable
    // under a dark theme. A stylesheet font propagates to child widgets, hence the explicit reset —
    // otherwise every spin box and editor inside the group turns bold too. Claude Generated.
    setStyleSheet(QStringLiteral(
        "QGroupBox { font-weight: bold; color: %1; margin-top: 8px;"
        " border: %2; border-radius: 4px; }"
        "QGroupBox::title { subcontrol-origin: margin; left: 8px; padding: 0 4px; }"
        "QGroupBox QWidget { font-weight: normal; color: palette(text); }")
                      .arg(m_invalid ? QStringLiteral("#c62828") : QStringLiteral("palette(text)"),
                          m_invalid ? QStringLiteral("2px solid #c62828") : QStringLiteral("1px solid palette(mid)")));
}

void PrepareBox::setInvalid(bool invalid)
{
    if (m_invalid == invalid)
        return;
    m_invalid = invalid;
    ApplyStyle();
}

void PrepareBox::Clear()
{
    setValue(m_initial);
    setInvalid(false);
}

void PrepareBox::setValue(const QJsonObject& descriptor)
{
    if (!descriptor.contains("value"))
        return;
    const QJsonValue value = descriptor["value"];
    // Block signals so filling the form from a preset does not fire the live-update chain per field.
    switch (m_type) {
    case 1:
        if (m_spinbox) {
            QSignalBlocker b(m_spinbox);
            m_spinbox->setValue(value.toInt());
        }
        break;
    case 2:
        if (m_doublespinbox) {
            QSignalBlocker b(m_doublespinbox);
            m_doublespinbox->setValue(value.toDouble());
        }
        break;
    case 3:
        if (m_lineedit) {
            QSignalBlocker b(m_lineedit);
            m_lineedit->setText(value.toString());
            m_highlight_match = value.toString().split("|");
        }
        break;
    case 4:
        if (m_textedit) {
            QSignalBlocker b(m_textedit);
            QStringList lines;
            const QJsonObject obj = value.toObject();
            QStringList keys = obj.keys();
            std::sort(keys.begin(), keys.end(), [](const QString& a, const QString& b) {
                bool okA = false, okB = false;
                const int ia = a.toInt(&okA);
                const int ib = b.toInt(&okB);
                return (okA && okB) ? ia < ib : a < b;
            });
            for (const QString& k : keys)
                lines << obj[k].toString();
            m_textedit->setText(lines.join("\n"));
        }
        break;
    case 6:
        if (m_reaction_editor) {
            QSignalBlocker b(m_reaction_editor);
            m_reaction_editor->setReactionText(value.toString());
        }
        break;
    default:
        break;
    }
    m_json["value"] = value;
}

PrepareWidget::PrepareWidget(const QVector<QJsonObject>& objects, bool initial, QWidget* parent)
    : QWidget{ parent }
{
    // The fields sit in their own container so an optional preset list can go beside them.
    m_field_layout = new QGridLayout;
    SetupFieldGrid(m_field_layout);
    m_highlighter = new Highlighter;

    for (const QJsonObject& object : qAsConst(objects)) {
        if ((object.contains("once") && object.value("once").toBool(false) == true) && initial == false) {
            // A field that is not shown still governs the model, so keep the descriptor around.
            m_hidden_fields.insert(object["name"].toString(), object);
            continue;
        }
        if (object.isEmpty())
            continue;
        PrepareBox* box = new PrepareBox(object, m_highlighter, this);
        PlaceField(box, box->Span());
        connect(box, &PrepareBox::changed, this, &PrepareWidget::changed);
        m_stored_objects << box;
    }
    QWidget* fields = new QWidget;
    fields->setLayout(m_field_layout);

    // Only the field grid scrolls. The preset list and the preview chart are siblings in the splitter,
    // so they stay put while the fields move — scrolling the whole dialog dragged the presets and the
    // plot out of view along with them. Claude Generated.
    m_field_scroll = new QScrollArea;
    m_field_scroll->setWidgetResizable(true);
    m_field_scroll->setFrameShape(QFrame::NoFrame);
    m_field_scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    m_field_scroll->setWidget(fields);
    // Both grid columns plus room for the vertical scrollbar, so the splitter never opens with the
    // fields squeezed into a single column next to the presets.
    m_field_scroll->setMinimumWidth(kColumns * kCellWidth + kCellSpacing + 24);

    // The pane stacks a toolbar and (later) a status banner ABOVE the scrolling grid, so both stay
    // visible no matter how far the fields are scrolled. Claude Generated.
    QWidget* pane = new QWidget;
    m_pane_layout = new QVBoxLayout(pane);
    m_pane_layout->setContentsMargins(0, 0, 0, 0);
    m_pane_layout->setSpacing(6);

    QHBoxLayout* toolbar = new QHBoxLayout;
    toolbar->addStretch(1);
    QPushButton* clearButton = new QPushButton(tr("Clear all"));
    clearButton->setToolTip(tr("Reset every field to its default and leave equilibrium mode."));
    connect(clearButton, &QPushButton::clicked, this, &PrepareWidget::ClearAll);
    toolbar->addWidget(clearButton);
    m_pane_layout->addLayout(toolbar);
    m_pane_layout->addWidget(m_field_scroll, 1);

    m_splitter = new QSplitter(Qt::Horizontal);
    m_splitter->addWidget(pane);
    m_splitter->setStretchFactor(0, 1);
    m_splitter->setChildrenCollapsible(false);

    m_outer_layout = new QHBoxLayout;
    m_outer_layout->setContentsMargins(0, 0, 0, 0);
    m_outer_layout->addWidget(m_splitter);
    setLayout(m_outer_layout);
}

PrepareWidget::~PrepareWidget()
{
    // Destroying the Highlighter is NOT quiet: ~QSyntaxHighlighter calls setDocument(nullptr), which
    // touches the (still alive) document one last time and emits contentsChanged. That runs the
    // equation editor's textChanged handler -> PrepareBox::changed -> UpdateGuidance() ->
    // DeriveDefinition() -> back into the Highlighter, whose vtable has already been reset to the
    // abstract base: highlightBlock() is pure virtual there, so the process aborts. Cutting the signal
    // chain at its source before the delete is what makes the teardown safe. Claude Generated.
    m_destroying = true;
    for (PrepareBox* box : qAsConst(m_stored_objects)) {
        if (box)
            box->blockSignals(true);
    }
    delete m_highlighter;
}

void PrepareWidget::ClearFields()
{
    for (PrepareBox* box : qAsConst(m_stored_objects)) {
        if (box)
            box->Clear();
    }
}

void PrepareWidget::ClearAll()
{
    ClearFields();
    setEquilibriumMode(false); // also drops the derived-field marks and refreshes the guidance
    emit changed();
}

void PrepareWidget::SetStatus(const QString& message, int severity)
{
    if (!m_status)
        return;

    // 0 = ok (green), 1 = neutral (nothing to check yet), 2 = error. An error has to be impossible to
    // miss: it used to be one line at the bottom of a long guidance paragraph. Claude Generated.
    static const char* const background[] = { "#e8f5e9", "palette(alternate-base)", "#fdecea" };
    static const char* const border[] = { "#2e7d32", "palette(mid)", "#c62828" };
    static const char* const foreground[] = { "#1b5e20", "palette(text)", "#b71c1c" };
    const int s = qBound(0, severity, 2);

    m_status->setStyleSheet(QString("QLabel { background: %1; border: 1px solid %2; color: %3;"
                                    " border-radius: 4px; padding: 8px; font-weight: bold; }")
                                .arg(background[s], border[s], foreground[s]));
    m_status->setText(message);
}

void PrepareWidget::PlaceField(QWidget* widget, int span)
{
    if (!m_field_layout || !widget)
        return;

    span = qBound(1, span, kColumns);
    if (m_grid_col + span > kColumns) { // does not fit next to what is already on this row
        ++m_grid_row;
        m_grid_col = 0;
    }
    m_field_layout->addWidget(widget, m_grid_row, m_grid_col, 1, span);
    m_grid_col += span;
    if (m_grid_col >= kColumns) {
        ++m_grid_row;
        m_grid_col = 0;
    }
}

static constexpr int kCustomEquation = -1;
static constexpr int kCustomEquilibrium = -2;

void PrepareWidget::AddPresets(const QVector<QPair<QString, QVector<QJsonObject>>>& presets)
{
    if (presets.isEmpty() || !m_outer_layout)
        return;

    QListWidget* list = new QListWidget;
    // No hard width cap: preset names are long ("Binding 1:1/1:2 — cubic_root primitive") and were
    // elided at 210 px. Start wide enough for the longest entry and let the splitter be dragged.
    list->setWordWrap(true);
    list->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    list->setToolTip(tr("Click a preset to fill the fields. You can still edit everything afterwards."));
    QFontMetrics metrics(list->font());
    int widest = 0;
    auto addSection = [&](const QString& title) {
        QListWidgetItem* header = new QListWidgetItem(title, list);
        header->setFlags(Qt::NoItemFlags);
        QFont bold = header->font();
        bold.setBold(true);
        header->setFont(bold);
        header->setData(Qt::UserRole, -999);
    };
    auto addEntry = [&](const QString& text, int id) {
        QListWidgetItem* item = new QListWidgetItem(text, list);
        item->setData(Qt::UserRole, id);
        widest = qMax(widest, metrics.horizontalAdvance(text));
    };

    addSection(tr("Start from scratch"));
    addEntry(tr("Free equation"), kCustomEquation);
    addEntry(tr("Equilibrium (reactions)"), kCustomEquilibrium);
    addSection(tr("Predefined"));
    for (int i = 0; i < presets.size(); ++i)
        addEntry(presets[i].first, i);
    list->setMinimumWidth(qMin(260, widest + 48)); // narrow enough to leave the fields room
    connect(list, &QListWidget::itemClicked, this, [this, presets](QListWidgetItem* item) {
        const int index = item->data(Qt::UserRole).toInt();
        if (index == kCustomEquation || index == kCustomEquilibrium) {
            ClearFields(); // "start from scratch" means exactly that
            setEquilibriumMode(index == kCustomEquilibrium);
            emit changed();
            return;
        }
        if (index < 0 || index >= presets.size())
            return;

        // A preset only carries the fields it needs, so applying it on top of a filled form left the
        // remains of the previous one behind (stale parameter names, a reaction system that no longer
        // belongs). Every fill therefore starts from a blank form. Claude Generated.
        ClearFields();

        bool equilibrium = false;
        for (const QJsonObject& descriptor : presets[index].second) {
            const QString name = descriptor["name"].toString();
            if (name == QLatin1String("Reactions") && !descriptor["value"].toString().trimmed().isEmpty())
                equilibrium = true;
            for (PrepareBox* box : qAsConst(m_stored_objects)) {
                if (box && box->Name() == name)
                    box->setValue(descriptor);
            }
        }
        // The preset IS the choice of model kind — no separate mode switch to get wrong.
        setEquilibriumMode(equilibrium);
        emit changed(); // one update for the whole form, not one per field
    });
    m_splitter->insertWidget(0, list);
    m_splitter->setStretchFactor(0, 0);
    m_splitter->setStretchFactor(1, 1);
}

PrepareWidget::PrepareWidget(const QHash<QString, QJsonObject>& objects, bool initial, QWidget* parent)
    : QWidget{ parent }
{
    m_field_layout = new QGridLayout;
    SetupFieldGrid(m_field_layout);

    QStringList keys = objects.keys();
    std::sort(keys.begin(), keys.end());
    m_highlighter = new Highlighter;
    for (const QString& key : keys) {
        const QJsonObject& object = objects[key];
        if (key.contains("GlobalParameterNames")) {
            QStringList list = object["value"].toString().split("|");
            m_highlighter->setGlobalParameterNames(list);
        } else if (key.contains("LocalParameterNames")) {
            QStringList list = object["value"].toString().split("|");
            m_highlighter->setLocalParameterNames(list);
        }
        if ((object.contains("once") && object.value("once").toBool(false) == true) && initial == false) {
            // A field that is not shown still governs the model, so keep the descriptor around.
            m_hidden_fields.insert(object["name"].toString(), object);
            continue;
        }
        if (object.isEmpty())
            continue;
        PrepareBox* box = new PrepareBox(object, m_highlighter, this);
        auto list = box->HighlightMatch();

        if (box->Type() == -1) {
            delete box;
            continue;
        }
        PlaceField(box, box->Span());
        connect(box, &PrepareBox::changed, this, &PrepareWidget::changed);
        m_stored_objects << box;
    }
    m_highlighter->setMathFunctions(function_names);
    setLayout(m_field_layout);
}

QHash<QString, QJsonObject> PrepareWidget::getObject() const
{
    QHash<QString, QJsonObject> objects;
    for (const auto i : m_stored_objects) {
        auto element = i->getElement();
        objects.insert(element.first, element.second);
    }
    return objects;
}

void PrepareWidget::AddTableHeader(const QStringList& list)
{
    m_highlighter->setTableHeader(list);
}

void PrepareWidget::AddScriptGuidance()
{
    if (m_guidance || !m_field_layout)
        return;

    m_guidance = new QLabel;
    m_guidance->setWordWrap(true);
    m_guidance->setTextFormat(Qt::RichText);
    m_guidance->setStyleSheet(QStringLiteral(
        "QLabel { background: palette(alternate-base); border: 1px solid palette(mid);"
        " border-radius: 4px; padding: 8px; }"));
    m_guidance->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Minimum);
    PlaceField(m_guidance, kColumns); // full width — it is prose, not a field

    // The compiler / reaction-parser verdict goes into a banner PINNED above the scrolling grid.
    // Buried at the end of the guidance paragraph it was regularly scrolled out of sight, so a broken
    // equation looked like no equation at all. Claude Generated.
    m_status = new QLabel;
    m_status->setWordWrap(true);
    m_status->setTextFormat(Qt::RichText);
    m_status->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Minimum);
    if (m_pane_layout)
        m_pane_layout->insertWidget(1, m_status); // 0 is the toolbar row
    else
        PlaceField(m_status, kColumns); // model-definition tab: no pane, so it rides in the grid

    // Refresh whenever any field changes — the reaction system and the parameter counts both matter.
    for (PrepareBox* box : qAsConst(m_stored_objects)) {
        if (!box)
            continue;
        connect(box, &PrepareBox::changed, this, &PrepareWidget::UpdateGuidance);
        if (box->Name() == QLatin1String("Reactions")) {
            connect(box, &PrepareBox::changed, this, [this]() {
                if (m_equilibrium)
                    DeriveFromReactions();
            });
        }
    }
    UpdateGuidance();
}

void PrepareWidget::UpdateGuidance()
{
    if (m_destroying || !m_guidance)
        return;

    DeriveDefinition(); // names and counts follow the equation, so refresh them before reporting

    // Read through FieldValue, never off the visible boxes: half of these fields do not exist in the
    // model-definition tab. Claude Generated.
    const QString reactions = FieldValue(QStringLiteral("Reactions")).toString();
    const QString globalNames = FieldValue(QStringLiteral("GlobalParameterNames")).toString();
    const int inputSize = FieldValue(QStringLiteral("InputSize")).toInt();
    const int globalSize = FieldValue(QStringLiteral("GlobalParameterSize")).toInt();

    if (reactions.trimmed().isEmpty()) {
        m_guidance->setText(tr("<b>Free equation.</b> Just write it — the parameters are read out of it:"
                               "<br>· <tt>X1…X%1</tt> are the independent columns"
                               "<br>· every other name becomes a <b>global</b> parameter (%2)"
                               "<br>· list a name under <b>Names of local parameters</b> to make it "
                               "vary <b>per series</b> instead"
                               "<br>Helpers: <tt>cubic_root(a,b,c,d)</tt>, <tt>quadratic_root(a,b,c)</tt>."
                               "<br><br>%3")
                                .arg(qMax(1, inputSize))
                                .arg(globalNames.isEmpty() ? tr("none yet") : globalNames)
                                .arg(ValidateEquation()));
        return;
    }

    const ReactionSystem system = ReactionParser::Parse(reactions);
    if (!system.valid) {
        if (PrepareBox* box = boxNamed(QStringLiteral("Reactions")))
            box->setInvalid(true);
        SetStatus(tr("✘ The reaction system does not parse. One reaction per line, "
                     "e.g. <tt>A + B &lt;=&gt; AB</tt>."),
            2);
        m_guidance->setText(tr("<b>The reaction system is not valid yet.</b> One reaction per line, "
                               "e.g. <tt>A + B &lt;=&gt; AB</tt>."));
        return;
    }
    if (PrepareBox* box = boxNamed(QStringLiteral("Reactions")))
        box->setInvalid(false);

    const int nComp = system.components.size();
    const int nSpecies = system.species.size();

    QStringList freeList, concList, betaList;
    for (int c = 0; c < nComp; ++c)
        freeList << QString("<tt>spec_free(%1)</tt> = free %2").arg(c).arg(system.components[c]);
    for (int k = 0; k < nSpecies; ++k) {
        concList << QString("<tt>spec_conc(%1)</tt> = %2").arg(k).arg(system.species[k].label);
        betaList << QString("lg &beta;(%1)").arg(system.species[k].label);
    }

    QStringList totals;
    for (int c = 0; c < nComp; ++c)
        totals << QString("X%1").arg(c + 1);

    QString warning;
    if (inputSize != nComp)
        warning += tr("<br><b>⚠ Set \"Columns of Input\" to %1</b> — one independent column per "
                      "component, in the order %2.")
                       .arg(nComp).arg(system.components.join(", "));
    if (globalSize < nSpecies)
        warning += tr("<br><b>⚠ Set \"Number of global parameters\" to at least %1</b> — the first "
                      "%1 are the stability constants.")
                       .arg(nSpecies);

    m_guidance->setText(tr("<b>Equilibrium model — the equation may use:</b>"
                           "<br><tt>spec_solve(%1)</tt> — solve this data point first"
                           "<br>%2<br>%3"
                           "<br><br>The first %4 global parameter(s) are <b>%5</b> and drive the "
                           "solver; further globals and all locals are free for the signal equation.%6")
                            .arg(totals.join(", "))
                            .arg(freeList.join(" &nbsp;·&nbsp; "))
                            .arg(concList.join(" &nbsp;·&nbsp; "))
                            .arg(nSpecies)
                            .arg(betaList.join(", "))
                            .arg(warning + "<br><br>" + ValidateEquation()));
}

PrepareBox* PrepareWidget::boxNamed(const QString& name) const
{
    for (PrepareBox* box : qAsConst(m_stored_objects)) {
        if (box && box->Name() == name)
            return box;
    }
    return nullptr;
}

void PrepareWidget::setEquilibriumMode(bool equilibrium)
{
    m_equilibrium = equilibrium;

    // A free-form equation never needs a reaction system, and an equilibrium model never sets its
    // component count by hand — showing both at once is what made this dialog unreadable. CG.
    if (PrepareBox* reactions = boxNamed(QStringLiteral("Reactions")))
        reactions->setVisible(equilibrium);

    if (!equilibrium) {
        // Hiding the reaction editor is not enough: ScriptModel builds its speciation from the
        // reaction TEXT, so a populated-but-hidden field would silently keep driving an equilibrium
        // model the user cannot see — and make the mode flag disagree with the field. Clear it, so
        // "hidden" really means "not used". Claude Generated.
        if (PrepareBox* box = boxNamed(QStringLiteral("Reactions"))) {
            QJsonObject empty;
            empty["value"] = QString();
            box->setValue(empty);
        }
        if (PrepareBox* box = boxNamed(QStringLiteral("InputSize")))
            box->setDerived(false);
        if (PrepareBox* box = boxNamed(QStringLiteral("GlobalParameterNames")))
            box->setDerived(false);
    } else {
        DeriveFromReactions();
    }
    UpdateGuidance();
}

QString PrepareWidget::EquationText() const
{
    // Reassemble exactly as DefineModel() does (numeric line order, not lexicographic).
    const QJsonObject lines = FieldValue(QStringLiteral("Equation")).toObject();
    QStringList keys = lines.keys();
    std::sort(keys.begin(), keys.end(), [](const QString& a, const QString& b) {
        bool okA = false, okB = false;
        const int ia = a.toInt(&okA);
        const int ib = b.toInt(&okB);
        return (okA && okB) ? ia < ib : a < b;
    });
    QStringList text;
    for (const QString& k : keys)
        text << lines[k].toString();
    return text.join("\n").trimmed();
}

QJsonValue PrepareWidget::FieldValue(const QString& name) const
{
    if (PrepareBox* box = boxNamed(name))
        return box->getElement().second["value"];
    const auto it = m_hidden_fields.constFind(name);
    return it != m_hidden_fields.constEnd() ? it.value()["value"] : QJsonValue();
}

QStringList PrepareWidget::DeclaredLocals() const
{
    QStringList names
        = FieldValue(QStringLiteral("LocalParameterNames")).toString().split("|", Qt::SkipEmptyParts);
    for (QString& n : names)
        n = n.trimmed();
    names.removeAll(QString());
    return names;
}

void PrepareWidget::DeriveDefinition()
{
    // Derive the whole parameter declaration from what the user actually wrote: the reaction system
    // fixes the components and the stability constants, the equation names everything else. Declaring
    // names and counts by hand next to an equation that already contains them was pure duplication —
    // and every mismatch produced either a dead fit parameter or an unresolved symbol.
    //
    // The ONE thing an equation cannot express is which parameters vary per series, so
    // "Names of local parameters" stays the user's field; everything listed there is local, every
    // other symbol is global. Claude Generated.
    if (m_deriving || m_destroying)
        return;
    m_deriving = true;

    QStringList betaNames;
    QStringList components;
    if (m_equilibrium) {
        {
            const ReactionSystem system = ReactionParser::Parse(FieldValue(QStringLiteral("Reactions")).toString());
            if (system.valid) {
                components = system.components;
                for (const ReactionSpecies& species : system.species)
                    betaNames << QString("lgB_%1").arg(ReactionParser::AsciiSpeciesId(system.components, species.stoich));
            }
        }
    }

    const QStringList locals = DeclaredLocals();
    const QStringList symbols = ExprTkEngine::CollectSymbols(EquationText());

    int maxInput = 0;
    QStringList globals = betaNames;
    static const QRegularExpression inputRe(QStringLiteral("^[Xx](\\d+)$"));
    m_unused_locals = locals;
    for (const QString& symbol : symbols) {
        const QRegularExpressionMatch match = inputRe.match(symbol);
        if (match.hasMatch()) { // X1, X2, … are the independent columns, not parameters
            maxInput = qMax(maxInput, match.captured(1).toInt());
            continue;
        }
        if (locals.contains(symbol)) {
            m_unused_locals.removeAll(symbol);
            continue;
        }
        if (!globals.contains(symbol))
            globals << symbol;
    }

    // Equilibrium: one column per component. Free equation: as many as the highest X index used.
    const int inputs = m_equilibrium && !components.isEmpty()
        ? qMax(components.size(), maxInput)
        : qMax(1, maxInput);

    if (PrepareBox* box = boxNamed(QStringLiteral("InputSize"))) {
        QJsonObject v;
        v["value"] = inputs;
        box->setValue(v);
        box->setDerived(true, m_equilibrium && !components.isEmpty()
                ? tr("One independent column per component (%1).").arg(components.join(", "))
                : tr("Highest independent column referenced by the equation (X1…X%1).").arg(inputs));
    }

    if (PrepareBox* box = boxNamed(QStringLiteral("GlobalParameterNames"))) {
        QJsonObject v;
        v["value"] = globals.join("|");
        box->setValue(v);
        box->setDerived(true, betaNames.isEmpty()
                ? tr("Every symbol in the equation that is not an input and not listed as local.")
                : tr("The first %1 are the stability constants of the reaction system; the rest comes "
                     "from the equation.")
                      .arg(betaNames.size()));
    }

    if (PrepareBox* box = boxNamed(QStringLiteral("GlobalParameterSize"))) {
        QJsonObject v;
        v["value"] = globals.size();
        box->setValue(v);
        box->setDerived(true, tr("Follows from the derived global parameter names."));
    }

    if (PrepareBox* box = boxNamed(QStringLiteral("LocalParameterSize"))) {
        QJsonObject v;
        v["value"] = locals.size();
        box->setValue(v);
        box->setDerived(true, tr("Follows from the names you list as local parameters."));
    }

    // The editor can now colour parameters correctly while typing.
    if (m_highlighter) {
        m_highlighter->setGlobalParameterNames(globals);
        m_highlighter->setLocalParameterNames(locals);
    }

    m_deriving = false;
}

void PrepareWidget::DeriveFromReactions()
{
    DeriveDefinition();
}

QString PrepareWidget::ValidateEquation()
{
    PrepareBox* equationBox = boxNamed(QStringLiteral("Equation"));
    if (!equationBox)
        return QString();

    const QString equation = EquationText();
    if (equation.isEmpty()) {
        equationBox->setInvalid(false);
        SetStatus(tr("No equation yet — pick a preset on the left or start typing."), 1);
        return tr("<span style='color:palette(mid)'>No equation yet.</span>");
    }

    const int inputs = FieldValue(QStringLiteral("InputSize")).toInt();
    const QString globalNames = FieldValue(QStringLiteral("GlobalParameterNames")).toString();
    const QString localNames = FieldValue(QStringLiteral("LocalParameterNames")).toString();
    const QString reactions = FieldValue(QStringLiteral("Reactions")).toString();

    QStringList variables;
    for (int i = 0; i < qMax(1, inputs); ++i)
        variables << QString("X%1").arg(i + 1);
    variables << globalNames.split("|", Qt::SkipEmptyParts)
              << localNames.split("|", Qt::SkipEmptyParts);

    // Same engine the model uses, so the check is the real thing and not an approximation — and gated
    // on the same thing: ScriptModel keys the speciation purely off the reaction TEXT
    // (scriptmodel.cpp, m_has_speciation), not off the dialog's mode flag. Gating this on
    // m_equilibrium as well meant a stale flag made the validator report "Undefined symbol:
    // 'spec_solve'" for an equation the model compiles happily. Claude Generated.
    ExprTkEngine engine;
    SpeciationEngine speciation;
    const bool hasSpeciation = !reactions.trimmed().isEmpty() && speciation.setReactions(reactions);
    if (hasSpeciation)
        engine.setSpeciation(&speciation);

    if (engine.prepare(equation, variables)) {
        equationBox->setInvalid(false);
        // A local that no longer appears in the equation is a dead fit parameter — it costs a Jacobian
        // column and can never be determined. Worth saying out loud. Claude Generated.
        if (!m_unused_locals.isEmpty()) {
            SetStatus(tr("✔ The equation compiles — but the local parameter(s) <tt>%1</tt> do not "
                         "appear in it and would be fitted without any effect.")
                          .arg(m_unused_locals.join(", ").toHtmlEscaped()),
                1);
        } else {
            SetStatus(tr("✔ The equation compiles. %1 global, %2 local parameter(s) derived.")
                          .arg(globalNames.split("|", Qt::SkipEmptyParts).size())
                          .arg(localNames.split("|", Qt::SkipEmptyParts).size()),
                0);
        }
        return tr("<span style='color:#2e7d32'>✔ The equation compiles.</span>");
    }

    // The parser message is the single most useful thing in this dialog when something is wrong, so
    // it goes into the pinned banner and the offending field gets a red frame. Claude Generated.
    QString error = engine.lastError().toHtmlEscaped();

    // "Undefined symbol: 'spec_solve'" is a true but useless diagnosis: the solver hooks only exist
    // once a reaction system is defined. Say THAT instead. Claude Generated.
    if (!hasSpeciation && equation.contains(QLatin1String("spec_"))) {
        equationBox->setInvalid(true);
        SetStatus(reactions.trimmed().isEmpty()
                ? tr("✘ <tt>spec_solve</tt> / <tt>spec_free</tt> / <tt>spec_conc</tt> need a reaction "
                     "system. Pick an equilibrium preset, or enter the reactions above.")
                : tr("✘ The reaction system does not parse, so <tt>spec_solve</tt> and friends are not "
                     "available. One reaction per line, e.g. <tt>A + B &lt;=&gt; AB</tt>."),
            2);
        return tr("<span style='color:#c62828'>✘ %1</span>").arg(error);
    }

    equationBox->setInvalid(true);
    SetStatus(tr("✘ The equation does not compile:<br>%1").arg(error), 2);
    return tr("<span style='color:#c62828'>✘ %1</span>").arg(error);
}

void PrepareWidget::AddPreview(QWidget* preview)
{
    if (!preview || !m_splitter)
        return;
    preview->setMinimumWidth(320);
    m_splitter->addWidget(preview);
    m_splitter->setStretchFactor(m_splitter->count() - 1, 0);
}
