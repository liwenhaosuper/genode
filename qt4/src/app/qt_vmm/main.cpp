/*
 * \brief   Simple Qt interface for i.MX VMM
 * \author  Stefan Kalkowski
 * \date    2013-04-17
 */

/*
 * Copyright (C) 2013 Genode Labs GmbH
 *
 * This file is part of the Genode OS framework, which is distributed
 * under the terms of the GNU General Public License version 2.
 */

/* Qt includes */
#include <QApplication>

/* qt_avplay includes */
#include "main_window.h"

extern "C" void _sigprocmask() { }
extern "C" void sigprocmask() { }

static inline void load_stylesheet()
{
        QFile file(":style.qss");
        if (!file.open(QFile::ReadOnly)) {
                qWarning() << "Warning:" << file.errorString()
                           << "opening file" << file.fileName();
                return;
        }
        qApp->setStyleSheet(QLatin1String(file.readAll()));
}


int main(int argc, char *argv[])
{
	static QApplication app(argc, argv);

	load_stylesheet();

	static QMember<Main_window> main_window;
	main_window->move(400, 80);

	main_window->show();

	return app.exec();
}
