// Copyright (c) 2011-2016 The Cryptonote developers
// Copyright (c) 2015-2016 XDN developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "version.h"
#include "CryptoNoteConfig.h"

#include <QApplication>
#include <QCommandLineParser>
#include <QLocale>
#include <QLockFile>
#include <QMessageBox>
#include <QSplashScreen>
#include <QStyleFactory>

#include "CommandLineParser.h"
#include "CurrencyAdapter.h"
#include "LoggerAdapter.h"
#include "NodeAdapter.h"
#include "Settings.h"
#include "SignalHandler.h"
#include "WalletAdapter.h"

#include "gui/MainWindow.h"

#define DEBUG 1

using namespace WalletGui;

int main(int argc, char* argv[]) {
  QApplication app(argc, argv);
  app.setApplicationName(CryptoNote::CRYPTONOTE_APP);
  app.setApplicationVersion(Settings::instance().getVersion());
  app.setQuitOnLastWindowClosed(false);

#ifndef Q_OS_MAC
  QApplication::setStyle(QStyleFactory::create("Fusion"));
#endif

  CommandLineParser cmdLineParser(nullptr);
  Settings::instance().setCommandLineParser(&cmdLineParser);
  bool cmdLineParseResult = cmdLineParser.process(app.arguments());
  Settings::instance().load();

#ifdef Q_OS_WIN
  if(!cmdLineParseResult) {
    QMessageBox::critical(nullptr, QObject::tr("Error"), cmdLineParser.getErrorText());
    return app.exec();
  } else if (cmdLineParser.hasHelpOption()) {
    QMessageBox::information(nullptr, QObject::tr("Help"), cmdLineParser.getHelpText());
    return app.exec();
  }
#endif

  LoggerAdapter::instance().init();

  QString dataDirPath = Settings::instance().getDataDir().absolutePath();
  if (!QDir().exists(dataDirPath)) {
    QDir().mkpath(dataDirPath);
  }

  QLockFile lockFile(Settings::instance().getDataDir().absoluteFilePath(QApplication::applicationName() + ".lock"));
  if (!lockFile.tryLock()) {
    QMessageBox::warning(nullptr, QObject::tr("Fail"), QString("%1 wallet already running").arg(CurrencyAdapter::instance().getCurrencyDisplayName()));
    return 0;
  }

  QLocale::setDefault(QLocale::c());

  SignalHandler::instance().init();
  QObject::connect(&SignalHandler::instance(), &SignalHandler::quitSignal, &app, &QApplication::quit);

  QSplashScreen* splash = new QSplashScreen(QPixmap(":images/splash"), Qt::WindowStaysOnTopHint | Qt::X11BypassWindowManagerHint);
  if (!splash->isVisible()) {
    splash->show();
  }

//$$$$
	std::string msg01 = std::string(CryptoNote::CRYPTONOTE_NAME) + std::string(" Wallet [Core ") + std::string(PROJECT_VERSION_BUILD_NO) + std::string("] is loading blockchain...");
	//splash->showMessage(QObject::tr(msg01.c_str()), Qt::AlignLeft | Qt::AlignBottom, Qt::black);
	splash->showMessage(QObject::tr(msg01.c_str()), Qt::AlignHCenter | Qt::AlignBottom, Qt::black);
//$$$$

	app.processEvents();
	qRegisterMetaType<CryptoNote::TransactionId>("CryptoNote::TransactionId");
	qRegisterMetaType<quintptr>("quintptr");
	if (!NodeAdapter::instance().init()) {
		return 0;
	}

  splash->finish(&MainWindow::instance());
  MainWindow::instance().show();
  WalletAdapter::instance().open("");
  QObject::connect(QApplication::instance(), &QApplication::aboutToQuit, []() {
    MainWindow::instance().quit();
    if (WalletAdapter::instance().isOpen()) {
      WalletAdapter::instance().close();
    }

    NodeAdapter::instance().deinit();
  });

  return app.exec();
}
