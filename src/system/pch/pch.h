// src/pch/pch.h
#pragma once

// ==== Standard C++ ====
#include <algorithm>
#include <array>
#include <bitset>
#include <chrono>
#include <fstream>
#include <functional>
#include <iomanip>
#include <iostream>
#include <map>
#include <memory>
#include <mutex>
#include <optional>
#include <random>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <thread>
#include <unordered_map>
#include <utility>
#include <variant>
#include <vector>

// ==== Qt Core / GUI ====
#include <QApplication>
#include <QByteArray>
#include <QComboBox>
#include <QDate>
#include <QDateTime>
#include <QDebug>
#include <QDialog>
#include <QDir>
#include <QFile>
#include <QFileDialog>
#include <QImage>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLabel>
#include <QLineEdit>
#include <QList>
#include <QMap>
#include <QMessageBox>
#include <QObject>
#include <QPixmap>
#include <QPushButton>
#include <QSettings>
#include <QString>
#include <QStringList>
#include <QTableWidget>
#include <QTextStream>
#include <QTimer>
#include <QToolButton>
#include <QVariant>
#include <QWidget>

// ==== Qt Modules (Optional je nach Bedarf) ====
#include <QSvgRenderer>
#include <QPrinter>
#include <QtCharts/QChartView>
#include <QtSerialPort/QSerialPort>
#include <QSqlDatabase>
#include <QSqlQuery>

// ==== Externe Libs ====
#include <openssl/evp.h>
#include <openssl/rand.h>
#include <openssl/aes.h>
#include <openssl/err.h>

#if defined(DB_BACKEND_MARIADB)
#include <mariadb/conncpp.hpp>
#else
#include <mysql/jdbc.h>
#endif

#ifdef _WIN32
#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <wincrypt.h>
#endif
