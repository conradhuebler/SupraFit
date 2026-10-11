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
#include <QtCore/QHash>
#include <QtCore/QJsonValue>
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
class ReactionEditorWidget;

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
    inline QString Name() const { return m_name; }
    /*! \brief Overwrite the box's value from a descriptor — used when a preset fills the dialog.
     * Updates both the widget and the stored json, without emitting changed(). Claude Generated. */
    void setValue(const QJsonObject& descriptor);
    /*! \brief Grey out a field that is DERIVED from something else (e.g. the component count follows
     * from the reaction system), so it cannot be set to a contradicting value. Claude Generated. */
    void setDerived(bool derived, const QString& reason = QString());
    inline int Type() const { return m_type; }
    /*! \brief Grid units this field occupies: 1 for a simple input, 2 for the equation and the
     * reaction editor, which need the full width. Keeps the form on a regular grid. CG. */
    inline int Span() const { return (m_type == 4 || m_type == 6) ? 2 : 1; }
    QStringList HighlightMatch() const { return m_highlight_match; }

    /*! \brief Reset the field to the value it was created with. Applying a preset on top of a filled
     * form silently mixed leftovers from the previous one, so every fill starts from here. CG. */
    void Clear();
    /*! \brief Mark the field as the source of a validation error — a red frame, so the message in the
     * status banner has a visible origin in the form. Claude Generated. */
    void setInvalid(bool invalid);

private:
    /*! \brief Re-apply the group styling; the error frame is part of it, so it has to be rebuilt
     * rather than appended (a second setStyleSheet would drop the base rules). Claude Generated. */
    void ApplyStyle();

    // Only ONE of these is created, decided by m_type — the others must be null, not uninitialised.
    // Every accessor tests them, so leaving them indeterminate was undefined behaviour waiting for a
    // type whose branch never runs (e.g. the removed species editor, type 5). Claude Generated.
    QLineEdit* m_lineedit = nullptr;
    QSpinBox* m_spinbox = nullptr;
    QDoubleSpinBox* m_doublespinbox = nullptr;
    QTextEdit* m_textedit = nullptr;
    ReactionEditorWidget* m_reaction_editor = nullptr;
    QJsonObject m_json;
    QJsonObject m_initial; ///< descriptor as constructed — the target of Clear()
    QString m_name;
    QString m_title;
    int m_type = -1;
    bool m_invalid = false;
    QStringList m_highlight_match;

signals:
    void changed();
};

class PrepareWidget : public QWidget {
    Q_OBJECT
public:
    explicit PrepareWidget(const QVector<QJsonObject>& objects, bool initial = true, QWidget* parent = nullptr);

    /*! \brief Show a click-to-fill preset list beside the fields, mirroring the reaction editor's UX:
     * a preset in the MENU opens the model directly, a preset in the DIALOG fills the form so it can
     * still be reviewed and edited. Each entry carries a full descriptor block. Claude Generated. */
    void AddPresets(const QVector<QPair<QString, QVector<QJsonObject>>>& presets);

    /*! \brief Show live guidance for scripted models: which identifiers the equation may use for the
     * reaction system currently in the Reactions field, which globals become lg beta, and how many
     * independent columns are required. The coupling is otherwise invisible. Claude Generated. */
    void AddScriptGuidance();

    /*! \brief Switch between the two kinds of scripted model. In equation mode the reaction editor is
     * hidden entirely; in equilibrium mode the component count and the stability-constant names are
     * derived from the reaction system and locked. Claude Generated. */
    /*! \brief Place a caller-supplied preview beside the fields. PrepareWidget stays free of any
     * model knowledge; the owner builds the preview and refreshes it on changed(). Claude Generated. */
    void AddPreview(QWidget* preview);

    void setEquilibriumMode(bool equilibrium);
    bool isEquilibriumMode() const { return m_equilibrium; }

    /*! \brief Reset every field to its default and leave equilibrium mode — the "start over" of the
     * definition dialog. Claude Generated. */
    void ClearAll();
    explicit PrepareWidget(const QHash<QString, QJsonObject>& objects, bool initial = true, QWidget* parent = nullptr);

    ~PrepareWidget();

    QHash<QString, QJsonObject> getObject() const;

    void AddTableHeader(const QStringList& list);
signals:
    void changed();

private:
    QVector<PrepareBox*> m_stored_objects;
    class QGridLayout* m_field_layout = nullptr;
    class QScrollArea* m_field_scroll = nullptr; ///< scrolls the fields only, not presets or preview
    class QHBoxLayout* m_outer_layout = nullptr;
    class QSplitter* m_splitter = nullptr;
    class QLabel* m_guidance = nullptr;
    class QLabel* m_status = nullptr; ///< always-visible banner: compiler / parser verdict
    class QVBoxLayout* m_pane_layout = nullptr; ///< toolbar + banner + scrolling field grid
    bool m_equilibrium = false;
    int m_grid_row = 0, m_grid_col = 0; ///< placement cursor of the semi-flexible field grid

    /*! \brief Reset all fields without touching the mode or emitting — the shared part of ClearAll()
     * and of filling the form from a preset. Claude Generated. */
    void ClearFields();
    /*! \brief Paint the status banner. @c severity 0 = ok, 1 = neutral, 2 = error. Claude Generated. */
    void SetStatus(const QString& message, int severity);

    /*! \brief Derive input count, global parameter names and both parameter counts from the reaction
     * system and the equation itself. Only "Names of local parameters" stays user-owned — an equation
     * cannot express which parameters vary per series. Claude Generated. */
    void DeriveDefinition();
    /*! \brief The equation as one string, in numeric line order (as DefineModel reads it). CG. */
    QString EquationText() const;
    /*! \brief Value of a definition field, whether or not the form SHOWS it.
     *
     * The model-definition tab is built with initial=false, which filters out every "once" descriptor
     * — Reactions, InputSize, both parameter-name lists. Reading those straight off the visible boxes
     * therefore returned nothing there, and the validator declared the model's own symbols undefined
     * (spec_solve, X2, every named parameter). Falls back to the descriptor the form was built with.
     * Claude Generated. */
    QJsonValue FieldValue(const QString& name) const;

    QHash<QString, QJsonObject> m_hidden_fields; ///< descriptors filtered out of the visible form
    /*! \brief Trimmed, non-empty names from the local-parameter field. Claude Generated. */
    QStringList DeclaredLocals() const;

    bool m_deriving = false; ///< re-entrance guard around the derivation
    bool m_destroying = false; ///< set in the destructor: no field may drive the form any more
    QStringList m_unused_locals; ///< locals declared but not referenced by the equation

    /*! \brief Place a field in the fixed-column grid, occupying @c span cells. A span-2 field always
     * starts a fresh row so the grid never breaks apart. Claude Generated. */
    void PlaceField(QWidget* widget, int span);
    PrepareBox* boxNamed(const QString& name) const;
    void DeriveFromReactions();
    QString ValidateEquation();
    void UpdateGuidance();
    QPointer<Highlighter> m_highlighter;
};
