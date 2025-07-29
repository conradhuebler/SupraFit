#pragma once

// SupraFit Pre-compiled Header (Claude Generated)
// Common headers used throughout the SupraFit project

// Qt6 Core
#include <QCoreApplication>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QIODevice>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QList>
#include <QMap>
#include <QObject>
#include <QPointer>
#include <QSharedPointer>
#include <QString>
#include <QStringList>
#include <QTextStream>
#include <QThread>
#include <QTimer>
#include <QUuid>
#include <QVariant>
#include <QVector>

// Qt6 Widgets (when building GUI components)
#ifdef QT_WIDGETS_LIB
#include <QApplication>
#include <QBoxLayout>
#include <QDialog>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>
#include <QWidget>
#endif

// Standard C++ Headers
#include <algorithm>
#include <array>
#include <atomic>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <functional>
#include <iostream>
#include <memory>
#include <mutex>
#include <random>
#include <sstream>
#include <string>
#include <thread>
#include <type_traits>
#include <utility>
#include <vector>

// Eigen (high-performance linear algebra)
#include <Eigen/Core>
#include <Eigen/Dense>

// fmt (modern C++ formatting)
#include <fmt/format.h>
#include <fmt/printf.h>

// ChaiScript (if available)
#ifdef USE_CHAISCRIPT
#include <chaiscript/chaiscript.hpp>
#endif