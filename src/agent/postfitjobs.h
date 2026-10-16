// postfitjobs.h - Job configurations for the statistical post-processing methods, GUI-free.
// Copyright (C) 2016 - 2026 Conrad Hübler <Conrad.Huebler@gmx.net>
//
// Claude Generated 2026 - The statistics dialog (ui/dialogs/statisticdialog.cpp) and the global
// search dialog (advancedsearch.cpp) assemble the JobManager jobs from widget values. This builds
// the same jobs from the dialogs' default values, so the agent can run weakened grid search,
// model comparison, reduction analysis and global search without a GUI.
#pragma once

#include <QJsonArray>
#include <QJsonObject>
#include <QSharedPointer>
#include <QString>

class AbstractModel;

struct PostfitJobOptions {
    int steps = 0;               ///< 0 = the agent default for the method
    double confidence = 95.0;    ///< confidence level in percent (grid search, model comparison)
    QJsonArray searchRanges;     ///< global search: "min;max;step" per global parameter (strings)
    int maxCalculations = 2000;  ///< global search: refuse grids larger than this
};

/// Build the job for @p method (SupraFit::Method id: 1 MC, 2 weakened grid search, 3 model comparison,
/// 4 cross validation, 5 reduction, 6 fast confidence, 7 global search) with the dialogs' defaults.
/// Returns an empty object and sets @p error when the job cannot be built (e.g. no fit result, too
/// many calculations). Claude Generated
QJsonObject buildPostfitJob(const QSharedPointer<AbstractModel>& model, int method,
    const PostfitJobOptions& options, QString* error);
