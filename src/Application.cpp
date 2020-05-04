/*
 * Copyright (C) 2019-2020 Ashar Khan <ashar786khan@gmail.com>
 *
 * This file is part of CP Editor.
 *
 * CP Editor is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * I will not be responsible if CP Editor behaves in unexpected way and
 * causes your ratings to go down and or lose any important contest.
 *
 * Believe Software is "Software" and it isn't immune to bugs.
 *
 */

#include "Application.hpp"
#include "Core/EventLogger.hpp"
#include "appwindow.hpp"
#include "mainwindow.hpp"
#include <QApplication>
#include <QCommandLineParser>
#include <QDir>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QProgressDialog>
#include <QTextStream>
#include <generated/version.hpp>
#include <singleapplication.h>

int Application::main(int argc, char *argv[])
{
    auto sApp = qobject_cast<SingleApplication *>(qApp);
    Q_ASSERT(sApp != nullptr);

    SingleApplication::setApplicationName("CP Editor");
    SingleApplication::setApplicationVersion(APP_VERSION);
    QApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);
    QApplication::setWindowIcon(QIcon(":/icon.png"));

    connect(this, SIGNAL(requestToExit()), qApp, SLOT(quit()), Qt::QueuedConnection);

    QTextStream cerr(stderr, QIODevice::WriteOnly);

    QString programName(argv[0]);

    QCommandLineParser parser;
    parser.addVersionOption();
    parser.addHelpOption();
    parser.setApplicationDescription(programName + " [-d/--depth <depth>] [options] [<path1> [<path2> [...]]]\n" +
                                     programName +
                                     " [-c/--contest] [options] <number of problems> <contest directory>");
    parser.addOptions(
        {{{"d", "depth"}, "Maximum depth when opening files in directories. No limit if not specified.", "depth", "-1"},
         {{"c", "contest"}, "Open a contest. i.e. Open files named A, B, ..., Z in a given directory."},
         /*{{"n", "new"},
          "Open a new window instead of open tabs in an existing window. This may cause error of the competitive "
          "companion server."},*/
         {"cpp", "Open C++ files in given directories. / Use C++ for open contests."},
         {"java", "Open Java files in given directories. / Use Java for open contests."},
         {"python", "Open Python files in given directories. / Use Python for open contests."},
         {"verbose", "Dump all logs to stderr of the application. (use only for debug purpose)"},
         {"no-hot-exit", "Do not load hot exit in this session. You won't be able to load the last session again."}});
    parser.setOptionsAfterPositionalArgumentsMode(QCommandLineParser::ParseAsOptions);
    parser.setSingleDashWordOptionMode(QCommandLineParser::ParseAsLongOptions);
    parser.process(*qApp);

#define GETSET(x) bool x = parser.isSet(#x)
    GETSET(cpp);
    GETSET(java);
    GETSET(python);
    GETSET(contest);
#undef GETSET
    bool noHotExit = parser.isSet("no-hot-exit");
    bool shouldDumpTostderr = parser.isSet("verbose");

    auto instance = sApp->instanceId();
    Core::Log::init(instance, shouldDumpTostderr);
    LOG_INFO(INFO_OF(instance));

    auto args = parser.positionalArguments();

    if (contest)
    {
        LOG_INFO("Using contest mode");
        if (args.length() != 2)
        {
            cerr << "Invalid Arguments\n\n"
                 << "See " + programName + " --help for more infomation.\n\n";
            LOG_ERR("Exiting because contest arguments are not satisfied");
            return 1;
        }
        LOG_INFO("Argument length is okay, Beginning parsing of arguments to int");

        bool ok = false;
        int number = args[0].toInt(&ok);

        if (!ok || number < 0 || number > 26)
        {
            swap(args[0], args[1]);
            LOG_WARN("Failed to parse : " << args[1]);
            LOG_WARN("Trying next (i.e) : " << args[0]);
            number = args[0].toInt(&ok);
        }

        if (!ok || number < 0 || number > 26)
        {
            cerr << "Number of problems should be an integer in 0~26.\n\n"
                 << "See " + programName + " --help for more infomation.\n\n";
            LOG_ERR("Bad arguments for contest: [" << args.join(", ") << "]");
            return 1;
        }

        auto path = args[1];
        if (QFileInfo(path).isRelative())
            path = QDir::current().filePath(path);

        LOG_INFO("Path extracted as : " << path);

        if (/*!parser.isSet("new") &&*/ sApp->isSecondary())
        {
            QJsonObject json;
            json["type"] = "contest";
#define TOJSON(x) json[#x] = x
            TOJSON(cpp);
            TOJSON(java);
            TOJSON(python);
            TOJSON(number);
            TOJSON(path);
#undef TOJSON
            if (sApp->sendMessage("AAAAAAAAAAAAAAAAAAAANOLOSTDATA" + QJsonDocument(json).toBinaryData()))
            {
                LOG_INFO("This is secondary application. Sending to primary instance the binary data : " +
                         QJsonDocument(json).toJson(QJsonDocument::Compact));
                cerr << "There is already a CP Editor running. New tabs are opened there.\n";
                return 0;
            }
        }

        LOG_INFO("Launching the new Appwindow with args: " << BOOL_INFO_OF(cpp) << BOOL_INFO_OF(java)
                                                           << BOOL_INFO_OF(python) << BOOL_INFO_OF(noHotExit)
                                                           << INFO_OF(number) << INFO_OF(path));

        AppWindow w(cpp, java, python, noHotExit, number, path);
        LOG_INFO("Launched window connecting this window to onReceiveMessage()");
        QObject::connect(sApp, &SingleApplication::receivedMessage, &w, &AppWindow::onReceivedMessage);
        LOG_INFO("Showing the application window and beginning the event loop");
        w.show();
        return qApp->exec();
    }
    else
    {
        LOG_INFO("Staarting in normal mode. Now parsing depth");
        bool ok = false;
        int depth = parser.value("depth").toInt(&ok);

        if (!ok || depth < -1)
        {
            LOG_ERR("Failed to use parse depth. Provided : " << parser.value("depth"));
            cerr << "Depth should be a non-negative integer.\n\n"
                 << "See " + programName + " --help for more infomation.\n\n";
            return 1;
        }

        if (!cpp && !java && !python)
            cpp = java = python = true;

        for (auto &path : args)
        {
            if (QFileInfo(path).isRelative())
                path = QDir::current().filePath(path);
            LOG_INFO("Path is : " << path);
        }

        if (/*!parser.isSet("new") &&*/ sApp->isSecondary())
        {
            QJsonObject json;
            json["type"] = "normal";
#define TOJSON(x) json[#x] = x
            TOJSON(depth);
            TOJSON(cpp);
            TOJSON(java);
            TOJSON(python);
#undef TOJSON
            json["paths"] = QJsonArray::fromStringList(args);
            if (sApp->sendMessage("AAAAAAAAAAAAAAAAAAAANOLOSTDATA" + QJsonDocument(json).toBinaryData()))
            {
                LOG_INFO("This is secondary application. Sending to primary instance the data : "
                         << QJsonDocument(json).toJson(QJsonDocument::Compact));
                cerr << "There is already a CP Editor running. New tabs are opened there.\n";
                return 0;
            }
        }
        LOG_INFO("Launching the new Appwindow with args: " << INFO_OF(depth) << BOOL_INFO_OF(cpp) << BOOL_INFO_OF(java)
                                                           << BOOL_INFO_OF(python) << BOOL_INFO_OF(noHotExit)
                                                           << INFO_OF(args.join(", ")));

        AppWindow w(depth, cpp, java, python, noHotExit, args);
        LOG_INFO("Launched window connecting this window to onReceiveMessage()");
        QObject::connect(sApp, &SingleApplication::receivedMessage, &w, &AppWindow::onReceivedMessage);
        LOG_INFO("Showing the application window and beginning the event loop");

        w.show();
        return qApp->exec();
    }
}

bool Application::handleSignal(int signal)
{
    if (qApp)
    {
        auto widgets = QApplication::topLevelWidgets();
        for (auto widget : widgets)
        {
            auto dialog = qobject_cast<QDialog *>(widget);
            if (dialog && dialog->isModal())
            {
                if (signal == SIG_INT)
                {
                    for (auto w : widgets)
                    {
                        auto appWindow = qobject_cast<AppWindow *>(w);
                        if (appWindow)
                        {
                            appWindow->showOnTop();
                            break;
                        }
                    }
                    dialog->setWindowState((dialog->windowState() & ~Qt::WindowMinimized) | Qt::WindowActive);
                    dialog->activateWindow();
                    dialog->raise();
                    return false;
                }
                else
                {
                    auto progressDialog = qobject_cast<QProgressDialog *>(dialog);
                    if (progressDialog)
                        progressDialog->cancel();
                    else
                        dialog->reject();
                }
            }
        }
        for (auto widget : widgets)
        {
            auto appWindow = qobject_cast<AppWindow *>(widget);
            if (appWindow)
            {
                if (signal == SIG_INT)
                    appWindow->close();
                else if (appWindow->forceClose())
                    emit requestToExit();
                break;
            }
        }
    }
    // Let the signal propagate as though we had not been there
    // It seems that it won't harm if we always return false
    // It's hard to determine whether the windows are really closed or not
    return false;
}