#include "VersionExit.h"
#include <QMessageBox>
#include <QApplication>
#include <QSplashScreen>
#include <cstdlib>

namespace VersionExit
{
	void AbortApp(QWidget* parent, const QString& message)
	{
		// Hide splash if provided
		if (auto splash = qobject_cast<QSplashScreen*>(parent))
			splash->hide();

		// Show fatal message
		QMessageBox::critical(parent, QObject::tr("Version incompatible"), message);

		// Graceful Qt shutdown (works if event loop is running)
		QCoreApplication::exit(EXIT_FAILURE);

		// Drain pending events briefly
		QApplication::processEvents();

		// Hard exit (covers startup before exec())
		std::exit(EXIT_FAILURE);
	}
}