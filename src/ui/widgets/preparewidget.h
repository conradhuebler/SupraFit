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

#pragma once

#include <QtCore/QJsonObject>
#include <QtCore/QPair>
#include <QtCore/QRegularExpression>
#include <QtCore/QVariant>

#include <QtGui/QSyntaxHighlighter>
#include <QtGui/QTextCharFormat>
#include <QtWidgets/QGroupBox>
#include <QtWidgets/QWidget>

class QDoubleSpinBox;
class QLineEdit;
class QSpinBox;
class QTextEdit;

const QStringList function_names = QStringList() << "cos"
                                                 << "sin"
                                                 << "tan"
                                                 << "acos"
                                                 << "asin"
                                                 << "atan"
                                                 << "cosh"
                                                 << "sinh"
                                                 << "tanh"
                                                 << "acosh"
                                                 << "asinh"
                                                 << "exp"
                                                 << "frexp"
                                                 << "ldexp"
                                                 << "log"
                                                 << "log10"
                                                 << "exp2"
                                                 << "expm1"
                                                 << "ilogb"
                                                 << "log1p"
                                                 << "log2"
                                                 << "logb"
                                                 << "scalnb"
                                                 << "scalbn"
                                                 << "pow"
                                                 << "sqrt"
                                                 << "cbrt"
                                                 << "hpot"
                                                 << "erf"
                                                 << "erc"
                                                 << "tgamma"
                                                 << "lgamma"
                                                 << "ceil"
                                                 << "floor"
                                                 << "fmod"
                                                 << "trunc"
                                                 << "round"
                                                 << "abs"
                                                 << "fabs"
                                                 << "fmin"
                                                 << "fmax"
                                                 << "fdim";

class Highlighter : public QSyntaxHighlighter {
    Q_OBJECT

public:
    Highlighter(QTextDocument* parent = nullptr);

    void addVariable(const QString& variable, int type);

    void setMathFunctions(const QStringList& math_functions)
    {
        m_math_functions = math_functions;
        rehighlight();
    }
    void setTableHeader(const QStringList& table_header)
    {
        m_table_header = table_header;
        rehighlight();
    }
    void setLocalParameterNames(const QStringList& names)
    {
        m_local_parameter_names = names;
        rehighlight();
    }
    void setGlobalParameterNames(const QStringList& names)
    {
        m_global_parameter_names = names;
        rehighlight();
    }

protected:
    void highlightBlock(const QString& text) override;

private:
    void Highlight(const QString& text, const QStringList& string, QTextCharFormat format);

    QStringList m_math_functions, m_table_header, m_local_parameter_names, m_global_parameter_names;

    QRegularExpression commentStartExpression;
    QRegularExpression commentEndExpression;

    QTextCharFormat singleLineCommentFormat;
    QTextCharFormat multiLineCommentFormat;
    QTextCharFormat quotationFormat;
};

class PrepareBox : public QGroupBox {
    Q_OBJECT
public:
    explicit PrepareBox(const QJsonObject& object, Highlighter* highlighter, QWidget* parent = NULL);

    inline QPair<QString, QJsonObject> getElement() const { return QPair<QString, QJsonObject>(m_name, m_json); }
    inline int Type() const { return m_type; }
    QStringList HighlightMatch() const { return m_highlight_match; }

private:
    QLineEdit* m_lineedit;
    QSpinBox* m_spinbox;
    QDoubleSpinBox* m_doublespinbox;
    QTextEdit* m_textedit;
    QJsonObject m_json;
    QString m_name;
    int m_type = -1;
    QStringList m_highlight_match;

signals:
    void changed();
};

class PrepareWidget : public QWidget {
    Q_OBJECT
public:
    explicit PrepareWidget(const QVector<QJsonObject>& objects, bool initial = true, QWidget* parent = nullptr);
    explicit PrepareWidget(const QHash<QString, QJsonObject>& objects, bool initial = true, QWidget* parent = nullptr);

    ~PrepareWidget()
    {
        if (m_highlighter)
            delete m_highlighter;
    }

    QHash<QString, QJsonObject> getObject() const;

    void AddTableHeader(const QStringList& list);
signals:
    void changed();

private:
    QVector<PrepareBox*> m_stored_objects;
    QPointer<Highlighter> m_highlighter;
};
