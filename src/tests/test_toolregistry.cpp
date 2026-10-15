/*
 * SupraFit - tool registry: schema validation, catalogue, invoke (headless, no net)
 * Copyright (C) 2016 - 2026 Conrad Hübler <Conrad.Huebler@gmx.net>
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

/* Phase 1 of roadmap/llm_agent_analysis.md: the registry is the one catalogue of
 * callable operations, and it must be correct before any tool exists. This test
 * pins the contract: well-formed schemas are accepted, malformed ones rejected at
 * registration time (a startup error, not a call-time surprise), unknown argument
 * keys are refused (a model that invents a parameter must be told, not silently
 * obeyed), and invoke() runs the handler with validated arguments.
 * Claude Generated (2026). */

#include <QtCore/QCoreApplication>
#include <QtTest/QtTest>

#include "src/agent/tool.h"
#include "src/agent/toolregistry.h"

class ToolRegistryTest : public QObject {
    Q_OBJECT

private:
    static QJsonObject objectSchema()
    {
        // The honoured subset: type object, properties with type/description/enum/
        // minimum/maximum, required.
        QJsonObject schema;
        schema.insert(QStringLiteral("type"), QStringLiteral("object"));
        QJsonObject props;
        QJsonObject name;
        name.insert(QStringLiteral("type"), QStringLiteral("string"));
        name.insert(QStringLiteral("description"), QStringLiteral("a name"));
        props.insert(QStringLiteral("name"), name);
        QJsonObject count;
        count.insert(QStringLiteral("type"), QStringLiteral("integer"));
        count.insert(QStringLiteral("minimum"), 1);
        count.insert(QStringLiteral("maximum"), 10);
        props.insert(QStringLiteral("count"), count);
        QJsonObject mode;
        mode.insert(QStringLiteral("type"), QStringLiteral("string"));
        mode.insert(QStringLiteral("enum"), QJsonArray{ QStringLiteral("a"), QStringLiteral("b") });
        props.insert(QStringLiteral("mode"), mode);
        schema.insert(QStringLiteral("properties"), props);
        schema.insert(QStringLiteral("required"), QJsonArray{ QStringLiteral("name") });
        return schema;
    }

    static ToolSpec makeSpec(const QString& name)
    {
        ToolSpec spec;
        spec.name = name;
        spec.description = QStringLiteral("test tool");
        spec.category = QStringLiteral("test");
        spec.paramSchema = objectSchema();
        spec.effect = ToolEffect::Read;
        spec.handler = [](const QJsonObject& args) {
            return ToolResult::success(QJsonObject{ { QStringLiteral("echo"), args } });
        };
        return spec;
    }

private slots:
    void registrationAcceptsWellFormed()
    {
        ToolRegistry registry;
        QString error;
        QVERIFY(registry.add(makeSpec(QStringLiteral("my_tool")), &error));
        QVERIFY(error.isEmpty());
        QCOMPARE(registry.size(), 1);
        QVERIFY(registry.contains(QStringLiteral("my_tool")));
        QCOMPARE(registry.names(), QStringList{ QStringLiteral("my_tool") });
    }

    void registrationRejectsEmptyName()
    {
        ToolRegistry registry;
        QString error;
        ToolSpec spec = makeSpec(QString());
        QVERIFY(!registry.add(spec, &error));
        QVERIFY(error.contains(QStringLiteral("empty")));
        QCOMPARE(registry.size(), 0);
    }

    void registrationRejectsDuplicateName()
    {
        ToolRegistry registry;
        QString error;
        QVERIFY(registry.add(makeSpec(QStringLiteral("dup")), &error));
        QVERIFY(!registry.add(makeSpec(QStringLiteral("dup")), &error));
        QVERIFY(error.contains(QStringLiteral("already registered")));
        QCOMPARE(registry.size(), 1);
    }

    void registrationRejectsMissingHandler()
    {
        ToolRegistry registry;
        QString error;
        ToolSpec spec = makeSpec(QStringLiteral("nohandler"));
        spec.handler = nullptr;
        QVERIFY(!registry.add(spec, &error));
        QVERIFY(error.contains(QStringLiteral("no handler")));
    }

    void registrationRejectsMalformedSchema()
    {
        ToolRegistry registry;
        QString error;

        // No type:object
        {
            ToolSpec spec = makeSpec(QStringLiteral("bad1"));
            spec.paramSchema = QJsonObject();
            QVERIFY(!registry.add(spec, &error));
            QVERIFY(error.contains(QStringLiteral("malformed schema")));
        }
        // Property without a valid type
        {
            ToolSpec spec = makeSpec(QStringLiteral("bad2"));
            QJsonObject schema;
            schema.insert(QStringLiteral("type"), QStringLiteral("object"));
            schema.insert(QStringLiteral("properties"),
                QJsonObject{ { QStringLiteral("x"), QJsonObject() } });
            spec.paramSchema = schema;
            QVERIFY(!registry.add(spec, &error));
        }
        // required names a property that is not declared
        {
            ToolSpec spec = makeSpec(QStringLiteral("bad3"));
            spec.paramSchema.insert(QStringLiteral("required"),
                QJsonArray{ QStringLiteral("ghost") });
            QVERIFY(!spec.paramSchema.value(QStringLiteral("properties")).toObject().contains(QStringLiteral("ghost")));
            QVERIFY(!registry.add(spec, &error));
        }
        // unsupported top-level key
        {
            ToolSpec spec = makeSpec(QStringLiteral("bad4"));
            spec.paramSchema.insert(QStringLiteral("additionalProperties"), false);
            QVERIFY(!registry.add(spec, &error));
            QVERIFY(error.contains(QStringLiteral("unsupported schema key")));
        }
        QCOMPARE(registry.size(), 0);
    }

    void validateRejectsMissingRequired()
    {
        ToolRegistry registry;
        QVERIFY(registry.add(makeSpec(QStringLiteral("t"))));
        const ToolValidation v = registry.validate(QStringLiteral("t"), QJsonObject());
        QVERIFY(!v.ok);
        QVERIFY(v.error.contains(QStringLiteral("name")));
    }

    void arrayItemsAreValidated()
    {
        ToolRegistry registry;
        ToolSpec spec = makeSpec(QStringLiteral("arr"));
        QJsonObject prop;
        prop.insert(QStringLiteral("type"), QStringLiteral("array"));
        prop.insert(QStringLiteral("items"), QJsonObject{ { QStringLiteral("type"), QStringLiteral("integer") } });
        spec.paramSchema = QJsonObject{ { QStringLiteral("type"), QStringLiteral("object") },
            { QStringLiteral("properties"), QJsonObject{ { QStringLiteral("ids"), prop } } } };
        QVERIFY(registry.add(spec));
        QVERIFY(registry.validate(QStringLiteral("arr"),
            QJsonObject{ { QStringLiteral("ids"), QJsonArray{ 1, 2 } } }).ok);
        const ToolValidation bad = registry.validate(QStringLiteral("arr"),
            QJsonObject{ { QStringLiteral("ids"), QJsonArray{ 1, QStringLiteral("x") } } });
        QVERIFY(!bad.ok);
        QVERIFY(bad.error.contains(QStringLiteral("array of integer")));
        // items on a non-array property is a malformed schema
        ToolSpec wrong = makeSpec(QStringLiteral("arr2"));
        wrong.paramSchema = QJsonObject{ { QStringLiteral("type"), QStringLiteral("object") },
            { QStringLiteral("properties"), QJsonObject{ { QStringLiteral("x"),
                QJsonObject{ { QStringLiteral("type"), QStringLiteral("string") },
                             { QStringLiteral("items"), QJsonObject{ { QStringLiteral("type"), QStringLiteral("string") } } } } } } } };
        QVERIFY(!registry.add(wrong));
    }

    void validateRejectsUnknownKey()
    {
        ToolRegistry registry;
        QVERIFY(registry.add(makeSpec(QStringLiteral("t"))));
        const ToolValidation v = registry.validate(QStringLiteral("t"),
            QJsonObject{ { QStringLiteral("name"), QStringLiteral("x") },
                         { QStringLiteral("invented"), 1 } });
        QVERIFY(!v.ok);
        QVERIFY(v.error.contains(QStringLiteral("unknown parameter")));
        QVERIFY(v.error.contains(QStringLiteral("invented")));
    }

    void validateRejectsWrongType()
    {
        ToolRegistry registry;
        QVERIFY(registry.add(makeSpec(QStringLiteral("t"))));
        const ToolValidation v = registry.validate(QStringLiteral("t"),
            QJsonObject{ { QStringLiteral("name"), 42 } });
        QVERIFY(!v.ok);
        QVERIFY(v.error.contains(QStringLiteral("string")));
    }

    void validateRejectsEnumViolation()
    {
        ToolRegistry registry;
        QVERIFY(registry.add(makeSpec(QStringLiteral("t"))));
        const ToolValidation v = registry.validate(QStringLiteral("t"),
            QJsonObject{ { QStringLiteral("name"), QStringLiteral("x") },
                         { QStringLiteral("mode"), QStringLiteral("c") } });
        QVERIFY(!v.ok);
        QVERIFY(v.error.contains(QStringLiteral("enum")));
    }

    void validateRejectsOutOfRange()
    {
        ToolRegistry registry;
        QVERIFY(registry.add(makeSpec(QStringLiteral("t"))));
        const ToolValidation tooBig = registry.validate(QStringLiteral("t"),
            QJsonObject{ { QStringLiteral("name"), QStringLiteral("x") },
                         { QStringLiteral("count"), 11 } });
        QVERIFY(!tooBig.ok);
        QVERIFY(tooBig.error.contains(QStringLiteral("maximum")));

        const ToolValidation tooSmall = registry.validate(QStringLiteral("t"),
            QJsonObject{ { QStringLiteral("name"), QStringLiteral("x") },
                         { QStringLiteral("count"), 0 } });
        QVERIFY(!tooSmall.ok);
        QVERIFY(tooSmall.error.contains(QStringLiteral("minimum")));
    }

    void validateAcceptsOptionalOmitted()
    {
        ToolRegistry registry;
        QVERIFY(registry.add(makeSpec(QStringLiteral("t"))));
        const ToolValidation v = registry.validate(QStringLiteral("t"),
            QJsonObject{ { QStringLiteral("name"), QStringLiteral("x") } });
        QVERIFY(v.ok);
    }

    void validateRejectsUnknownTool()
    {
        ToolRegistry registry;
        const ToolValidation v = registry.validate(QStringLiteral("ghost"), QJsonObject());
        QVERIFY(!v.ok);
        QVERIFY(v.error.contains(QStringLiteral("unknown tool")));
    }

    void invokeRunsHandlerWithValidatedArgs()
    {
        ToolRegistry registry;
        QVERIFY(registry.add(makeSpec(QStringLiteral("t"))));
        const ToolResult result = registry.invoke(QStringLiteral("t"),
            QJsonObject{ { QStringLiteral("name"), QStringLiteral("hello") },
                         { QStringLiteral("count"), 3 } });
        QVERIFY(result.ok);
        QCOMPARE(result.data.value(QStringLiteral("echo")).toObject().value(QStringLiteral("name")).toString(),
            QStringLiteral("hello"));
        QCOMPARE(result.data.value(QStringLiteral("echo")).toObject().value(QStringLiteral("count")).toInt(), 3);
    }

    void invokeReturnsFailureForBadArgs()
    {
        ToolRegistry registry;
        QVERIFY(registry.add(makeSpec(QStringLiteral("t"))));
        const ToolResult result = registry.invoke(QStringLiteral("t"), QJsonObject());
        QVERIFY(!result.ok);
        QVERIFY(result.error.contains(QStringLiteral("invalid arguments")));
    }

    void invokeReturnsFailureForUnknownTool()
    {
        ToolRegistry registry;
        const ToolResult result = registry.invoke(QStringLiteral("ghost"), QJsonObject());
        QVERIFY(!result.ok);
        QVERIFY(result.error.contains(QStringLiteral("unknown tool")));
    }

    void invokeHonoursAvailabilityPredicate()
    {
        ToolRegistry registry;
        ToolSpec spec = makeSpec(QStringLiteral("t"));
        spec.available = [] { return false; };
        QVERIFY(registry.add(spec));
        const ToolResult result = registry.invoke(QStringLiteral("t"),
            QJsonObject{ { QStringLiteral("name"), QStringLiteral("x") } });
        QVERIFY(!result.ok);
        QVERIFY(result.error.contains(QStringLiteral("not available")));
    }

    void catalogueBytesGrowsWithTools()
    {
        ToolRegistry registry;
        const int empty = registry.catalogueBytes();
        QVERIFY(registry.add(makeSpec(QStringLiteral("alpha"))));
        const int one = registry.catalogueBytes();
        QVERIFY(registry.add(makeSpec(QStringLiteral("beta"))));
        const int two = registry.catalogueBytes();
        QVERIFY(one > empty);
        QVERIFY(two > one);
    }

    void approvalRuleMatchesAutonomy()
    {
        // Read and Display never ask; Compute asks at Ask and InProgram;
        // FileWrite asks at Ask and InProgram but not at Full.
        QVERIFY(!needsApprovalAt(ToolEffect::Read, ToolAutonomy::Ask));
        QVERIFY(!needsApprovalAt(ToolEffect::Display, ToolAutonomy::Ask));
        QVERIFY(needsApprovalAt(ToolEffect::Compute, ToolAutonomy::Ask));
        QVERIFY(!needsApprovalAt(ToolEffect::Compute, ToolAutonomy::InProgram));
        QVERIFY(needsApprovalAt(ToolEffect::FileWrite, ToolAutonomy::InProgram));
        QVERIFY(!needsApprovalAt(ToolEffect::FileWrite, ToolAutonomy::Full));
        QVERIFY(!needsApprovalAt(ToolEffect::Process, ToolAutonomy::Full));
    }

    void clearRemovesAll()
    {
        ToolRegistry registry;
        QVERIFY(registry.add(makeSpec(QStringLiteral("a"))));
        QVERIFY(registry.add(makeSpec(QStringLiteral("b"))));
        QCOMPARE(registry.size(), 2);
        registry.clear();
        QCOMPARE(registry.size(), 0);
        QVERIFY(registry.names().isEmpty());
    }
};

QTEST_GUILESS_MAIN(ToolRegistryTest)
#include "test_toolregistry.moc"
