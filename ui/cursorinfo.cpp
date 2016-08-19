/*
 * Traceshark - a visualizer for visualizing ftrace and perf traces
 * Copyright (C) 2015, 2016  Viktor Rosendahl <viktor.rosendahl@gmail.com>
 *
 *   This program is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <QHBoxLayout>
#include <QIcon>
#include <QToolBar>
#include <QAction>
#include <QLineEdit>
#include <QString>
#include <QWidget>
#include <cmath>

#include "ui/cursorinfo.h"
#include "misc/resources.h"
#include "misc/traceshark.h"

#define RED_CURSOR_TOOLTIP "Move the red cursor to the specified time"
#define BLUE_CURSOR_TOOLTIP "Move the blue cursor to the specified time"

CursorInfo::CursorInfo(int nr, QWidget *parent):
	QWidget(parent), id(nr)
{
	QString text;
	QString qresource;
	QHBoxLayout *layout  = new QHBoxLayout(this);
	line = new QLineEdit(this);

	line->setReadOnly(false);
	line->setInputMask(QString("0000000.0000000"));

	switch (nr) {
	case TShark::RED_CURSOR:
		text = QString(tr(RED_CURSOR_TOOLTIP));
		qresource = QLatin1String(RESSRC_PNG_MOVE_BLUE);
		break;
	case TShark::BLUE_CURSOR:
		text = QString(tr(BLUE_CURSOR_TOOLTIP));
		qresource = QLatin1String(RESSRC_PNG_MOVE_RED);
		break;
	default:
		text = QString(tr("error in cursorinfo.cpp"));
		break;
	}

	layout->addWidget(line);

	moveCursorAction = new QAction(tr("Move"), this);
	moveCursorAction->setIcon(QIcon(qresource));
	/* Todo: come up with a shortcut below */
	/* moveCursorAction->setShortcuts(I_dont_know); */
	moveCursorAction->setToolTip(text);
	setTraceActionsEnabled(false);

	moveToolBar = new QToolBar(tr("Move Toolbar"), this);
	layout->addWidget(moveToolBar);
	moveToolBar->addAction(moveCursorAction);

	updateValue(0);
	tsconnect(moveCursorAction, triggered(), this, moveTriggered());
}

CursorInfo::~CursorInfo()
{
}

void CursorInfo::updateValue(double value)
{
	int precision = 7;
	double extra = 0;

	if (value >= 10)
		extra = floor (log(value) / log(10));

	precision += (int) extra;
	line->setText(QString::number((value), 'f', precision));
}

void CursorInfo::moveTriggered()
{
	if (line->hasAcceptableInput())
		emit valueChanged(line->text().toDouble(), id);
}

void CursorInfo::setTraceActionsEnabled(bool e)
{
	if (e == false)
		line->clear();
	line->setEnabled(e);
	moveCursorAction->setEnabled(e);
}
