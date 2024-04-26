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

#include <QtWidgets/QDoubleSpinBox>
#include <QtWidgets/QGroupBox>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QSpinBox>
#include <QtWidgets/QTextEdit>
#include <QtWidgets/QWidget>

#include "src/ui/guitools/flowlayout.h"

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

PrepareBox::PrepareBox(const QJsonObject& object, Highlighter* highlighter, QWidget* parent)
    : QGroupBox(parent)
    , m_json(object)
{
    QVBoxLayout* layout = new QVBoxLayout;

    QLabel* description = new QLabel;
    description->setWordWrap(true);
    description->setText(object["description"].toString());
    layout->addWidget(description);

    setTitle(object["title"].toString());
    m_type = object["type"].toInt();
    setMaximumWidth(350);
    setMinimumWidth(350);
    setMinimumHeight(150);
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
        m_lineedit = new QLineEdit;
        m_lineedit->setText(object["value"].toString());
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
        for (const QString& key : value.keys())
            execute << value[key].toString();
        m_textedit->setText(execute.join("\n"));
        layout->addWidget(m_textedit);
        setMaximumWidth(350);
        setMinimumWidth(350);
        setMinimumHeight(150);
        connect(m_textedit, &QTextEdit::textChanged, this, [this, object]() {
            QString script = m_textedit->document()->toPlainText();
            QStringList lines = script.split("\n");
            QJsonObject json;
            for (int i = 0; i < lines.size(); ++i)
                json[QString::number(i)] = lines[i];
            m_json["value"] = json;
            emit this->changed();
        });
        setMaximumWidth(700);
        setMinimumWidth(700);
        setMinimumHeight(50);
    }
    setLayout(layout);
}

PrepareWidget::PrepareWidget(const QVector<QJsonObject>& objects, bool initial, QWidget* parent)
    : QWidget{ parent }
{
    FlowLayout* layout = new FlowLayout;
    m_highlighter = new Highlighter;

    for (const QJsonObject& object : qAsConst(objects)) {
        if ((object.contains("once") && object.value("once").toBool(false) == true) && initial == false)
            continue;
        if (object.isEmpty())
            continue;
        PrepareBox* box = new PrepareBox(object, m_highlighter, this);
        layout->addWidget(box);
        connect(box, &PrepareBox::changed, this, &PrepareWidget::changed);
        m_stored_objects << box;
    }
    setLayout(layout);
}

PrepareWidget::PrepareWidget(const QHash<QString, QJsonObject>& objects, bool initial, QWidget* parent)
    : QWidget{ parent }
{
    FlowLayout* layout = new FlowLayout;

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
        layout->addWidget(box);
        connect(box, &PrepareBox::changed, this, &PrepareWidget::changed);
        m_stored_objects << box;
    }
    m_highlighter->setMathFunctions(function_names);
    setLayout(layout);
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
