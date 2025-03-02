// SPDX-License-Identifier: (GPL-2.0-or-later OR BSD-2-Clause)
/*
 * Traceshark - a visualizer for visualizing ftrace and perf traces
 * Copyright (C) 2015-2023  Viktor Rosendahl <viktor.rosendahl@gmail.com>
 *
 * This file is dual licensed: you can use it either under the terms of
 * the GPL, or the BSD license, at your option.
 *
 *  a) This program is free software; you can redistribute it and/or
 *     modify it under the terms of the GNU General Public License as
 *     published by the Free Software Foundation; either version 2 of the
 *     License, or (at your option) any later version.
 *
 *     This program is distributed in the hope that it will be useful,
 *     but WITHOUT ANY WARRANTY; without even the implied warranty of
 *     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *     GNU General Public License for more details.
 *
 *     You should have received a copy of the GNU General Public
 *     License along with this library; if not, write to the Free
 *     Software Foundation, Inc., 51 Franklin St, Fifth Floor, Boston,
 *     MA 02110-1301 USA
 *
 * Alternatively,
 *
 *  b) Redistribution and use in source and binary forms, with or
 *     without modification, are permitted provided that the following
 *     conditions are met:
 *
 *     1. Redistributions of source code must retain the above
 *        copyright notice, this list of conditions and the following
 *        disclaimer.
 *     2. Redistributions in binary form must reproduce the above
 *        copyright notice, this list of conditions and the following
 *        disclaimer in the documentation and/or other materials
 *        provided with the distribution.
 *
 *     THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND
 *     CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES,
 *     INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 *     MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 *     DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
 *     CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 *     SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 *     NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 *     LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 *     HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 *     CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 *     OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 *     EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <cstdio>
#include <cstring>
#include <utility>

#include <QApplication>
#include <QColorDialog>
#include <QDateTime>
#include <QList>
#include <QScrollBar>
#include <QVBoxLayout>
#include <QToolBar>

#include "ui/cursor.h"
#include "ui/eventinfodialog.h"
#include "ui/eventswidget.h"
#include "analyzer/traceanalyzer.h"
#include "ui/errordialog.h"
#include "ui/graphenabledialog.h"
#include "ui/infowidget.h"
#include "ui/latencywidget.h"
#include "ui/licensedialog.h"
#include "ui/mainwindow.h"
#include "ui/migrationline.h"
#include "ui/regexdialog.h"
#include "ui/taskgraph.h"
#include "ui/taskrangeallocator.h"
#include "ui/taskselectdialog.h"
#include "ui/tasktoolbar.h"
#include "ui/eventselectdialog.h"
#include "ui/cpuselectdialog.h"
#include "parser/traceevent.h"
#include "ui/traceplot.h"
#include "ui/yaxisticker.h"
#include "misc/errors.h"
#include "misc/qtcompat.h"
#include "misc/resources.h"
#include "misc/setting.h"
#include "misc/settingstore.h"
#include "misc/statefile.h"
#include "misc/traceshark.h"
#include "threads/workqueue.h"
#include "threads/workitem.h"
#include "ui/qcustomplot.h"
#include "vtl/compiler.h"
#include "vtl/error.h"


#define TOOLTIP_OPEN			\
"Open a new trace file"

#define TOOLTIP_CLOSE			\
"Close the currently open tracefile"

#define TOOLTIP_SAVESCREEN		\
"Take a screenshot of the current graph and save it to a file"

#define CURSOR_ZOOM_TOOLTIP	        \
"Zoom to the time interval defined by the cursors"

#define DEFAULT_ZOOM_TOOLTIP	        \
"Zoom to the default zoom level"

#define FULL_ZOOM_TOOLTIP               \
"Zoom so that the whole trace is visible"

#define VERTICAL_ZOOM_TOOLTIP		\
"Toggle vertical zoom and scroll"

#define TOOLTIP_EXIT			\
"Exit traceshark"

#define TOOLTIP_SHOWTASKS		\
"Show a list of all tasks and it's possible to select one"

#define TOOLTIP_SHOWSCHEDLATENCIES		\
"Shows a list of scheduling latencies and it's possible to select one"

#define TOOLTIP_SHOWWAKELATENCIES		\
"Shows a list of wakeup latencies and it's possible to select one"

#define TOOLTIP_SHOWARGFILTER		\
"Show a dialog for filtering the info field with POSIX regular expressions"

#define TOOLTIP_CPUFILTER		\
"Select a subset of CPUs to filter on"

#define TOOLTIP_SHOWEVENTS		\
"Show a list of event types and it's possible to select which to filter on"

#define TOOLTIP_TIMEFILTER		\
"Filter on the time interval specified by the current position of the cursors"

#define TOOLTIP_GRAPHENABLE		\
"Select graphs or change settings"

#define TOOLTIP_RESETFILTERS		\
"Reset all filters"

#define TOOLTIP_RESETCOLORS		\
"Reset the task colors to the default colors"

#define TOOLTIP_EXPORTEVENTS		\
"Export the filtered events"

#define TOOLTIP_EXPORT_CPU		\
"Export cycles/cpu-cycles events"

#define TOOLTIP_GETSTATS		\
"Show the statistics dialog"

#define TOOLTIP_GETSTATS_TIMELIMITED	\
"Show the dialog with statistics that are time limited by the cursors"

#define TOOLTIP_FIND_SLEEP		\
"Find the next sched_switch event that puts the selected task to sleep"

#define FIND_WAKEUP_TOOLTIP		\
"Find the wakeup of the selected task that precedes the active cursor"

#define FIND_WAKING_TOOLTIP		\
"Find the waking event that precedes this wakeup event"

#define FIND_WAKING_DIRECT_TOOLTIP	\
"Find the waking event of the selected task that precedes the active cursor"

#define REMOVE_TASK_TOOLTIP		\
"Remove the unified graph for this task"

#define CLEAR_TASK_TOOLTIP		\
"Remove all the unified task graphs"

#define TASK_FILTER_TOOLTIP		\
"Filter on the selected task"

#define TASK_FILTER_TIMELIMIT_TOOLTIP	\
"Filter on the selected task and time limited by the cursors"

#define ADD_UNIFIED_TOOLTIP		\
"Add a unified graph for this task"

#define ADD_LEGEND_TOOLTIP		\
"Add this task to the legend"

#define COLOR_TASK_TOOLTIP		\
"Pick a new color for this task"

#define CLEAR_LEGEND_TOOLTIP		\
"Remove all tasks from the legend"

#define ABOUT_QT_TOOLTIP		\
"Show info about Qt"

#define ABOUT_TSHARK_TOOLTIP		\
"Show info about Traceshark"

#define SHOW_QCP_TOOLTIP		\
"Show info about QCustomPlot"

#define SHOW_LICENSE_TOOLTIP		\
"Show the license of Traceshark"

#define EVENT_BACKTRACE_TOOLTIP         \
"Show the backtrace of the selected event"

#define EVENT_CPU_TOOLTIP		\
"Filter the events view on the CPU of the selected event"

#define EVENT_PID_TOOLTIP		\
"Filter the events view on the PID of the selected event"

#define EVENT_TYPE_TOOLTIP		\
"Filter the events view on the type of the selected event"

#define EVENT_MOVEBLUE_TOOLTIP	        \
"Move the blue cursor to the time of the selected event"

#define EVENT_MOVERED_TOOLTIP		\
"Move the red cursor to the time of the selected event"

#define QCPRANGE_DIFF(A, B) \
	(TSABS(A.lower - B.lower) + TSABS(A.upper - B.upper))

const double MainWindow::bugWorkAroundOffset = 100;
const double MainWindow::schedSectionOffset = 100;
const double MainWindow::schedSpacing = 250;
const double MainWindow::schedHeight = 950;
const double MainWindow::cpuSectionOffset = 100;
const double MainWindow::cpuSpacing = 100;
const double MainWindow::cpuHeight = 800;
const double MainWindow::pixelZoomFactor = 33;
const double MainWindow::refDpiY = 96;
/*
 * const double migrateHeight doesn't exist. The value used is the
 * dynamically calculated inc variable in MainWindow::computeLayout()
 */

const double MainWindow::migrateSectionOffset = 250;

const QString MainWindow::RUNNING_NAME = tr("is runnable");
const QString MainWindow::PREEMPTED_NAME = tr("was preempted");
const QString MainWindow::UNINT_NAME = tr("uninterruptible");

const QString MainWindow::F_SEP = QString(";;");

const QString MainWindow::PNG_SUFFIX = QString(".png");
const QString MainWindow::BMP_SUFFIX = QString(".bmp");
const QString MainWindow::JPG_SUFFIX = QString(".jpg");
const QString MainWindow::PDF_SUFFIX = QString(".pdf");
const QString MainWindow::CSV_SUFFIX = QString(".csv");
const QString MainWindow::ASC_SUFFIX = QString(".asc");
const QString MainWindow::TXT_SUFFIX = QString(".txt");

const QString MainWindow::PNG_FILTER = QString("PNG (*.png)");
const QString MainWindow::BMP_FILTER = QString("BMP (*.bmp)");
const QString MainWindow::JPG_FILTER = QString ("JPEG (*.jpg)");
const QString MainWindow::PDF_FILTER = QString("PDF (*.pdf)");
const QString MainWindow::CSV_FILTER = QString("CSV (*.csv)");
const QString MainWindow::ASC_FILTER = QString("ASCII Text (*.asc)");
const QString MainWindow::TXT_FILTER = QString("ASCII Text (*.txt)");
const QString MainWindow::ASCTXT_FILTER = QString("ASCII Text (*.asc *.txt)");

const double MainWindow::RUNNING_SIZE = 8;
const double MainWindow::PREEMPTED_SIZE = 8;
const double MainWindow::UNINT_SIZE = 12;
const double MainWindow::CPUIDLE_SIZE = 5;

const QCPScatterStyle::ScatterShape MainWindow::RUNNING_SHAPE =
	QCPScatterStyle::ssTriangle;
const QCPScatterStyle::ScatterShape MainWindow::PREEMPTED_SHAPE =
	QCPScatterStyle::ssTriangle;
const QCPScatterStyle::ScatterShape MainWindow::UNINT_SHAPE =
	QCPScatterStyle::ssPlus;
const QCPScatterStyle::ScatterShape MainWindow::CPUIDLE_SHAPE =
	QCPScatterStyle::ssCircle;

const QColor MainWindow::RUNNING_COLOR = Qt::blue;
const QColor MainWindow::PREEMPTED_COLOR = Qt::red;
const QColor MainWindow::UNINT_COLOR = QColor(205, 0, 205);

MainWindow::MainWindow():
	tracePlot(nullptr), scrollBarUpdate(false), graphEnableDialog(nullptr),
	filterActive(false), foptions(QtCompat::ts_foptions)
{
	stateFile = new StateFile();

	createAboutBox();
	createAboutQCustomPlot();
	settingStore = new SettingStore();
	loadSettings();

	analyzer = new TraceAnalyzer(settingStore);

	infoWidget = new InfoWidget(this);
	infoWidget->setAllowedAreas(Qt::TopDockWidgetArea |
				    Qt::BottomDockWidgetArea);
	addDockWidget(Qt::TopDockWidgetArea, infoWidget);

	createActions();
	createToolBars();
	createMenus();
	createStatusBar();

	plotWidget = new QWidget(this);
	plotLayout = new QHBoxLayout(plotWidget);
	setCentralWidget(plotWidget);

	/* createTracePlot needs to have plotWidget created */
	createScrollBar();
	createTracePlot();
	plotConnections();
	tsconnect(scrollBar, valueChanged(int), this, scrollBarChanged(int));
	tsconnect(tracePlot->yAxis, rangeChanged(QCPRange), this,
		  yAxisChanged(QCPRange));
	tsconnect(tracePlot->yAxis,
		  selectionChanged (const QCPAxis::SelectableParts &),
		  this,
		  yAxisSelectionChange(const QCPAxis::SelectableParts &));

	eventsWidget = new EventsWidget(this);
	eventsWidget->setAllowedAreas(Qt::TopDockWidgetArea |
				      Qt::BottomDockWidgetArea);
	addDockWidget(Qt::BottomDockWidgetArea, eventsWidget);

	cursors[TShark::RED_CURSOR] = nullptr;
	cursors[TShark::BLUE_CURSOR] = nullptr;
	cursorPos[TShark::RED_CURSOR] = 0;
	cursorPos[TShark::BLUE_CURSOR] = 0;

	createDialogs();
	widgetConnections();
	dialogConnections();
}

void MainWindow::createTracePlot()
{
	QString mainLayerName = QString("main");
	QString cursorLayerName = QString("cursor");
	QCPLayer *mainLayer;
	yaxisTicker = new YAxisTicker();
	QSharedPointer<QCPAxisTicker> ticker((QCPAxisTicker*) (yaxisTicker));

	tracePlot = new TracePlot(plotWidget);
	setupOpenGL();

	tracePlot->yAxis->setTicker(std::move(ticker));
	tracePlot->yAxis->setSelectableParts(QCPAxis::spAxis);
	tracePlot->xAxis->setSelectableParts(QCPAxis::spNone);
	taskRangeAllocator = new TaskRangeAllocator(schedHeight
						    + schedSpacing);
	taskRangeAllocator->setStart(bugWorkAroundOffset);

	mainLayer = tracePlot->layer(mainLayerName);

	tracePlot->addLayer(cursorLayerName, mainLayer, QCustomPlot::limAbove);
	cursorLayer = tracePlot->layer(cursorLayerName);

	tracePlot->setCurrentLayer(mainLayerName);

	tracePlot->setAutoAddPlottableToLegend(false);
	tracePlot->hide();
	plotLayout->addWidget(tracePlot);

	tracePlot->setInteractions(QCP::iRangeDrag | QCP::iRangeZoom |
				   QCP::iSelectAxes | QCP::iSelectLegend |
				   QCP::iSelectPlottables);

	analyzer->setQCustomPlot(tracePlot);
}

void MainWindow::createScrollBar()
{
	scrollBar = new QScrollBar();
	scrollBar->setInvertedAppearance(true);
	scrollBar->setInvertedControls(false);
	scrollBar->setSingleStep(1);
	scrollBar->hide();
	plotLayout->addWidget(scrollBar);
}

void MainWindow::configureScrollBar()
{
	int pixels = plotWidget->height();
	double px_per_zrange;
	double diff_px;
	const QCPRange &zrange = tracePlot->yAxis->range();
	double high = TSMAX(top, zrange.upper);
	double low = TSMIN(bottom + zrange.size(), zrange.upper);
	double diff = TSABS(high - low);
	int smin, smax;
	int value;
	int pstep;
	bool visible = tracePlot->yAxis->range().upper < (top - 0.001) ||
		       tracePlot->yAxis->range().lower > (bottom + 0.001);

	if (visible) {
		px_per_zrange = pixels / zrange.size();
		diff_px = diff * px_per_zrange;
		smin = 0;
		smax = (int)(diff_px / 2.0) + 1;
		value = TSABS(zrange.upper - low) * smax / diff;
	} else {
		smin = 1;
		smax = 1;
		value = 1;
	}

	pstep = zrange.size() / diff * smax;

	scrollBarUpdate = true;
	if (scrollBar->minimum() != smin || scrollBar->maximum() != smax)
		scrollBar->setRange(smin, smax);
	if (scrollBar->value() != value)
		scrollBar->setValue(value);
	if (scrollBar->pageStep() != pstep)
		scrollBar->setPageStep(pstep);
	scrollBar->setVisible(visible);
	scrollBarUpdate = false;
}

MainWindow::~MainWindow()
{
	int i;

	if (analyzer->isOpen())
		closeTrace();
	delete analyzer;
	delete taskRangeAllocator;
	delete settingStore;

	vtl::set_error_handler(nullptr);

	for (i = 0; i < STATUS_NR; i++)
		delete statusStrings[i];

	delete stateFile;
}

void MainWindow::closeEvent(QCloseEvent *event)
{
	int wt;
	int ht;
	int ts_errno;

	/* Here is a great place to save settings, if we ever want to do it */
	taskSelectDialog->hide();
	eventSelectDialog->hide();
	cpuSelectDialog->hide();
	statsDialog->hide();
	statsLimitedDialog->hide();
	schedLatencyWidget->hide();
	wakeupLatencyWidget->hide();
	if (settingStore->getValue(Setting::SAVE_WINDOW_SIZE_EXIT).boolv()) {
		wt = width();
		ht = height();
		settingStore->setIntValue(Setting::MAINWINDOW_WIDTH, wt);
		settingStore->setIntValue(Setting::MAINWINDOW_HEIGHT, ht);
		ts_errno = settingStore->saveSettings();
		if (ts_errno != 0)
			vtl::warn(ts_errno, "Failed to save settings to %s",
				  TS_SETTING_FILENAME);
	}
	event->accept();
	/* event->ignore() could be used to refuse to close the window */
}

void MainWindow::openTrace()
{
	QString name;
	QString caption = tr("Open a trace file");

	name = QFileDialog::getOpenFileName(this, caption, QString(),
					    ASCTXT_FILTER, nullptr,
					    foptions);
	if (!name.isEmpty()) {
		openFile(name);
	}
}

void MainWindow::openFile(const QString &name)
{
	int ts_errno;

	if (analyzer->isOpen())
		closeTrace();
	ts_errno = loadTraceFile(name);

	if (ts_errno != 0) {
		vtl::warn(ts_errno, "Failed to open trace file %s",
			  name.toLocal8Bit().data());
		return;
	}

	if (analyzer->isOpen()) {
		quint64 start, process, layout, rescale, showt, eventsw;
		quint64 scursor, tshow;

		clearPlot();
		setupOpenGL();

		start = QDateTime::currentDateTimeUtc().toMSecsSinceEpoch();

		processTrace();
		process = QDateTime::currentDateTimeUtc().toMSecsSinceEpoch();

		computeLayout();
		layout = QDateTime::currentDateTimeUtc().toMSecsSinceEpoch();

		eventsWidget->beginResetModel();
		eventsWidget->setEvents(analyzer->events);
		if (analyzer->events->size() > 0)
			setEventActionsEnabled(true);
		setEventActionsEnabled(true);
		eventsWidget->endResetModel();

		taskSelectDialog->beginResetModel();
		taskSelectDialog->setTaskMap(&analyzer->taskMap,
					     analyzer->getNrCPUs());
		taskSelectDialog->endResetModel();

		eventSelectDialog->beginResetModel();
		eventSelectDialog->setStringTree(TraceEvent::getStringTree());
		eventSelectDialog->endResetModel();

		cpuSelectDialog->beginResetModel();
		cpuSelectDialog->setNrCPUs(analyzer->getNrCPUs());
		cpuSelectDialog->endResetModel();

		eventsw = QDateTime::currentDateTimeUtc().toMSecsSinceEpoch();

		setupCursors();
		scursor = QDateTime::currentDateTimeUtc().toMSecsSinceEpoch();

		rescaleTrace();
		rescale = QDateTime::currentDateTimeUtc().toMSecsSinceEpoch();

		computeStats();
		statsDialog->beginResetModel();
		statsDialog->setTaskMap(&analyzer->taskMap,
					analyzer->getNrCPUs());
		statsDialog->endResetModel();

		statsLimitedDialog->beginResetModel();
		statsLimitedDialog->setTaskMap(&analyzer->taskMap,
					       analyzer->getNrCPUs());
		statsLimitedDialog->endResetModel();

		schedLatencyWidget->setAnalyzer(analyzer);
		wakeupLatencyWidget->setAnalyzer(analyzer);

		showTrace();
		showt = QDateTime::currentDateTimeUtc().toMSecsSinceEpoch();

		tracePlot->show();
		tshow = QDateTime::currentDateTimeUtc().toMSecsSinceEpoch();

		setStatus(STATUS_FILE, &name);

		printf("processTrace() took %.6lf s\n"
		       "computeLayout() took %.6lf s\n"
		       "updating EventsWidget took %.6lf s\n"
		       "setupCursors() took %.6lf s\n"
		       "rescaleTrace() took %.6lf s\n"
		       "showTrace() took %.6lf s\n"
		       "tracePlot->show took %.6lf s\n",
		       (double) (process - start) / 1000,
		       (double) (layout - process) / 1000,
		       (double) (eventsw - layout) / 1000,
		       (double) (scursor - eventsw) / 1000,
		       (double) (rescale - scursor) / 1000,
		       (double) (showt - rescale) / 1000,
		       (double) (tshow - showt) / 1000);
		fflush(stdout);
		tracePlot->legend->setVisible(true);
		setCloseActionsEnabled(true);
		if (analyzer->events->size() <= 0)
			vtl::warnx("You have opened an empty trace!");
		else
			setTraceActionsEnabled(true);
	} else {
		setStatus(STATUS_ERROR);
		vtl::warnx("Unknown error when opening trace!");
	}
}

void MainWindow::resizeEvent(QResizeEvent */*event*/)
{
	if (!tracePlot->isVisible())
		return;

	QCPRange range = tracePlot->yAxis->range();
	double b;
	double maxsize = maxZoomVSize();

	if (range.size() > maxsize) {
		b = range.lower;
		tracePlot->yAxis->setRange(QCPRange(b, b + maxsize));
		tracePlot->replot();
	}
}

void MainWindow::processTrace()
{
	const QMap<int, QColor> &cmap = stateFile->getColorMap();
	bool usercolors;

	usercolors = analyzer->processTrace(cmap);
	startTime = analyzer->getStartTime().toDouble();
	endTime = analyzer->getEndTime().toDouble();
	if (usercolors)
		setResetTaskColorEnabled(true);
}

void MainWindow::computeLayout()
{
	unsigned int cpu;
	unsigned int nrCPUs;
	unsigned int offset;
	QString label;
	double inc, o, p;
	QColor color;

	bottom = bugWorkAroundOffset;
	offset = bottom;
	nrCPUs = analyzer->getNrCPUs();
	ticks.resize(0);
	tickLabels.resize(0);

	if (analyzer->enableMigrations()) {
		offset += migrateSectionOffset;

		analyzer->setMigrationOffset(offset);
		inc = nrCPUs * 315 + 67.5;
		analyzer->setMigrationScale(inc);

		/* add labels and lines here for the migration graph */
		color = QColor(135, 206, 250); /* Light sky blue */
		label = QString("fork/exit");
		ticks.append(offset);
		new MigrationLine(startTime, endTime, offset, color, tracePlot);
		tickLabels.append(label);
		o = offset;
		p = inc / nrCPUs ;
		for (cpu = 0; cpu < nrCPUs; cpu++) {
			o += p;
			label = QString("cpu") + QString::number(cpu);
			ticks.append(o);
			tickLabels.append(label);
			new MigrationLine(startTime, endTime, o, color,
					  tracePlot);
		}

		offset += inc;
		offset += p;
	}

	if (settingStore->getValue(Setting::SHOW_SCHED_GRAPHS).boolv()) {
		offset += schedSectionOffset;

		/* Set the offset and scale of the scheduling graphs */
		for (cpu = 0; cpu < nrCPUs; cpu++) {
			analyzer->setSchedOffset(cpu, offset);
			analyzer->setSchedScale(cpu, schedHeight);
			label = QString("cpu") + QString::number(cpu);
			ticks.append(offset);
			tickLabels.append(label);
			offset += schedHeight + schedSpacing;
		}
	}

	if (settingStore->getValue(Setting::SHOW_CPUFREQ_GRAPHS).boolv() ||
	    settingStore->getValue(Setting::SHOW_CPUIDLE_GRAPHS).boolv()) {
		offset += cpuSectionOffset;

		for (cpu = 0; cpu < nrCPUs; cpu++) {
			analyzer->setCpuFreqOffset(cpu, offset);
			analyzer->setCpuIdleOffset(cpu, offset);
			analyzer->setCpuFreqScale(cpu, cpuHeight);
			analyzer->setCpuIdleScale(cpu, cpuHeight);
			label = QString("cpu") + QString::number(cpu);
			ticks.append(offset);
			tickLabels.append(label);
			offset += cpuHeight + cpuSpacing;
		}
	}

	top = offset;
}

void MainWindow::rescaleTrace()
{
	int maxwakeup;
	const Setting::Value &maxvalue =
		settingStore->getValue(Setting::MAX_VRT_LATENCY);

	maxwakeup = maxvalue.intv();
	CPUTask::setVerticalDelayMAX(maxwakeup);
	analyzer->doScale();
}

void MainWindow::computeStats()
{
	analyzer->doStats();
}

void MainWindow::clearPlot()
{
	cursors[TShark::RED_CURSOR] = nullptr;
	cursors[TShark::BLUE_CURSOR] = nullptr;
	tracePlot->clearItems();
	tracePlot->clearPlottables();
	tracePlot->hide();
	scrollBar->hide();
	TaskGraph::clearMap();
	taskRangeAllocator->clearAll();
	infoWidget->setTime(0, TShark::RED_CURSOR);
	infoWidget->setTime(0, TShark::BLUE_CURSOR);
}

void MainWindow::showTrace()
{
	unsigned int cpu;
	int precision = 7;
	double extra = 0;

	if (endTime >= 10)
		extra = floor (log(endTime) / log(10));

	precision += (int) extra;

	tracePlot->yAxis->setRange(QCPRange(bottom, bottom + autoZoomVSize()));
	tracePlot->xAxis->setRange(QCPRange(startTime, endTime));
	tracePlot->xAxis->setNumberPrecision(precision);
	tracePlot->yAxis->setTicks(false);
	yaxisTicker->setTickVector(ticks);
	yaxisTicker->setTickVectorLabels(tickLabels);
	tracePlot->yAxis->setTicks(true);


	if (!settingStore->getValue(Setting::SHOW_CPUFREQ_GRAPHS).boolv() &&
	    !settingStore->getValue(Setting::SHOW_CPUIDLE_GRAPHS).boolv())
		goto skipIdleFreqGraphs;

	/* Show CPU frequency and idle graphs */
	for (cpu = 0; cpu <= analyzer->getMaxCPU(); cpu++) {
		QPen pen = QPen();
		QPen penF = QPen();

		QCPGraph *graph;
		QString name;
		QCPScatterStyle style;

		if (settingStore->getValue(Setting::SHOW_CPUIDLE_GRAPHS)
		    .boolv()) {
			const int lwidth = settingStore->getValue(
				Setting::IDLE_LINE_WIDTH).intv();
			const double adjsize = adjustScatterSize(CPUIDLE_SIZE,
								 lwidth);
			graph = tracePlot->addGraph(tracePlot->xAxis,
						    tracePlot->yAxis);
			graph->setSelectable(QCP::stNone);
			name = QString(tr("cpuidle")) + QString::number(cpu);
			style = QCPScatterStyle(CPUIDLE_SHAPE, adjsize);
			pen.setColor(Qt::red);
			pen.setWidth(lwidth);
			style.setPen(pen);
			graph->setScatterStyle(style);
			pen.setColor(Qt::green);
			graph->setPen(pen);
			graph->setName(name);
			graph->setAdaptiveSampling(true);
			graph->setLineStyle(QCPGraph::lsStepLeft);
			graph->setData(analyzer->cpuIdle[cpu].timev,
				       analyzer->cpuIdle[cpu].scaledData);
		}

		if (settingStore->getValue(Setting::SHOW_CPUFREQ_GRAPHS)
		    .boolv()) {
			graph = tracePlot->addGraph(tracePlot->xAxis,
						    tracePlot->yAxis);
			graph->setSelectable(QCP::stNone);
			name = QString(tr("cpufreq")) + QString::number(cpu);
			penF.setColor(Qt::blue);
			penF.setWidth(settingStore
				      ->getValue(Setting::FREQ_LINE_WIDTH)
				      .intv());
			graph->setPen(penF);
			graph->setName(name);
			graph->setAdaptiveSampling(true);
			graph->setLineStyle(QCPGraph::lsStepLeft);
			graph->setData(analyzer->cpuFreq[cpu].timev,
				       analyzer->cpuFreq[cpu].scaledData);
		}
	}

skipIdleFreqGraphs:

	/* Show scheduling graphs */
	for (cpu = 0; cpu <= analyzer->getMaxCPU(); cpu++) {
		DEFINE_CPUTASKMAP_ITERATOR(iter) = analyzer->
			cpuTaskMaps[cpu].begin();
		while(iter != analyzer->cpuTaskMaps[cpu].end()) {
			CPUTask &task = iter.value();
			iter++;

			addSchedGraph(task, cpu);
			if (settingStore->getValue(Setting::SHOW_SCHED_GRAPHS)
			    .boolv())
			{
				addHorizontalWakeupGraph(task);
				addWakeupGraph(task);
				addPreemptedGraph(task);
				addStillRunningGraph(task);
				addUninterruptibleGraph(task);
			}
		}
	}

	tracePlot->replot();
}

/*
 * The purpose of this function is to calculate how much the QCPScatterStyle
 * size should be increased, if we have a large line width.
 */
double MainWindow::adjustScatterSize(double default_size, int linewidth)
{
	if (linewidth == 1 || linewidth == 2)
		return default_size;

	return default_size * linewidth / 2;
}

double MainWindow::maxZoomVSize()
{
	double max = (double)(plotWidget->height() * pixelZoomFactor);

	max = refDpiY * max / logicalDpiY();
	return max;
}

double MainWindow::autoZoomVSize()
{
	double max = maxZoomVSize();
	double size = top - bottom;

	if (size < 0)
		size = -size;

	if (size > max)
		return max;

	return size;
}

void MainWindow::loadSettings()
{
	int ts_errno;
	int wt;
	int ht;
	QRect geometry;

	ts_errno = settingStore->loadSettings();
	if (ts_errno != 0) {
		vtl::warn(ts_errno, "Failed to load settings from %s",
			  TS_SETTING_FILENAME);
		return;
	}
	if (settingStore->getValue(Setting::LOAD_WINDOW_SIZE_START).boolv()) {
		wt = settingStore->getValue(
			Setting::MAINWINDOW_WIDTH).intv();
		ht = settingStore->getValue(
			Setting::MAINWINDOW_HEIGHT).intv();
	} else {
		geometry = QtCompat::availableGeometry();
		wt = geometry.width() - geometry.width() / 32;
		ht = geometry.height() - geometry.height() / 16;
		settingStore->setIntValue(Setting::MAINWINDOW_WIDTH, wt);
		settingStore->setIntValue(Setting::MAINWINDOW_HEIGHT, ht);
	}
	resize(wt, ht);
}

void MainWindow::setupCursors()
{
	double red, blue;

	red = (startTime + endTime) / 2;
	blue = (startTime + endTime) / 2 + (endTime - startTime) / 10;

	setupCursors(red, blue);
}

void MainWindow::setupCursors(const double &red, const double &blue)
{
	vtl::Time redtime = vtl::Time::fromDouble(red);
	redtime.setPrecision(analyzer->getTimePrecision());
	vtl::Time bluetime = vtl::Time::fromDouble(blue);
	bluetime.setPrecision(analyzer->getTimePrecision());

	setupCursors_(redtime, red, bluetime, blue);
}

void MainWindow::setupCursors(const vtl::Time &redtime,
			      const vtl::Time &bluetime)
{
	double red = redtime.toDouble();
	double blue = bluetime.toDouble();

	setupCursors_(redtime, red, bluetime, blue);
}

void MainWindow::setupCursors_(vtl::Time redtime, const double &red,
			       vtl::Time bluetime, const double &blue)
{
	cursors[TShark::RED_CURSOR] = new Cursor(tracePlot,
						 TShark::RED_CURSOR);
	cursors[TShark::BLUE_CURSOR] = new Cursor(tracePlot,
						  TShark::BLUE_CURSOR);

	cursors[TShark::RED_CURSOR]->setLayer(cursorLayer);
	cursors[TShark::BLUE_CURSOR]->setLayer(cursorLayer);

	cursors[TShark::RED_CURSOR]->setPosition(redtime);
	cursorPos[TShark::RED_CURSOR] = red;
	infoWidget->setTime(redtime, TShark::RED_CURSOR);

	cursors[TShark::BLUE_CURSOR]->setPosition(bluetime);
	cursorPos[TShark::BLUE_CURSOR] = blue;
	infoWidget->setTime(bluetime, TShark::BLUE_CURSOR);
	checkStatsTimeLimited();

	scrollTo(redtime);
}

void MainWindow::addSchedGraph(CPUTask &cpuTask, unsigned int cpu)
{
	/* Add scheduling graph */
	TaskGraph *graph = new TaskGraph(tracePlot, cpu,
					 TaskGraph::GRAPH_CPUGRAPH);
	QColor color = analyzer->getTaskColor(cpuTask.pid);
	Task *task = analyzer->findTask(cpuTask.pid);
	QPen pen = QPen();

	pen.setColor(color);
	pen.setWidth(settingStore->getValue(Setting::LINE_WIDTH).intv());
	graph->setPen(pen);
	graph->setTask(task);
	if (settingStore->getValue(Setting::SHOW_SCHED_GRAPHS).boolv())
		graph->setData(cpuTask.schedTimev, cpuTask.scaledSchedData);
	/*
	 * Save a pointer to the graph object in the task. The destructor of
	 * AbstractClass will delete this when it is destroyed.
	 */
	cpuTask.graph = graph;
}

void MainWindow::addHorizontalWakeupGraph(CPUTask &task)
{
	if (!settingStore->getValue(Setting::HORIZONTAL_LATENCY).boolv())
		return;

	/* Add wakeup graph on top of scheduling */
	QCPGraph *graph = tracePlot->addGraph(tracePlot->xAxis,
					      tracePlot->yAxis);
	QCPScatterStyle style = QCPScatterStyle(QCPScatterStyle::ssDot);
	QColor color = analyzer->getTaskColor(task.pid);
	QPen pen = QPen();
	QCPErrorBars *errorBars = new QCPErrorBars(tracePlot->xAxis,
						   tracePlot->yAxis);
	errorBars->setAntialiased(false);
	pen.setColor(color);
	pen.setWidth(settingStore->getValue(Setting::LINE_WIDTH).intv());
	style.setPen(pen);
	graph->setScatterStyle(style);
	graph->setLineStyle(QCPGraph::lsNone);
	graph->setAdaptiveSampling(true);
	graph->setData(task.delayTimev, task.delayHeight);
	errorBars->setData(task.delay, task.delayZero);
	errorBars->setErrorType(QCPErrorBars::etKeyError);
	errorBars->setPen(pen);
	errorBars->setWhiskerWidth(4);
	errorBars->setDataPlottable(graph);
	task.horizontalDelayBars = errorBars;
	/* errorBars->setSymbolGap(0); */
}

void MainWindow::addWakeupGraph(CPUTask &task)
{
	if (!settingStore->getValue(Setting::VERTICAL_LATENCY).boolv())
		return;

	/* Add wakeup graph on top of scheduling */
	QCPGraph *graph = tracePlot->addGraph(tracePlot->xAxis,
					      tracePlot->yAxis);
	QCPScatterStyle style = QCPScatterStyle(QCPScatterStyle::ssDot);
	QColor color = analyzer->getTaskColor(task.pid);
	QPen pen = QPen();
	QCPErrorBars *errorBars = new QCPErrorBars(tracePlot->xAxis,
						   tracePlot->yAxis);
	errorBars->setAntialiased(false);

	pen.setColor(color);
	pen.setWidth(settingStore->getValue(Setting::LINE_WIDTH).intv());
	style.setPen(pen);
	graph->setScatterStyle(style);
	graph->setLineStyle(QCPGraph::lsNone);
	graph->setAdaptiveSampling(true);
	graph->setData(task.delayTimev, task.delayHeight);
	errorBars->setData(task.delayZero, task.verticalDelay);
	errorBars->setErrorType(QCPErrorBars::etValueError);
	errorBars->setPen(pen);
	errorBars->setWhiskerWidth(4);
	errorBars->setDataPlottable(graph);
	task.verticalDelayBars = errorBars;
}

void MainWindow::addGenericAccessoryGraph(const QString &name,
					  const QVector<double> &timev,
					  const QVector<double> &scaledData,
					  QCPScatterStyle::ScatterShape sshape,
					  double size,
					  const QColor &color)
{
	if (timev.size() == 0)
		return;
	const int lwidth = settingStore->getValue(Setting::LINE_WIDTH).intv();
	const double adjsize = adjustScatterSize(size, lwidth);
	/* Add still running graph on top of the other two...*/
	QCPGraph *graph = tracePlot->addGraph(tracePlot->xAxis,
					      tracePlot->yAxis);
	graph->setName(name);
	QCPScatterStyle style = QCPScatterStyle(sshape, adjsize);
	QPen pen = QPen();

	pen.setColor(color);
	pen.setWidth(lwidth);
	style.setPen(pen);
	graph->setScatterStyle(style);
	graph->setLineStyle(QCPGraph::lsNone);
	graph->setAdaptiveSampling(true);
	graph->setData(timev, scaledData);
}

void MainWindow::addPreemptedGraph(CPUTask &task)
{
	addGenericAccessoryGraph(PREEMPTED_NAME, task.preemptedTimev,
				 task.scaledPreemptedData,
				 PREEMPTED_SHAPE, PREEMPTED_SIZE,
				 PREEMPTED_COLOR);
}

void MainWindow::addStillRunningGraph(CPUTask &task)
{
	addGenericAccessoryGraph(RUNNING_NAME, task.runningTimev,
				 task.scaledRunningData,
				 RUNNING_SHAPE, RUNNING_SIZE,
				 RUNNING_COLOR);
}

void MainWindow::addUninterruptibleGraph(CPUTask &task)
{
	addGenericAccessoryGraph(UNINT_NAME,
				 task.uninterruptibleTimev,
				 task.scaledUninterruptibleData,
				 UNINT_SHAPE, UNINT_SIZE,
				 UNINT_COLOR);
}

/*
 * These are actions that should be enabled whenever we have a non-empty
 * trace open
 */
void MainWindow::setTraceActionsEnabled(bool e)
{
	infoWidget->setTraceActionsEnabled(e);

	saveAction->setEnabled(e);
	exportEventsAction->setEnabled(e);
	exportCPUAction->setEnabled(e);
	cursorZoomAction->setEnabled(e);
	defaultZoomAction->setEnabled(e);
	fullZoomAction->setEnabled(e);
	verticalZoomAction->setEnabled(e);
	showTasksAction->setEnabled(e);
	filterCPUsAction->setEnabled(e);
	showEventsAction->setEnabled(e);
	showArgFilterAction->setEnabled(e);
	timeFilterAction->setEnabled(e);
	showStatsAction->setEnabled(e);
	showStatsTimeLimitedAction->setEnabled(e);
	showSchedLatencyAction->setEnabled(e);
	showWakeupLatencyAction->setEnabled(e);
}

void MainWindow::setLegendActionsEnabled(bool e)
{
	clearLegendAction->setEnabled(e);
}

/*
 * These are action that should be enabled whenever we have a trace open
 */
void MainWindow::setCloseActionsEnabled(bool e)
{
	closeAction->setEnabled(e);
}

/*
 * These are actions that should be enabled whenever a task is selected
 */
void MainWindow::setTaskActionsEnabled(bool e)
{
	colorTaskAction->setEnabled(e);
	findWakeupAction->setEnabled(e);
	findWakingDirectAction->setEnabled(e);
	findSleepAction->setEnabled(e);
	taskFilterAction->setEnabled(e);
	taskFilterLimitedAction->setEnabled(e);
}

void MainWindow::setAddToLegendActionEnabled(bool e)
{
	addToLegendAction->setEnabled(e);
}

void MainWindow::setWakeupActionsEnabled(bool e)
{
	findWakingAction->setEnabled(e);
}

void MainWindow::setAddTaskGraphActionEnabled(bool e)
{
	addTaskGraphAction->setEnabled(e);
}

void MainWindow::setTaskGraphRemovalActionEnabled(bool e)
{
	removeTaskGraphAction->setEnabled(e);
}

void MainWindow::setTaskGraphClearActionEnabled(bool e)
{
	clearTaskGraphsAction->setEnabled(e);
}

void MainWindow::setEventActionsEnabled(bool e)
{
	backTraceAction->setEnabled(e);
	moveBlueAction->setEnabled(e);
	moveRedAction->setEnabled(e);
	eventCPUAction->setEnabled(e);
	eventPIDAction->setEnabled(e);
	eventTypeAction->setEnabled(e);
}

void MainWindow::setResetTaskColorEnabled(bool e)
{
	resetTaskColorAction->setEnabled(e);
}

void MainWindow::closeTrace()
{
	quint64 startt, mresett, clearptt, acloset, disablet;
	int ts_errno = 0;

	ts_errno = stateFile->saveState();
	if (ts_errno != 0)
		vtl::warn(ts_errno, "Failed to save state file");
	stateFile->clear();

	startt = QDateTime::currentDateTimeUtc().toMSecsSinceEpoch();
	resetFilters();

	eventsWidget->beginResetModel();
	eventsWidget->clear();
	eventsWidget->endResetModel();
	eventsWidget->clearScrollTime();

	taskSelectDialog->beginResetModel();
	taskSelectDialog->setTaskMap(nullptr, 0);
	taskSelectDialog->endResetModel();

	statsDialog->beginResetModel();
	statsDialog->setTaskMap(nullptr, 0);
	statsDialog->endResetModel();

	statsLimitedDialog->beginResetModel();
	statsLimitedDialog->setTaskMap(nullptr, 0);
	statsLimitedDialog->endResetModel();

	eventSelectDialog->beginResetModel();
	eventSelectDialog->setStringTree(nullptr);
	eventSelectDialog->endResetModel();

	cpuSelectDialog->beginResetModel();
	cpuSelectDialog->setNrCPUs(0);
	cpuSelectDialog->endResetModel();

	schedLatencyWidget->clear();
	wakeupLatencyWidget->clear();

	mresett = QDateTime::currentDateTimeUtc().toMSecsSinceEpoch();

	clearPlot();

	clearptt = QDateTime::currentDateTimeUtc().toMSecsSinceEpoch();

	if(analyzer->isOpen()) {
		analyzer->close(&ts_errno);
	}

	acloset = QDateTime::currentDateTimeUtc().toMSecsSinceEpoch();

	taskToolBar->clear();
	setTraceActionsEnabled(false);
	setEventActionsEnabled(false);
	setLegendActionsEnabled(false);
	setCloseActionsEnabled(false);
	setTaskActionsEnabled(false);
	setWakeupActionsEnabled(false);
	setAddTaskGraphActionEnabled(false);
	setTaskGraphRemovalActionEnabled(false);
	setTaskGraphClearActionEnabled(false);
	setAddToLegendActionEnabled(false);
	setResetTaskColorEnabled(false);
	setStatus(STATUS_NOFILE);

	if (ts_errno != 0)
		vtl::warn(ts_errno, "Failed to close() trace file");

	disablet = QDateTime::currentDateTimeUtc().toMSecsSinceEpoch();

	if (( (disablet - startt) > 1000 )) {
		printf( "\n\n\n"
			"This is a diagnostic message generated because "
			"closing the trace took more than\n"
			"one second:\n"
			"MainWindow::closeTrace() took %.6lf s\n"
			"MainWindow::clearPlot() took %.6lf s\n"
			"analyzer->close() took %.6lf s\n",
			(double) (disablet - startt) / 1000,
			(double) (clearptt - mresett) / 1000,
			(double) (acloset - clearptt) / 1000);
		fflush(stdout);
	}
}

void MainWindow::saveScreenshot()
{
	QStringList fileNameList;
	QString fileName;
	QString pdfCreator = QString("traceshark ");
	QString pdfTitle;
	QString diagcapt;
	QString filter;
	QString selected;

	pdfCreator += QString(TRACESHARK_VERSION_STRING);

	if (!analyzer->isOpen())
		return;

	switch (analyzer->getTraceType()) {
	case TRACE_TYPE_FTRACE:
		pdfTitle = tr("Ftrace rendered by ");
		break;
	case TRACE_TYPE_PERF:
		pdfTitle = tr("Perf events rendered by ");
		break;
	default:
		pdfTitle = tr("Unknown garbage rendered by ");
		break;
	}

	pdfTitle += pdfCreator;
	filter = PNG_FILTER + F_SEP + BMP_FILTER + F_SEP + JPG_FILTER + F_SEP +
		PDF_FILTER;

	diagcapt = tr("Save screenshot to image");
	fileName = QFileDialog::getSaveFileName(this, diagcapt, QString(),
						filter, &selected, foptions);

	if (fileName.isEmpty())
		return;

	/*
	 * If the user has taken the trouble to type in a suffix that tells us
	 * what the expected format is, then we will use that, in spite of
	 * everything else. Otherwiise, we will go with the format selected by
	 * the QFileDialog::getSaveFileName() dialog.
	 */
	if (fileName.endsWith(PNG_SUFFIX))
		tracePlot->savePng(fileName);
	else if (fileName.endsWith(BMP_SUFFIX))
		tracePlot->saveBmp(fileName);
	else if (fileName.endsWith(JPG_SUFFIX))
		tracePlot->saveJpg(fileName);
	else if (fileName.endsWith(PDF_FILTER)) {
		tracePlot->savePdf(fileName, 0, 0,  QCP::epAllowCosmetic,
				   pdfCreator, pdfTitle);
	} else if (selected == PNG_FILTER) {
		TShark::checkSuffix(&fileName, PNG_SUFFIX);
		tracePlot->savePng(fileName);
	} else if (selected == BMP_FILTER) {
		TShark::checkSuffix(&fileName, BMP_SUFFIX);
		tracePlot->saveBmp(fileName);
	} else if (selected == JPG_FILTER) {
		TShark::checkSuffix(&fileName, JPG_SUFFIX);
		tracePlot->saveJpg(fileName);
	} else if (selected == PDF_FILTER) {
		TShark::checkSuffix(&fileName, PDF_SUFFIX);
		tracePlot->savePdf(fileName, 0, 0,  QCP::epAllowCosmetic,
				   pdfCreator, pdfTitle);
	} else {
		/*
		 * I believe that this should never happen but if it does,
		 * then we use PNG as default.
		 */
		TShark::checkSuffix(&fileName, PNG_SUFFIX);
		tracePlot->savePng(fileName);
	}
}

void MainWindow::cursorZoom()
{
	double min, max;

	/* Give up if both cursors are exactly on the same location */
	if (cursorPos[TShark::RED_CURSOR] == cursorPos[TShark::BLUE_CURSOR])
		return;

	min = TSMIN(cursorPos[TShark::RED_CURSOR],
		    cursorPos[TShark::BLUE_CURSOR]);
	max = TSMAX(cursorPos[TShark::RED_CURSOR],
		    cursorPos[TShark::BLUE_CURSOR]);

	tracePlot->xAxis->setRange(QCPRange(min, max));
	tracePlot->replot();
}

void MainWindow::fullZoom()
{
	tracePlot->yAxis->setRange(QCPRange(bottom, top));
	tracePlot->xAxis->setRange(QCPRange(startTime, endTime));
	tracePlot->replot();
}

void MainWindow::defaultZoom()
{
	tracePlot->yAxis->setRange(QCPRange(bottom, bottom + autoZoomVSize()));
	tracePlot->xAxis->setRange(QCPRange(startTime, endTime));
	tracePlot->replot();
}

void MainWindow::verticalZoom()
{
	bool actionChecked = verticalZoomAction->isChecked();
	bool axisSelected = tracePlot->yAxis->selectedParts().
		testFlag(QCPAxis::spAxis);

	if (actionChecked != axisSelected) {
		QCPAxis::SelectableParts selected =
			tracePlot->yAxis->selectedParts();
		if (actionChecked)
			selected |= QCPAxis::spAxis;
		else
			selected ^= QCPAxis::spAxis;
		tracePlot->yAxis->setSelectedParts(selected);
		tracePlot->replot();
	}
}

void MainWindow::createAboutBox()
{
	QString textAboutCaption;
	QString textAbout;

	textAboutCaption = QMessageBox::tr(
	       "<h1>About Traceshark</h1>"
	       "<p>This is version %1.</p>"
	       "<p>Built with " VTL_COMPILER " at " __DATE__ " " __TIME__
	       "</p>"
		).arg(QLatin1String(TRACESHARK_VERSION_STRING));
	textAbout = QMessageBox::tr(
	       "<p>Copyright &copy; 2014-2023 Viktor Rosendahl<p>"
	       "<p>This program comes with ABSOLUTELY NO WARRANTY; details below.</p>"
	       "<p>This is free software, and you are welcome to redistribute it"
	       " under certain conditions; select \"License\" under the \"Help\""
	       " menu for details.</p>"

	       "<h2>15. Disclaimer of Warranty.</h2>"
	       "<p>THERE IS NO WARRANTY FOR THE PROGRAM, TO THE EXTENT "
	       "PERMITTED BY APPLICABLE LAW.  EXCEPT WHEN OTHERWISE STATED IN "
	       "WRITING THE COPYRIGHT HOLDERS AND/OR OTHER PARTIES PROVIDE "
	       "THE PROGRAM \"AS IS\" WITHOUT WARRANTY OF ANY KIND, EITHER "
	       "EXPRESSED OR IMPLIED, INCLUDING, BUT NOT LIMITED TO, THE "
	       "IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A "
	       "PARTICULAR PURPOSE. THE ENTIRE RISK AS TO THE QUALITY AND "
	       "PERFORMANCE OF THE PROGRAM IS WITH YOU.  SHOULD THE PROGRAM "
	       "PROVE DEFECTIVE, YOU ASSUME THE COST OF ALL NECESSARY "
	       "SERVICING, REPAIR OR CORRECTION.</p>"

	       "<h2>16. Limitation of Liability.</h2>"
	       "<p>IN NO EVENT UNLESS REQUIRED BY APPLICABLE LAW OR AGREED "
	       "TO IN WRITING WILL ANY COPYRIGHT HOLDER, OR ANY OTHER PARTY "
	       "WHO MODIFIES AND/OR CONVEYS THE PROGRAM AS PERMITTED ABOVE, "
	       "BE LIABLE TO YOU FOR DAMAGES, INCLUDING ANY GENERAL, SPECIAL, "
	       "INCIDENTAL OR CONSEQUENTIAL DAMAGES ARISING OUT OF THE USE OR "
	       "INABILITY TO USE THE PROGRAM (INCLUDING BUT NOT LIMITED TO "
	       "LOSS OF DATA OR DATA BEING RENDERED INACCURATE OR LOSSES "
	       "SUSTAINED BY YOU OR THIRD PARTIES OR A FAILURE OF THE PROGRAM "
	       "TO OPERATE WITH ANY OTHER PROGRAMS), EVEN IF SUCH HOLDER OR "
	       "OTHER PARTY HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH "
	       "DAMAGES.</p>"

	       "<h2>17. Interpretation of Sections 15 and 16.</h2>"
	       "<p>If the disclaimer of warranty and limitation of "
	       "liability provided above cannot be given local legal effect "
	       "according to their terms, reviewing courts shall apply local "
	       "law that most closely approximates an absolute waiver of all "
	       "civil liability in connection with the Program, unless a "
	       "warranty or assumption of liability accompanies a copy of the "
	       "Program in return for a fee.</p>");
	aboutBox = new QMessageBox(this);
	aboutBox->setWindowTitle(tr("About Traceshark"));
	aboutBox->setText(textAboutCaption);
	aboutBox->setInformativeText(textAbout);

	QPixmap pm(QLatin1String(RESSRC_GPH_SHARK_PENG256));
	if (!pm.isNull())
		aboutBox->setIconPixmap(pm);
}

void MainWindow::about()
{
	aboutBox->show();
}

void MainWindow::createAboutQCustomPlot()
{
	QString textAboutCaption;
	QString textAbout;
	QString years = tr("2011-2022");

	if (!strcmp(QCUSTOMPLOT_VERSION_STR, "2.1.0"))
		years = tr("2011-2021");
	if (!strcmp(QCUSTOMPLOT_VERSION_STR, "2.0.1"))
		years = tr("2011-2018");
	else if (!strcmp(QCUSTOMPLOT_VERSION_STR, "2.0.0"))
		years = tr("2011-2017");

	textAboutCaption = QMessageBox::tr(
	       "<h1>About QCustomPlot</h1>"
#ifdef CONFIG_SYSTEM_QCUSTOMPLOT
	       "<p>This program uses QCustomPlot %1.</p>"
#else
	       "<p>This program contains a modified version of QCustomPlot %1.</p>"
#endif
		).arg(QLatin1String(QCUSTOMPLOT_VERSION_STR));
	textAbout = QMessageBox::tr(
	       "<p>Copyright &copy; %1 Emanuel Eichhammer"
	       "<p>QCustomPlot is licensed under GNU General Public License as "
	       "published by the Free Software Foundation, either version 3 of "
	       " the License, or (at your option) any later version.</p>"
	       "<p>See <a href=\"%2/\">%2</a> for more information about QCustomPlot.</p>"
	       "<p>This program comes with ABSOLUTELY NO WARRANTY; select \"License\" under the \"Help\""
	       " menu for details."
	       "<p>This is free software, and you are welcome to redistribute it"
	       " under certain conditions; see the license for details.").arg(
		       years).arg(QLatin1String("http://qcustomplot.com"));
	aboutQCPBox = new QMessageBox(this);
	aboutQCPBox->setWindowTitle(tr("About QCustomPlot"));
	aboutQCPBox->setText(textAboutCaption);
	aboutQCPBox->setInformativeText(textAbout);

	QPixmap pm(QLatin1String(RESSRC_GPH_QCP_LOGO));
	if (!pm.isNull())
		aboutQCPBox->setIconPixmap(pm);
}

void MainWindow::aboutQCustomPlot()
{
	aboutQCPBox->show();
}

void MainWindow::license()
{
	if (licenseDialog->isVisible())
		licenseDialog->hide();
	else
		licenseDialog->show();
}

void MainWindow::mouseWheel()
{
	bool ySelected = tracePlot->yAxis->selectedParts().
		testFlag(QCPAxis::spAxis);

	if (ySelected)
		tracePlot->axisRect()->setRangeZoom(Qt::Vertical);
	else
		tracePlot->axisRect()->setRangeZoom(Qt::Horizontal);
}

void MainWindow::mousePress()
{
	bool xSelected = tracePlot->xAxis->selectedParts().
		testFlag(QCPAxis::spAxis);
	bool ySelected = tracePlot->yAxis->selectedParts().
		testFlag(QCPAxis::spAxis);

	/* This is not possible but would be cool */
	if (xSelected && ySelected)
		tracePlot->axisRect()->setRangeDrag(Qt::Vertical |
						    Qt::Horizontal);
	else if (ySelected)
		tracePlot->axisRect()->setRangeDrag(Qt::Vertical);
	else
		tracePlot->axisRect()->setRangeDrag(Qt::Horizontal);
}

void MainWindow::yAxisSelectionChange(const QCPAxis::SelectableParts &parts)
{
	bool actionChecked = verticalZoomAction->isChecked();
	bool ySelected = parts.testFlag(QCPAxis::spAxis);
	QString text;
	/*
	 * We could also have used the following:
	 * bool ySelected = tracePlot->yAxis->selectedParts().
	 * testFlag(QCPAxis::spAxis);
	 */

	if (ySelected != actionChecked) {
		verticalZoomAction->setChecked(ySelected);
	}
}

void MainWindow::scrollBarChanged(int value)
{
	const QCPRange &zrange = tracePlot->yAxis->range();
	double high = TSMAX(top, zrange.upper);
	double low = TSMIN(bottom + zrange.size(), zrange.upper);
	double diff = TSABS(high - low);
	double quantum = 1.0L / scrollBar->maximum() * diff;
	QCPRange newrange;

	newrange.upper = value * quantum + low;
	newrange.lower = newrange.upper - zrange.size();
	tracePlot->yAxis->setRange(newrange);
	tracePlot->replot();
}

void MainWindow::yAxisChanged(QCPRange /*range*/)
{
	if (!scrollBarUpdate)
		configureScrollBar();
}

void MainWindow::plotDoubleClicked(QMouseEvent *event)
{
	int cursorIdx;
	QVariant details;
	QCPLayerable *clickedLayerable;
	QCPLegend *legend;
	QCPAbstractLegendItem *legendItem;

	/* Let's filter out double clicks on the legend or its items */
	clickedLayerable = tracePlot->getLayerableAt(event->pos(), false,
						     &details);
	if (clickedLayerable != nullptr) {
		legend = qobject_cast<QCPLegend*>(clickedLayerable);
		if (legend != nullptr)
			return;
		legendItem = qobject_cast<QCPAbstractLegendItem*>
			(clickedLayerable);
		if (legendItem != nullptr)
			return;
	}

	cursorIdx = infoWidget->getCursorIdx();
	if (cursorIdx != TShark::RED_CURSOR && cursorIdx != TShark::BLUE_CURSOR)
		return;

	Cursor *cursor = cursors[cursorIdx];
	if (cursor != nullptr) {
		double pixel = QtCompat::getPosXFromMouseEvent(event);
		double coord = tracePlot->xAxis->pixelToCoord(pixel);
		vtl::Time time = vtl::Time::fromDouble(coord);
		time.setPrecision(analyzer->getTimePrecision());
		cursorPos[cursorIdx] = coord;
		cursor->setPosition(time);
		checkStatsTimeLimited();
		eventsWidget->scrollTo(time);
		infoWidget->setTime(time, cursorIdx);
	}
}

void MainWindow::infoValueChanged(vtl::Time value, int nr)
{
	Cursor *cursor;
	double dblValue = value.toDouble();
	if (nr == TShark::RED_CURSOR || nr == TShark::BLUE_CURSOR) {
		cursor = cursors[nr];
		if (cursor != nullptr) {
			cursor->setPosition(value);
			checkStatsTimeLimited();
		}
		eventsWidget->scrollTo(value);
		cursorPos[nr] = dblValue;
	}
}

void MainWindow::moveActiveCursor(vtl::Time time)
{
	int cursorIdx;

	cursorIdx = infoWidget->getCursorIdx();
	moveCursor(time, cursorIdx);
}

void MainWindow::moveCursor(vtl::Time time, int cursorIdx)
{
	double dblTime = time.toDouble();

	if (cursorIdx != TShark::RED_CURSOR && cursorIdx != TShark::BLUE_CURSOR)
		return;

	Cursor *cursor = cursors[cursorIdx];
	if (cursor != nullptr) {
		cursor->setPosition(time);
		checkStatsTimeLimited();
		infoWidget->setTime(time, cursorIdx);
		cursorPos[cursorIdx] = dblTime;
	}
}

void MainWindow::handleEventDoubleClicked(EventsModel::column_t col,
					  const TraceEvent &event)
{
	switch (col) {
	case EventsModel::COLUMN_TIME:
		moveActiveCursor(event.time);
		break;
	case EventsModel::COLUMN_TASKNAME:
		/* Do nothing, not yet implemented */
		break;
	case EventsModel::COLUMN_PID:
		createEventPIDFilter(event);
		break;
	case EventsModel::COLUMN_CPU:
		createEventCPUFilter(event);
		break;
	case EventsModel::COLUMN_TYPE:
		createEventTypeFilter(event);
		break;
	case EventsModel::COLUMN_INFO:
		eventInfoDialog->show(event, *analyzer->getTraceFile());
		break;
	default:
		/* This should not happen ? */
		break;
	}
}

void MainWindow::taskTriggered(int pid)
{
	selectTaskByPid(pid, nullptr, PR_TRY_TASKGRAPH);
}

void MainWindow::showLatency(const Latency *latency)
{
	int activeIdx = infoWidget->getCursorIdx();
	int inactiveIdx;
	unsigned int lcpu;
	int lpid;

	inactiveIdx = TShark::RED_CURSOR;
	if (activeIdx == inactiveIdx)
		inactiveIdx = TShark::BLUE_CURSOR;

	Cursor *activeCursor = cursors[activeIdx];
	Cursor *inactiveCursor = cursors[inactiveIdx];

	const TraceEvent &schedevent = analyzer->events->at(latency->sched_idx);
	const TraceEvent &wakeupevent =
		analyzer->events->at(latency->runnable_idx);

	/*
	 * This is what we do, we move the *active* cursor to the wakeup
	 * event, move the *inactive* cursor to the scheduling event and then
	 * finally scroll the events widget to the same time and highlight
	 * the task that was doing the wakeup. This way we can push the button
	 * again to see who woke up the task that was doing the wakeup
	 */
	activeCursor->setPosition(wakeupevent.time);
	inactiveCursor->setPosition(schedevent.time);
	checkStatsTimeLimited();
	infoWidget->setTime(wakeupevent.time, activeIdx);
	infoWidget->setTime(schedevent.time, inactiveIdx);
	cursorPos[activeIdx] = wakeupevent.time.toDouble();
	cursorPos[inactiveIdx] = schedevent.time.toDouble();

	if (!analyzer->isFiltered()) {
		eventsWidget->scrollTo(latency->runnable_idx);
	} else {
		/*
		 * If a filter is enabled we need to try to find the index in
		 * analyzer->filteredEvents
		 */
		int filterIndex;
		if (analyzer->findFilteredEvent(latency->runnable_idx,
						&filterIndex)
		    != nullptr)
			eventsWidget->scrollTo(filterIndex);
	}

	lcpu = schedevent.cpu;
	lpid = latency->pid;

	selectTaskByPid(lpid, &lcpu, PR_TRY_TASKGRAPH);
}

void MainWindow::handleEventSelected(const TraceEvent *event)
{
	if (event == nullptr) {
		handleWakeUpChanged(false);
		handleEventChanged(false);
		return;
	} else {
		handleEventChanged(true);
	}

	if (event->type == SCHED_WAKEUP || event->type == SCHED_WAKEUP_NEW) {
		handleWakeUpChanged(true);
	} else {
		handleWakeUpChanged(false);
	}
}

void MainWindow::handleWakeUpChanged(bool selected)
{
	setWakeupActionsEnabled(selected);
}

void MainWindow::handleEventChanged(bool selected)
{
	setEventActionsEnabled(selected);
}

void MainWindow::createActions()
{
	openAction = new QAction(tr("&Open..."), this);
	openAction->setIcon(QIcon(RESSRC_GPH_OPEN));
	openAction->setShortcuts(QKeySequence::Open);
	openAction->setToolTip(tr(TOOLTIP_OPEN));
	tsconnect(openAction, triggered(), this, openTrace());

	closeAction = new QAction(tr("&Close"), this);
	closeAction->setIcon(QIcon(RESSRC_GPH_CLOSE));
	closeAction->setShortcuts(QKeySequence::Close);
	closeAction->setToolTip(tr(TOOLTIP_CLOSE));
	tsconnect(closeAction, triggered(), this, closeTrace());

	saveAction = new QAction(tr("&Save screenshot as..."), this);
	saveAction->setIcon(QIcon(RESSRC_GPH_SCREENSHOT));
	saveAction->setShortcuts(QKeySequence::SaveAs);
	saveAction->setToolTip(tr(TOOLTIP_SAVESCREEN));
	tsconnect(saveAction, triggered(), this, saveScreenshot());

	showSchedLatencyAction = new QAction(tr("Show scheduling latencies..."),
					     this);
	showSchedLatencyAction->setIcon(QIcon(RESSRC_GPH_LATENCY));
	showSchedLatencyAction->setToolTip(tr(TOOLTIP_SHOWSCHEDLATENCIES));
	tsconnect(showSchedLatencyAction, triggered(), this,
		  showSchedLatencyWidget());

	showWakeupLatencyAction = new QAction(tr("Show wakeup latencies..."),
					      this);
	showWakeupLatencyAction->setIcon(QIcon(RESSRC_GPH_WAKEUP_LATENCY));
	showWakeupLatencyAction->setToolTip(tr(TOOLTIP_SHOWWAKELATENCIES));
	tsconnect(showWakeupLatencyAction, triggered(), this,
		  showWakeupLatencyWidget());

	showTasksAction = new QAction(tr("Show task &list..."), this);
	showTasksAction->setIcon(QIcon(RESSRC_GPH_TASKSELECT));
	showTasksAction->setToolTip(tr(TOOLTIP_SHOWTASKS));
	tsconnect(showTasksAction, triggered(), this, showTaskSelector());

	filterCPUsAction = new QAction(tr("Filter on &CPUs..."), this);
	filterCPUsAction->setIcon(QIcon(RESSRC_GPH_CPUFILTER));
	filterCPUsAction->setToolTip(tr(TOOLTIP_CPUFILTER));
	tsconnect(filterCPUsAction, triggered(), this, filterOnCPUs());

	showEventsAction = new QAction(tr("Filter on &event type..."), this);
	showEventsAction->setIcon(QIcon(RESSRC_GPH_EVENTFILTER));
	showEventsAction->setToolTip(tr(TOOLTIP_SHOWEVENTS));
	tsconnect(showEventsAction, triggered(), this, showEventFilter());

	showArgFilterAction = new QAction(tr("Filter on info field..."), this);
	showArgFilterAction->setIcon(QIcon(RESSRC_GPH_ARGFILTER));
	showArgFilterAction->setToolTip(tr(TOOLTIP_SHOWARGFILTER));
	tsconnect(showArgFilterAction, triggered(), this, showArgFilter());

	timeFilterAction = new QAction(tr("Filter on &time"), this);
	timeFilterAction->setIcon(QIcon(RESSRC_GPH_TIMEFILTER));
	timeFilterAction->setToolTip(tr(TOOLTIP_TIMEFILTER));
	tsconnect(timeFilterAction, triggered(), this, timeFilter());

	graphEnableAction = new QAction(tr("Select &graphs and settings..."),
					this);
	graphEnableAction->setIcon(QIcon(RESSRC_GPH_GRAPHENABLE));
	graphEnableAction->setToolTip(tr(TOOLTIP_GRAPHENABLE));
	tsconnect(graphEnableAction, triggered(), this, showGraphEnable());

	resetFiltersAction = new QAction(tr("&Reset all filters"), this);
	resetFiltersAction->setIcon(QIcon(RESSRC_GPH_RESETFILTERS));
	resetFiltersAction->setToolTip(tr(TOOLTIP_RESETFILTERS));
	resetFiltersAction->setEnabled(false);
	tsconnect(resetFiltersAction, triggered(), this, resetFilters());

	resetTaskColorAction = new QAction(tr("&Reset all filters"), this);
	resetTaskColorAction->setIcon(QIcon(RESSRC_GPH_RESETCOLORS));
	resetTaskColorAction->setToolTip(tr(TOOLTIP_RESETCOLORS));
	resetTaskColorAction ->setEnabled(false);
	tsconnect(resetTaskColorAction , triggered(), this, resetTaskColors());

	exportEventsAction = new QAction(tr("&Export events to a file..."),
					 this);
	exportEventsAction->setIcon(QIcon(RESSRC_GPH_EXPORTEVENTS));
	exportEventsAction->setToolTip(tr(TOOLTIP_EXPORTEVENTS));
	exportEventsAction->setEnabled(false);
	tsconnect(exportEventsAction, triggered(), this,
		  exportEventsTriggered());

	exportCPUAction = new QAction(
		tr("Ex&port cpu-cycles events to a file..."), this);
	exportCPUAction->setIcon(QIcon(RESSRC_GPH_EXPORTCPUEVENTS));
	exportCPUAction->setToolTip(tr(TOOLTIP_EXPORT_CPU));
	exportCPUAction->setEnabled(false);
	tsconnect(exportCPUAction, triggered(), this,
		  exportCPUTriggered());

	cursorZoomAction = new QAction(tr("Cursor &zoom"), this);
	cursorZoomAction->setIcon(QIcon(RESSRC_GPH_CURSOR_ZOOM));
	cursorZoomAction->setToolTip(tr(CURSOR_ZOOM_TOOLTIP));
	tsconnect(cursorZoomAction, triggered(), this, cursorZoom());

	defaultZoomAction = new QAction(tr("&Default zoom"), this);
	defaultZoomAction->setIcon(QIcon(RESSRC_GPH_DEFAULT_ZOOM));
	defaultZoomAction->setToolTip(tr(DEFAULT_ZOOM_TOOLTIP));
	tsconnect(defaultZoomAction, triggered(), this,
		  defaultZoom());

	fullZoomAction = new QAction(tr("&Full zoom"), this);
	fullZoomAction->setIcon(QIcon(RESSRC_GPH_FULL_ZOOM));
	fullZoomAction->setToolTip(tr(FULL_ZOOM_TOOLTIP));
	tsconnect(fullZoomAction, triggered(), this, fullZoom());

	verticalZoomAction = new QAction("&Vertical zooming/scrolling", this);
	verticalZoomAction->setIcon(QIcon(RESSRC_GPH_VERTICAL_ZOOM));
	verticalZoomAction->setToolTip(tr(VERTICAL_ZOOM_TOOLTIP));
	verticalZoomAction->setCheckable(true);
	tsconnect(verticalZoomAction, triggered(), this, verticalZoom());

	showStatsAction = new QAction(tr("Sh&ow stats..."), this);
	showStatsAction->setIcon(QIcon(RESSRC_GPH_GETSTATS));
	showStatsAction->setToolTip(TOOLTIP_GETSTATS);
	tsconnect(showStatsAction, triggered(), this, showStats());

	showStatsTimeLimitedAction = new QAction(
		tr("Show stats c&ursor time..."), this);
	showStatsTimeLimitedAction->setIcon(
		QIcon(RESSRC_GPH_GETSTATS_TIMELIMIT));
	showStatsTimeLimitedAction->setToolTip(TOOLTIP_GETSTATS_TIMELIMITED);
	tsconnect(showStatsTimeLimitedAction, triggered(), this,
		  showStatsTimeLimited());

	exitAction = new QAction(tr("E&xit"), this);
	exitAction->setShortcuts(QKeySequence::Quit);
	exitAction->setToolTip(tr(TOOLTIP_EXIT));
	tsconnect(exitAction, triggered(), this, close());

	backTraceAction = new QAction(tr("&Show backtrace"), this);
	backTraceAction->setIcon(QIcon(RESSRC_GPH_EVENTBTRACE));
	backTraceAction->setToolTip(tr(EVENT_BACKTRACE_TOOLTIP));
	tsconnect(backTraceAction, triggered(), this, showBackTraceTriggered());

	moveBlueAction = new QAction(tr("Move &blue cursor"), this);
	moveBlueAction->setIcon(QIcon(RESSRC_GPH_EVENTMOVEBLUE));
	moveBlueAction->setToolTip(tr(EVENT_MOVEBLUE_TOOLTIP));
	tsconnect(moveBlueAction, triggered(), this, eventMoveBlueTriggered());

	moveRedAction = new QAction(tr("Move &red cursor"), this);
	moveRedAction->setIcon(QIcon(RESSRC_GPH_EVENTMOVERED));
	moveRedAction->setToolTip(tr(EVENT_MOVERED_TOOLTIP));
	tsconnect(moveRedAction, triggered(), this, eventMoveRedTriggered());

	eventPIDAction = new QAction(tr("Filter on event &PID"), this);
	eventPIDAction->setIcon(QIcon(RESSRC_GPH_EVENTFLTPID));
	eventPIDAction->setToolTip(tr(EVENT_PID_TOOLTIP));
	tsconnect(eventPIDAction, triggered(), this, eventPIDTriggered());

	eventCPUAction = new QAction(tr("Filter on event &CPU"), this);
	eventCPUAction->setIcon(QIcon(RESSRC_GPH_EVENTFLTCPU));
	eventCPUAction->setToolTip(tr(EVENT_CPU_TOOLTIP));
	tsconnect(eventCPUAction, triggered(), this, eventCPUTriggered());

	eventTypeAction = new QAction(tr("Filter on event &type"), this);
	eventTypeAction->setIcon(QIcon(RESSRC_GPH_EVENTFLTTYPE));
	eventTypeAction->setToolTip(tr(EVENT_TYPE_TOOLTIP));
	tsconnect(eventTypeAction, triggered(), this, eventTypeTriggered());

	aboutQtAction = new QAction(tr("About &Qt"), this);
	aboutQtAction->setIcon(QIcon(RESSRC_GPH_QT_LOGO));
	aboutQtAction->setToolTip(tr(ABOUT_QT_TOOLTIP));
	tsconnect(aboutQtAction, triggered(), qApp, aboutQt());

	aboutAction = new QAction(tr("&About Traceshark"), this);
	aboutAction->setIcon(QIcon(RESSRC_GPH_SHARK_PENG256));
	aboutAction->setToolTip(tr(ABOUT_TSHARK_TOOLTIP));
	tsconnect(aboutAction, triggered(), this, about());

	aboutQCPAction = new QAction(tr("About QCustom&Plot"), this);
	aboutQCPAction->setIcon(QIcon(RESSRC_GPH_QCP_LOGO));
	aboutAction->setToolTip(tr(SHOW_QCP_TOOLTIP));
	tsconnect(aboutQCPAction, triggered(), this, aboutQCustomPlot());

	licenseAction = new QAction(tr("&License"), this);
	licenseAction->setToolTip(tr(SHOW_LICENSE_TOOLTIP));
	tsconnect(licenseAction, triggered(), this, license());

	addTaskGraphAction = new QAction(tr("Add unified &graph"), this);
	addTaskGraphAction->setIcon(QIcon(RESSRC_GPH_ADD_TASK));
	addTaskGraphAction->setToolTip(tr(ADD_UNIFIED_TOOLTIP));
	tsconnect(addTaskGraphAction, triggered(), this,
		  addTaskGraphTriggered());

	addToLegendAction = new QAction(tr("&Add task to the legend"), this);
	addToLegendAction->setIcon(QIcon(RESSRC_GPH_ADD_TO_LEGEND));
	addToLegendAction->setToolTip(tr(ADD_LEGEND_TOOLTIP));
	tsconnect(addToLegendAction, triggered(), this, addToLegendTriggered());

	colorTaskAction = new QAction(tr("C&olor task"), this);
	colorTaskAction->setIcon(QIcon(RESSRC_GPH_COLORTASK));
	colorTaskAction->setToolTip(tr(COLOR_TASK_TOOLTIP));
	tsconnect(colorTaskAction, triggered(), this, colorToolbarTaskTriggered());

	clearLegendAction = new QAction(tr("&Clear the legend"), this);
	clearLegendAction->setIcon(QIcon(RESSRC_GPH_CLEAR_LEGEND));
	clearLegendAction->setToolTip(tr(CLEAR_LEGEND_TOOLTIP));
	tsconnect(clearLegendAction, triggered(), this, clearLegendTriggered());

	findWakeupAction = new QAction(tr("&Find wakeup"), this);
	findWakeupAction->setIcon(QIcon(RESSRC_GPH_FIND_WAKEUP));
	findWakeupAction->setToolTip(tr(FIND_WAKEUP_TOOLTIP));
	tsconnect(findWakeupAction, triggered(), this, findWakeupTriggered());

	findWakingAction = new QAction(tr("Find &waking"), this);
	findWakingAction->setIcon(QIcon(RESSRC_GPH_FIND_WAKING));
	findWakingAction->setToolTip(tr(FIND_WAKING_TOOLTIP));
	tsconnect(findWakingAction, triggered(), this, findWakingTriggered());

	findWakingDirectAction = new QAction(tr("Find waking &direct"), this);
	findWakingDirectAction->setIcon(QIcon(RESSRC_GPH_FIND_WAKING_DIRECT));
	findWakingDirectAction->setToolTip(tr(FIND_WAKING_DIRECT_TOOLTIP));
	tsconnect(findWakingDirectAction, triggered(), this,
		  findWakingDirectTriggered());

	findSleepAction = new QAction(tr("Find sched_switch &sleep event"),
				      this);
	findSleepAction->setIcon(QIcon(RESSRC_GPH_FIND_SLEEP));
	findSleepAction->setToolTip(tr(TOOLTIP_FIND_SLEEP));
	tsconnect(findSleepAction, triggered(), this, findSleepTriggered());

	removeTaskGraphAction = new QAction(tr("&Remove unified graph"), this);
	removeTaskGraphAction->setIcon(QIcon(RESSRC_GPH_REMOVE_TASK));
	removeTaskGraphAction->setToolTip(tr(REMOVE_TASK_TOOLTIP));
	tsconnect(removeTaskGraphAction, triggered(), this,
		  removeTaskGraphTriggered());

	clearTaskGraphsAction = new QAction(tr("Cl&ear all unified graphs"),
					    this);
	clearTaskGraphsAction->setIcon(QIcon(RESSRC_GPH_CLEAR_TASK));
	clearTaskGraphsAction->setToolTip(tr(CLEAR_TASK_TOOLTIP));
	tsconnect(clearTaskGraphsAction, triggered(), this,
		  clearTaskGraphsTriggered());

	taskFilterAction = new QAction(tr("Filter on selected &task"), this);
	taskFilterAction->setIcon(QIcon(RESSRC_GPH_FILTERCURRENT));
	taskFilterAction->setToolTip(tr(TASK_FILTER_TOOLTIP));
	tsconnect(taskFilterAction, triggered(), this,
		  taskFilterTriggered());

	taskFilterLimitedAction =
		new QAction(tr("Filter on selected task (time &limited)"), this);
	taskFilterLimitedAction->setIcon(QIcon(RESSRC_GPH_FILTERCURRENT_LIMIT));
	taskFilterLimitedAction->setToolTip(tr(TASK_FILTER_TIMELIMIT_TOOLTIP));
	tsconnect(taskFilterLimitedAction, triggered(), this,
		  taskFilterLimitedTriggered());

	setTraceActionsEnabled(false);
	setEventActionsEnabled(false);
	setLegendActionsEnabled(false);
	setCloseActionsEnabled(false);
	setTaskActionsEnabled(false);
	setWakeupActionsEnabled(false);
	setAddTaskGraphActionEnabled(false);
	setTaskGraphRemovalActionEnabled(false);
	setTaskGraphClearActionEnabled(false);
	setAddToLegendActionEnabled(false);
}

void MainWindow::createToolBars()
{
	bool widescreen = Setting::isWideScreen();

	fileToolBar = new QToolBar(tr("&File"));
	addToolBar(Qt::LeftToolBarArea, fileToolBar);
	fileToolBar->addAction(openAction);
	fileToolBar->addAction(closeAction);
	fileToolBar->addAction(saveAction);
	fileToolBar->addAction(exportEventsAction);
	fileToolBar->addAction(exportCPUAction);

	viewToolBar = new QToolBar(tr("&View"));
	addToolBar(Qt::LeftToolBarArea, viewToolBar);
	viewToolBar->addAction(cursorZoomAction);
	viewToolBar->addAction(defaultZoomAction);
	viewToolBar->addAction(fullZoomAction);
	viewToolBar->addAction(verticalZoomAction);
	viewToolBar->addAction(showSchedLatencyAction);
	viewToolBar->addAction(showWakeupLatencyAction);
	viewToolBar->addAction(showTasksAction);
	viewToolBar->addAction(filterCPUsAction);
	viewToolBar->addAction(showEventsAction);
	viewToolBar->addAction(showArgFilterAction);
	viewToolBar->addAction(timeFilterAction);
	viewToolBar->addAction(resetFiltersAction);
	viewToolBar->addAction(resetTaskColorAction);
	viewToolBar->addAction(graphEnableAction);
	viewToolBar->addAction(showStatsAction);
	viewToolBar->addAction(showStatsTimeLimitedAction);

	taskToolBar = new TaskToolBar(tr("Task"));
	if (widescreen) {
		infoWidget->addToolBar(taskToolBar);
	} else {
		addToolBar(Qt::TopToolBarArea, taskToolBar);
		infoWidget->addStretch();
	}

	taskToolBar->addAction(addToLegendAction);
	taskToolBar->addAction(colorTaskAction);
	taskToolBar->addAction(clearLegendAction);
	taskToolBar->addAction(findWakeupAction);
	taskToolBar->addAction(findWakingAction);
	taskToolBar->addAction(findWakingDirectAction);
	taskToolBar->addAction(findSleepAction);
	taskToolBar->addAction(addTaskGraphAction);
	taskToolBar->addAction(removeTaskGraphAction);
	taskToolBar->addAction(clearTaskGraphsAction);
	taskToolBar->addAction(taskFilterAction);
	taskToolBar->addAction(taskFilterLimitedAction);
	taskToolBar->addStretch();
}

void MainWindow::createMenus()
{
	fileMenu = menuBar()->addMenu(tr("&File"));
	fileMenu->addAction(openAction);
	fileMenu->addAction(closeAction);
	fileMenu->addAction(saveAction);
	fileMenu->addSeparator();
	fileMenu->addAction(exportEventsAction);
	fileMenu->addAction(exportCPUAction);
	fileMenu->addSeparator();
	fileMenu->addAction(exitAction);

	viewMenu = menuBar()->addMenu(tr("&View"));
	viewMenu->addAction(cursorZoomAction);
	viewMenu->addAction(defaultZoomAction);
	viewMenu->addAction(fullZoomAction);
	viewMenu->addAction(verticalZoomAction);
	viewMenu->addAction(showSchedLatencyAction);
	viewMenu->addAction(showWakeupLatencyAction);
	viewMenu->addAction(showTasksAction);
	viewMenu->addAction(filterCPUsAction);
	viewMenu->addAction(showEventsAction);
	viewMenu->addAction(showArgFilterAction);
	viewMenu->addAction(timeFilterAction);
	viewMenu->addAction(resetFiltersAction);
	viewMenu->addAction(resetTaskColorAction);
	viewMenu->addAction(graphEnableAction);
	viewMenu->addAction(showStatsAction);
	viewMenu->addAction(showStatsTimeLimitedAction);

	taskMenu = menuBar()->addMenu(tr("&Task"));
	taskMenu->addAction(addToLegendAction);
	taskMenu->addAction(colorTaskAction);
	taskMenu->addAction(clearLegendAction);
	taskMenu->addAction(findWakeupAction);
	taskMenu->addAction(findWakingAction);
	taskMenu->addAction(findWakingDirectAction);
	taskMenu->addAction(findSleepAction);
	taskMenu->addAction(addTaskGraphAction);
	taskMenu->addAction(removeTaskGraphAction);
	taskMenu->addAction(clearTaskGraphsAction);
	taskMenu->addAction(taskFilterAction);
	taskMenu->addAction(taskFilterLimitedAction);

	eventMenu = menuBar()->addMenu(tr("&Event"));
	eventMenu->addAction(backTraceAction);
	eventMenu->addAction(moveBlueAction);
	eventMenu->addAction(moveRedAction);
	eventMenu->addAction(eventPIDAction);
	eventMenu->addAction(eventCPUAction);
	eventMenu->addAction(eventTypeAction);

	helpMenu = menuBar()->addMenu(tr("&Help"));
	helpMenu->addAction(aboutAction);
	helpMenu->addAction(aboutQCPAction);
	helpMenu->addAction(aboutQtAction);
	helpMenu->addAction(licenseAction);
}

void MainWindow::createStatusBar()
{
	statusLabel = new QLabel(" W999 ");
	statusLabel->setAlignment(Qt::AlignHCenter);
	statusLabel->setMinimumSize(statusLabel->sizeHint());
	statusBar()->addWidget(statusLabel);

	statusStrings[STATUS_NOFILE] = new QString(tr("No file loaded"));
	statusStrings[STATUS_FILE] = new QString(tr("Loaded file "));
	statusStrings[STATUS_ERROR] = new QString(tr("An error has occurred"));

	setStatus(STATUS_NOFILE);
}

void MainWindow::createDialogs()
{
	errorDialog = new ErrorDialog(this);
	licenseDialog = new LicenseDialog(this);
	eventInfoDialog = new EventInfoDialog(this);
	taskSelectDialog =
		new TaskSelectDialog(this, tr("Task Selector"),
				     TaskSelectDialog::TaskSelectRegular);
	statsDialog = new TaskSelectDialog(this, tr("Global Statistics"),
					   TaskSelectDialog::TaskSelectStats);
	statsLimitedDialog =
		new TaskSelectDialog(this, tr("Cursor Statistics"),
				     TaskSelectDialog::TaskSelectStatsLimited);

	taskSelectDialog->setAllowedAreas(Qt::LeftDockWidgetArea);
	statsDialog->setAllowedAreas(Qt::LeftDockWidgetArea);
	statsLimitedDialog->setAllowedAreas(Qt::RightDockWidgetArea);

	eventSelectDialog = new EventSelectDialog(this);
	cpuSelectDialog = new CPUSelectDialog(this);
	graphEnableDialog = new GraphEnableDialog(settingStore, this);
	regexDialog = new RegexDialog(this);
	schedLatencyWidget = new LatencyWidget(tr("Scheduling Latencies"),
					       Latency::TYPE_SCHED, this);
	schedLatencyWidget->setAllowedAreas(Qt::RightDockWidgetArea);
	wakeupLatencyWidget = new LatencyWidget(tr("Wakeup Latencies"),
						Latency::TYPE_WAKEUP, this);
	wakeupLatencyWidget->setAllowedAreas(Qt::LeftDockWidgetArea);

	vtl::set_error_handler(errorDialog);
}

void MainWindow::plotConnections()
{
	tsconnect(tracePlot, mouseWheel(QWheelEvent*), this, mouseWheel());
	tsconnect(tracePlot->xAxis, rangeChanged(QCPRange), tracePlot->xAxis2,
		  setRange(QCPRange));
	tsconnect(tracePlot, mousePress(QMouseEvent*), this, mousePress());
	tsconnect(tracePlot, selectionChangedByUser() , this,
		  selectionChanged());
	tsconnect(tracePlot, legendDoubleClick(QCPLegend*,
					       QCPAbstractLegendItem*,
					       QMouseEvent*), this,
		  legendDoubleClick(QCPLegend*, QCPAbstractLegendItem*));
	tsconnect(tracePlot, mouseDoubleClick(QMouseEvent*),
		  this, plotDoubleClicked(QMouseEvent*));
}

void MainWindow::widgetConnections()
{
	tsconnect(infoWidget, valueChanged(vtl::Time, int),
		  this, infoValueChanged(vtl::Time, int));

	/* Events widget */
	tsconnect(eventsWidget, eventDoubleClicked(EventsModel::column_t,
						   const TraceEvent &),
		  this, handleEventDoubleClicked(EventsModel::column_t,
						 const TraceEvent &));
	tsconnect(eventsWidget, eventSelected(const TraceEvent *),
		  this, handleEventSelected(const TraceEvent *));

	/* TaskToolBar widget */
	tsconnect(taskToolBar, LegendEmptyChanged(bool), this,
		  legendEmptyChanged(bool));
}

void MainWindow::dialogConnections()
{
	/* task select dialog */
	tsconnect(taskSelectDialog, addTaskGraph(int), this, addTaskGraph(int));
	tsconnect(taskSelectDialog, needReplot(), this, doReplot());
	tsconnect(taskSelectDialog, needLegendCheck(), this, doLegendCheck());
	tsconnect(taskSelectDialog, addTaskToLegend(int), this,
		  addTaskToLegend(int));
	tsconnect(taskSelectDialog, createFilter(QMap<int, int> &, bool, bool),
		  this, createPidFilter(QMap<int, int> &, bool, bool));
	tsconnect(taskSelectDialog, resetFilter(), this, resetPidFilter());
	tsconnect(taskSelectDialog, QDockWidgetNeedsRemoval(QDockWidget*),
		  this, removeQDockWidget(QDockWidget*));
	tsconnect(taskSelectDialog, taskDoubleClicked(int),
		  this, taskTriggered(int));
	tsconnect(taskSelectDialog, doExport(bool), this, exportTasks(bool));
	tsconnect(taskSelectDialog, colorChangeReq(const QList<int> *),
		  this, changeColors(const QList<int> *));

	/* statistics Dialog */
	tsconnect(statsDialog, addTaskGraph(int), this, addTaskGraph(int));
	tsconnect(statsDialog, needReplot(), this, doReplot());
	tsconnect(statsDialog, needLegendCheck(), this, doLegendCheck());
	tsconnect(statsDialog, addTaskToLegend(int), this,
		  addTaskToLegend(int));
	tsconnect(statsDialog, createFilter(QMap<int, int> &, bool, bool),
		  this, createPidFilter(QMap<int, int> &, bool, bool));
	tsconnect(statsDialog, resetFilter(), this, resetPidFilter());
	tsconnect(statsDialog, QDockWidgetNeedsRemoval(QDockWidget*),
		  this, removeQDockWidget(QDockWidget*));
	tsconnect(statsDialog, taskDoubleClicked(int),
		  this, taskTriggered(int));
	tsconnect(statsDialog, doExport(bool), this, exportStats(bool));
	tsconnect(statsDialog, colorChangeReq(const QList<int> *),
		  this, changeColors(const QList<int> *));

	/* Time limited statistics Dialog */
	tsconnect(statsLimitedDialog, addTaskGraph(int), this,
		  addTaskGraph(int));
	tsconnect(statsLimitedDialog, needReplot(), this, doReplot());
	tsconnect(statsLimitedDialog, addTaskToLegend(int), this,
		  addTaskToLegend(int));
	tsconnect(statsLimitedDialog, needLegendCheck(), this, doLegendCheck());
	tsconnect(statsLimitedDialog,
		  createFilter(QMap<int, int> &, bool, bool),
		  this, createPidFilter(QMap<int, int> &, bool, bool));
	tsconnect(statsLimitedDialog, resetFilter(), this, resetPidFilter());
	tsconnect(statsLimitedDialog, QDockWidgetNeedsRemoval(QDockWidget*),
		  this, removeQDockWidget(QDockWidget*));
	tsconnect(statsLimitedDialog, taskDoubleClicked(int),
		  this, taskTriggered(int));
	tsconnect(statsLimitedDialog, doExport(bool), this,
		  exportStatsTimeLimited(bool));
	tsconnect(statsLimitedDialog, colorChangeReq(const QList<int> *),
		  this, changeColors(const QList<int> *));

	/* the CPU filter dialog */
	tsconnect(cpuSelectDialog, createFilter(QMap<unsigned, unsigned> &,
						bool),
		  this, createCPUFilter(QMap<unsigned, unsigned> &, bool));
	tsconnect(cpuSelectDialog, resetFilter(), this, resetCPUFilter());

	/* event select dialog */
	tsconnect(eventSelectDialog, createFilter(QMap<event_t, event_t> &,
						  bool),
		  this, createEventFilter(QMap<event_t, event_t> &, bool));
	tsconnect(eventSelectDialog, resetFilter(), this, resetEventFilter());

	/* graph enable dialog */
	tsconnect(graphEnableDialog, settingsChanged(),
		  this, consumeSettings());
	tsconnect(graphEnableDialog, filterSettingsChanged(),
		  this, consumeFilterSettings());
	tsconnect(graphEnableDialog, sizeChanged(),
		  this, consumeSizeChange());
	tsconnect(graphEnableDialog, sizeRequest(),
		  this, transmitSize());

	/* regex dialog */
	tsconnect(regexDialog, createFilter(RegexFilter &, bool),
		  this, createRegexFilter(RegexFilter &, bool));
	tsconnect(regexDialog, resetFilter(), this, resetRegexFilter());

	/* sched latency widget */
	tsconnect(schedLatencyWidget,
		  latencyDoubleClicked(const Latency *),
		  this, showLatency(const Latency *));
	tsconnect(schedLatencyWidget, QDockWidgetNeedsRemoval(QDockWidget *),
		  this, removeQDockWidget(QDockWidget*));
	tsconnect(schedLatencyWidget, exportRequested(int),
		  this, exportSchedLatencies(int));

	/* wakeup latency widget */
	tsconnect(wakeupLatencyWidget,
		  latencyDoubleClicked(const Latency *),
		  this, showLatency(const Latency *));
	tsconnect(wakeupLatencyWidget, QDockWidgetNeedsRemoval(QDockWidget *),
		  this, removeQDockWidget(QDockWidget*));
	tsconnect(wakeupLatencyWidget, exportRequested(int),
		  this, exportWakeupLatencies(int));
}

void MainWindow::setStatus(status_t status, const QString *fileName)
{
	QString string;
	if (fileName != nullptr)
		string = *statusStrings[status] + *fileName;
	else
		string = *statusStrings[status];

	statusLabel->setText(string);
}

int MainWindow::loadTraceFile(const QString &fileName)
{
	qint64 start, stop;
        int rval;

	stateFile->setTraceFile(fileName);
	rval = stateFile->loadState();

	if (rval != 0)
		vtl::warn(rval, "Failed to load state file");

	printf("opening %s\n", fileName.toLocal8Bit().data());
	
	start = QDateTime::currentDateTimeUtc().toMSecsSinceEpoch();
	rval = analyzer->open(fileName);
	stop = QDateTime::currentDateTimeUtc().toMSecsSinceEpoch();

	stop = stop - start;

	printf("Loading took %.6lf s\n", (double) stop / 1000);
	return rval;
}

void MainWindow::selectionChanged()
{
	TaskGraph *graph = selectedGraph();
	if (graph == nullptr) {
		setTaskActionsEnabled(false);
		taskToolBar->removeTaskGraph();
		setTaskGraphRemovalActionEnabled(false);
		setAddTaskGraphActionEnabled(false);
		setAddToLegendActionEnabled(false);
		return;
	}

	setTaskActionsEnabled(true);
	taskToolBar->setTaskGraph(graph);
	updateTaskGraphActions();
	updateAddToLegendAction();
}

void MainWindow::legendDoubleClick(QCPLegend * /* legend */,
				   QCPAbstractLegendItem *abstractItem)
{
	QCPPlottableLegendItem *plottableItem;
	QCPAbstractPlottable *plottable;
	QCPGraph *legendGraph;

	plottableItem = qobject_cast<QCPPlottableLegendItem*>(abstractItem);
	if (plottableItem == nullptr)
		return;
	plottable = plottableItem->plottable();
	legendGraph = qobject_cast<QCPGraph*>(plottable);
	if (legendGraph == nullptr)
		return;

	handleLegendGraphDoubleClick(legendGraph);
}

void MainWindow::legendEmptyChanged(bool empty)
{
	setLegendActionsEnabled(!empty);
}

void MainWindow::handleLegendGraphDoubleClick(QCPGraph *graph)
{
	TaskGraph *tgraph = TaskGraph::fromQCPGraph(graph);
	const Task *task;

	if (tgraph == nullptr)
		return;
	tgraph->removeFromLegend();
	task = tgraph->getTask();
	/*
	 * Inform the TaskToolBar class that the pid has been removed. This is
	 * needed because TaskToolBar keeps track of this for the purpose of
	 * preventing the same pid being added twice from different legend 
	 * graphs, there might be "identical" legend graphs when the same pid
	 * has migrated between CPUs
	 */
	if (task != nullptr)
		taskToolBar->pidRemoved(task->pid);
	updateAddToLegendAction();
}

void MainWindow::addTaskToLegend(int pid)
{
	CPUTask *cpuTask = nullptr;
	unsigned int cpu;
	int realpid;
	Task *task = analyzer->findRealTask(pid);

	/*
	 * I believe that if task == nullptr, then we will probably fail to
	 * find any per CPU task below but let's anyway try with the original
	 * pid. The idea behind using findRealTask is that pid may be a ghost
	 * pid selected by the user in the TaskSelectDialog class.
	 */
	if (task == nullptr)
		realpid = pid;
	else
		realpid = task->pid;

	/*
	 * Let's find a per CPU taskGraph, because they are always created,
	 * the unified graphs only exist for those that have been chosen to be
	 * displayed by the user
	 */
	for (cpu = 0; cpu < analyzer->getNrCPUs(); cpu++) {
		cpuTask = analyzer->findCPUTask(realpid, cpu);
		if (cpuTask != nullptr)
			break;
	}

	if (cpuTask == nullptr)
		return;

	taskToolBar->addTaskGraphToLegend(cpuTask->graph);
}

void MainWindow::setEventsWidgetEvents()
{
	if (analyzer->isFiltered())
		eventsWidget->setEvents(&analyzer->filteredEvents);
	else
		eventsWidget->setEvents(analyzer->events);
}

void MainWindow::scrollTo(const vtl::Time &time)
{
	vtl::Time start, end;
	start = analyzer->getStartTime();
	end = analyzer->getEndTime();

	/*
	 * Fixme:
	 * For some reason the EventsWidget doesn't want to make its first
	 * scroll to somewhere in the middle of the trace. As a work around
	 * we first scroll to the beginning and to the end, and then to
	 * where we want.
	 */
	eventsWidget->scrollTo(start);
	eventsWidget->scrollTo(end);
	eventsWidget->scrollTo(time);
}

void MainWindow::updateResetFiltersEnabled(void)
{
	if (analyzer->isFiltered()) {
		resetFiltersAction->setEnabled(true);
	} else {
		resetFiltersAction->setEnabled(false);
	}
}

void MainWindow::timeFilter(void)
{
	double min, max;
	vtl::Time saved = eventsWidget->getSavedScroll();

	min = TSMIN(cursorPos[TShark::RED_CURSOR],
		    cursorPos[TShark::BLUE_CURSOR]);
	max = TSMAX(cursorPos[TShark::RED_CURSOR],
		    cursorPos[TShark::BLUE_CURSOR]);

	vtl::Time tmin = vtl::Time::fromDouble(min);
	vtl::Time tmax = vtl::Time::fromDouble(max);

	eventsWidget->beginResetModel();
	analyzer->createTimeFilter(tmin, tmax, false);
	setEventsWidgetEvents();
	eventsWidget->endResetModel();
	scrollTo(saved);
	updateResetFiltersEnabled();
}

void MainWindow::createEventCPUFilter(const TraceEvent &event)
{
	eventCPUMap.clear();
	eventCPUMap[event.cpu] =  event.cpu;
	createCPUFilter(eventCPUMap, false);
}

void MainWindow::createEventPIDFilter(const TraceEvent &event)
{
	bool incl;

	incl = settingStore->getValue(Setting::EVENT_PID_FLT_INCL_ON).boolv();
	eventPIDMap.clear();
	eventPIDMap[event.pid] = event.pid;
	createPidFilter(eventPIDMap, false, incl);
}

void MainWindow::createEventTypeFilter(const TraceEvent &event)
{
	eventTypeMap.clear();
	eventTypeMap[event.type] = event.type;
	createEventFilter(eventTypeMap, false);
}

void MainWindow::createPidFilter(QMap<int, int> &map,
				 bool orlogic, bool inclusive)
{
	vtl::Time saved = eventsWidget->getSavedScroll();

	eventsWidget->beginResetModel();
	analyzer->createPidFilter(map, orlogic, inclusive);
	setEventsWidgetEvents();
	eventsWidget->endResetModel();
	scrollTo(saved);
	updateResetFiltersEnabled();
}

void MainWindow::createCPUFilter(QMap<unsigned, unsigned> &map, bool orlogic)
{
	vtl::Time saved = eventsWidget->getSavedScroll();

	eventsWidget->beginResetModel();
	analyzer->createCPUFilter(map, orlogic);
	setEventsWidgetEvents();
	eventsWidget->endResetModel();
	scrollTo(saved);
	updateResetFiltersEnabled();
}

void MainWindow::createEventFilter(QMap<event_t, event_t> &map, bool orlogic)
{
	vtl::Time saved = eventsWidget->getSavedScroll();

	eventsWidget->beginResetModel();
	analyzer->createEventFilter(map, orlogic);
	setEventsWidgetEvents();
	eventsWidget->endResetModel();
	scrollTo(saved);
	updateResetFiltersEnabled();
}

void MainWindow::createRegexFilter(RegexFilter &regexFilter, bool orlogic)
{
	vtl::Time saved = eventsWidget->getSavedScroll();
	int ts_errno;

	eventsWidget->beginResetModel();
	ts_errno = analyzer->createRegexFilter(regexFilter, orlogic);
	setEventsWidgetEvents();
	eventsWidget->endResetModel();
	scrollTo(saved);
	updateResetFiltersEnabled();
	if (ts_errno != 0)
		vtl::warn(ts_errno, "Failed to compile regex");
}

void MainWindow::resetPidFilter()
{
	resetFilter(FilterState::FILTER_PID);
}

void MainWindow::resetCPUFilter()
{
	resetFilter(FilterState::FILTER_CPU);
}

void MainWindow::resetEventFilter()
{
	resetFilter(FilterState::FILTER_EVENT);
}

void MainWindow::resetRegexFilter()
{
	resetFilter(FilterState::FILTER_REGEX);
}

void MainWindow::resetFilter(FilterState::filter_t filter)
{
	vtl::Time saved;

	if (!analyzer->filterActive(filter))
		return;

	saved = eventsWidget->getSavedScroll();
	eventsWidget->beginResetModel();
	analyzer->disableFilter(filter);
	setEventsWidgetEvents();
	eventsWidget->endResetModel();
	scrollTo(saved);
	updateResetFiltersEnabled();
}

void MainWindow::resetFilters()
{
	vtl::Time saved;

	if (!analyzer->isFiltered())
		return;

	const TraceEvent *event = eventsWidget->getSelectedEvent();

	if (event != nullptr) {
		saved = event->time;
	} else {
		saved = eventsWidget->getSavedScroll();
	}

	eventsWidget->beginResetModel();
	analyzer->disableAllFilters();
	setEventsWidgetEvents();
	eventsWidget->endResetModel();
	scrollTo(saved);
	updateResetFiltersEnabled();
}

void MainWindow::exportEvents(TraceAnalyzer::exporttype_t export_type)
{
	QStringList fileNameList;
	QString fileName;
	int ts_errno;
	QString caption;

	if (analyzer->events->size() <= 0) {
		vtl::warnx("The trace is empty. There is nothing to export");
		return;
	}

	if (analyzer->getTraceType() != TRACE_TYPE_PERF) {
		vtl::warnx("The trace type is not perf. Only perf traces can be exported");
		return;
	}

	switch (export_type) {
	case TraceAnalyzer::EXPORT_TYPE_CPU_CYCLES:
		caption = tr("Export CPU cycles events");
		break;
	case TraceAnalyzer::EXPORT_TYPE_ALL:
		caption = tr("Export all filtered events");
		break;
	default:
		caption = tr("Unknown export");
		break;
	}

	fileName = QFileDialog::getSaveFileName(this, caption, QString(),
						ASCTXT_FILTER, nullptr,
						foptions);
	if (fileName.isEmpty())
		return;

	TShark::checkSuffix(&fileName, ASC_SUFFIX, TXT_SUFFIX);

	if (!analyzer->exportTraceFile(fileName.toLocal8Bit().data(),
				       &ts_errno, export_type)) {
		vtl::warn(ts_errno, "Failed to export trace to %s",
			  fileName.toLocal8Bit().data());
	}
}

void MainWindow::exportCPUTriggered()
{
	exportEvents(TraceAnalyzer::EXPORT_TYPE_CPU_CYCLES);
}

void MainWindow::exportEventsTriggered()
{
	exportEvents(TraceAnalyzer::EXPORT_TYPE_ALL);
}

void MainWindow::exportSchedLatencies(int format)
{
	exportLatencies((TraceAnalyzer::exportformat_t)format,
			TraceAnalyzer::LATENCY_SCHED);
}

void MainWindow::exportWakeupLatencies(int format)
{
	exportLatencies((TraceAnalyzer::exportformat_t)format,
			TraceAnalyzer::LATENCY_WAKEUP);
}

void MainWindow::exportLatencies(TraceAnalyzer::exportformat_t format,
				 TraceAnalyzer::latencytype_t type)
{
	QString caption;
	QString fileName;
	int ts_errno;
	QString selected;
	QString filter;
	TraceAnalyzer::exportformat_t override_fmt = format;

	/*
	 * The first filter will be the default one displayed by the
	 * QFileDialog::getSaveFileName() dialog. The "format" variable contains
	 * the format selected in LatencyWidget. So based on this we select the
	 * default format by arranging the order of the filter string. The user
	 * still have the option to select another format. This will be recorded
	 * in the "selected" variable.
	 */
	switch (format) {
	case TraceAnalyzer::EXPORT_ASCII:
		filter = TXT_FILTER + F_SEP + CSV_FILTER;
		break;
	case TraceAnalyzer::EXPORT_CSV:
		filter = CSV_FILTER + F_SEP + TXT_FILTER;
		break;
	default:
		vtl::warn(TS_ERROR_INTERNAL, "Unknown file format");
		return;
	}

	switch (type) {
	case TraceAnalyzer::LATENCY_WAKEUP:
		caption = tr("Export the wakeup latencies");
		break;
	case TraceAnalyzer::LATENCY_SCHED:
		caption = tr("Export the scheduling latencies");
		break;
	default:
		vtl::warn(TS_ERROR_INTERNAL, "Unknown latency type");
		return;
	}

	fileName = QFileDialog::getSaveFileName(this, caption, QString(),
						filter, &selected, foptions);

	if (fileName.isEmpty())
		return;

	/*
	 * The purpose of this override_fmt is to allow the user to select
	 * another format in the dialog provided by
	 * QFileDialog::getSaveFileName(). This will override the originally
	 * selected format in the LatencyWidget widget.
	 *
	 * However, first we will check if the user has taken the trouble a
	 * suffix. In that case we will follow that.
	 */
	if (fileName.endsWith(TXT_SUFFIX) || fileName.endsWith(ASC_SUFFIX))
		override_fmt = TraceAnalyzer::EXPORT_ASCII;
	else if (fileName.endsWith(CSV_SUFFIX))
		override_fmt = TraceAnalyzer::EXPORT_CSV;
	else if (selected == TXT_FILTER) {
		override_fmt = TraceAnalyzer::EXPORT_ASCII;
		TShark::checkSuffix(&fileName, TXT_SUFFIX);
	} else if (selected == CSV_FILTER) {
		override_fmt = TraceAnalyzer::EXPORT_CSV;
		TShark::checkSuffix(&fileName, CSV_SUFFIX);
	} else {
		/*
		 * I believe that this should never happen but let's handle it
		 * anyway.
		 */
		switch (format) {
		case TraceAnalyzer::EXPORT_ASCII:
			TShark::checkSuffix(&fileName, TXT_SUFFIX);
			break;
		case TraceAnalyzer::EXPORT_CSV:
			TShark::checkSuffix(&fileName, CSV_SUFFIX);
			break;
		default:
			override_fmt = TraceAnalyzer::EXPORT_CSV;
			TShark::checkSuffix(&fileName, CSV_SUFFIX);
		}
	}

	if (!analyzer->exportLatencies(override_fmt, type,
				       fileName.toLocal8Bit().data(),
				       &ts_errno))
		vtl::warn(ts_errno, "Failed to export latencies to %s",
			  fileName.toLocal8Bit().data());

}

void MainWindow::consumeSettings()
{
	unsigned int cpu;
	QList<int> taskGraphs;
	QList<int> legendPids;
	vtl::Time redtime, bluetime;
	bool selected = false;
	bool unified = false;
	int selected_pid = 0;
	unsigned int selected_cpu = 0;
	TaskGraph *selected_graph;
	enum TaskGraph::GraphType graph_type;

	if (!analyzer->isOpen()) {
		setupOpenGL();
		graphEnableDialog->checkConsumption();
		return;
	}

	/* Save the PIDs of the tasks that have a unified task graph */
	taskGraphs = taskRangeAllocator->getPidList();

	/* Save the Pids of the tasks that have a legend */
	legendPids = taskToolBar->legendPidList();

	/* Save the cursor time */
	Cursor *redCursor = cursors[TShark::RED_CURSOR];
	Cursor *blueCursor = cursors[TShark::BLUE_CURSOR];

	if (redCursor != nullptr)
		redtime = redCursor->getTime();
	if (blueCursor != nullptr)
		bluetime = blueCursor->getTime();

	/* Save the zoom */
	QCPRange savedRangeX = tracePlot->xAxis->range();

	/* Save wether a task was selected */
	selected_graph = selectedGraph();
	if (selected_graph != nullptr) {
		selected = true;
		selected_cpu = selected_graph->getCPU();
		selected_pid = selected_graph->getPid();
		graph_type = selected_graph->getGraphType();
		unified = graph_type == TaskGraph::GRAPH_UNIFIED;
	}

	clearPlot();
	setupOpenGL();
	taskToolBar->clear();

	for (cpu = 0; cpu <= analyzer->getMaxCPU(); cpu++) {
		DEFINE_CPUTASKMAP_ITERATOR(iter);
		for (iter = analyzer->cpuTaskMaps[cpu].begin();
		     iter != analyzer->cpuTaskMaps[cpu].end();
		     iter++) {
			CPUTask &task = iter.value();
			delete task.graph;
			task.graph = nullptr;
			task.horizontalDelayBars = nullptr;
			task.verticalDelayBars = nullptr;
		}
	}

	DEFINE_TASKMAP_ITERATOR(iter);
	for (iter = analyzer->taskMap.begin();
	     iter != analyzer->taskMap.end();
	     iter++) {
		Task *task = iter.value().task;
		if (task->graph != nullptr) {
			/*
			 * This implies that the task had a unified graph added.
			 * We delete the TaskGraph object and set the pointers
			 * to nullptr. The actual QCPGraph objects is already
			 * deleted by the clearPlot() function above.
			 */
			delete task->graph;
			task->graph = nullptr;
			task->delayGraph = nullptr;
			task->runningGraph = nullptr;
			task->preemptedGraph = nullptr;
			task->uninterruptibleGraph = nullptr;
			task->horizontalDelayBars = nullptr;
		}
	}

	computeLayout();
	setupCursors(redtime, bluetime);
	rescaleTrace();
	showTrace();
	tracePlot->show();

	tracePlot->xAxis->setRange(savedRangeX);
	/* Restore the unified task graphs from the list */
	QList<int>::const_iterator j;
	for (j = taskGraphs.begin(); j != taskGraphs.end(); j++)
		addTaskGraph(*j);

	/* Restore the legends from the list */
	for (j = legendPids.begin(); j != legendPids.end(); j++)
		addTaskToLegend(*j);

	if (selected) {
		/* Restore the graph selection */
		if (unified)
			selectTaskByPid(selected_pid, nullptr,
					PR_TRY_TASKGRAPH);
		else
			selectTaskByPid(selected_pid, &selected_cpu,
					PR_CPUGRAPH_ONLY);
	} else {
		/* No task was selected */
		tracePlot->replot();
		setTaskActionsEnabled(false);
		updateAddToLegendAction();
		updateTaskGraphActions();
	}
	graphEnableDialog->checkConsumption();
}

void MainWindow::consumeFilterSettings()
{
	bool inclusive =
		settingStore->getValue(Setting::EVENT_PID_FLT_INCL_ON).boolv();
	if (analyzer->updatePidFilter(inclusive))
		/*
		 * When this function is called, the focus is often on the
		 * graphEnableDialog widget but the user still might be
		 * expecting to see an immediate update of the eventsWidget,
		 * therefore we call repaint() here. Unfortunately, it doesn't
		 * help to call update().
		 */
		eventsWidget->repaint();
}

void MainWindow::consumeSizeChange()
{
	int wt, ht;

	if (settingStore->getValue(Setting::LOAD_WINDOW_SIZE_START).boolv()) {
		ht = settingStore->getValue(Setting::MAINWINDOW_HEIGHT).intv();
		wt = settingStore->getValue(Setting::MAINWINDOW_WIDTH).intv();
		if (wt != width() || ht != height())
			resize(wt, ht);
	}
}

void MainWindow::transmitSize()
{
	graphEnableDialog->setMainWindowSize(width(), height());
}

void MainWindow::addTaskGraph(int pid)
{
	/* Add a unified scheduling graph for pid */
	bool isNew;
	TaskRange *taskRange;
	TaskGraph *taskGraph;
	unsigned int cpu;
	CPUTask *cpuTask = nullptr;

	taskRange = taskRangeAllocator->getTaskRange(pid, isNew);

	if (!isNew || taskRange == nullptr)
		return;

	Task *task = analyzer->findRealTask(pid);

	if (task == nullptr) {
		taskRangeAllocator->putTaskRange(taskRange);
		return;
	}

	/* task->pid may be different from pid, if pid is a ghost task */
	QColor color = analyzer->getTaskColor(task->pid);

	for (cpu = 0; cpu < analyzer->getNrCPUs(); cpu++) {
		cpuTask = analyzer->findCPUTask(task->pid, cpu);
		if (cpuTask != nullptr)
			break;
	}
	if (cpuTask == nullptr || cpuTask->graph == nullptr) {
		taskRangeAllocator->putTaskRange(taskRange);
		return;
	}

	bottom = taskRangeAllocator->getBottom();

	taskGraph = new TaskGraph(tracePlot, 0, TaskGraph::GRAPH_UNIFIED);
	taskGraph->setTaskGraphForLegend(cpuTask->graph);
	QPen pen = QPen();

	pen.setColor(color);
	pen.setWidth(settingStore->getValue(Setting::LINE_WIDTH).intv());
	taskGraph->setPen(pen);
	taskGraph->setTask(task);

	task->offset = taskRange->lower;
	task->scale = schedHeight;
	task->doScale();
	task->doScaleDelay();
	task->doScaleRunning();
	task->doScalePreempted();
	task->doScaleUnint();

	taskGraph->setData(task->schedTimev, task->scaledSchedData);
	task->graph = taskGraph;

	/* Add the horizontal wakeup graph as well */
	QCPGraph *graph = tracePlot->addGraph(tracePlot->xAxis,
					      tracePlot->yAxis);
	QCPErrorBars *errorBars = new QCPErrorBars(tracePlot->xAxis,
						   tracePlot->yAxis);
	errorBars->setAntialiased(false);
	QCPScatterStyle style = QCPScatterStyle(QCPScatterStyle::ssDot);
	style.setPen(pen);
	graph->setScatterStyle(style);
	graph->setLineStyle(QCPGraph::lsNone);
	graph->setAdaptiveSampling(true);
	graph->setData(task->delayTimev, task->delayHeight);
	errorBars->setData(task->delay, task->delayZero);
	errorBars->setErrorType(QCPErrorBars::etKeyError);
	errorBars->setPen(pen);
	errorBars->setWhiskerWidth(4);
	errorBars->setDataPlottable(graph);
	task->delayGraph = graph;
	task->horizontalDelayBars = errorBars;

	addStillRunningTaskGraph(task);
	addPreemptedTaskGraph(task);
	addUninterruptibleTaskGraph(task);

	/*
	 * We only modify the lower part of the range to show the newly
	 * added unified task graph.
	 */
	QCPRange range = tracePlot->yAxis->range();
	tracePlot->yAxis->setRange(QCPRange(bottom, range.upper));

	updateTaskGraphActions();
}

void MainWindow::doReplot()
{
	tracePlot->replot();
}

void MainWindow::doLegendCheck()
{
	updateAddToLegendAction();
}

void MainWindow::addAccessoryTaskGraph(QCPGraph **graphPtr,
				       const QString &name,
				       const QVector<double> &timev,
				       const QVector<double> &scaledData,
				       QCPScatterStyle::ScatterShape sshape,
				       double size,
				       const QColor &color)
{
	/* Add the still running graph on top of the other two... */
	QCPGraph *graph;
	QPen pen;
	const int lwidth = settingStore->getValue(Setting::LINE_WIDTH).intv();
	const double adjsize = adjustScatterSize(size, lwidth);
	QCPScatterStyle style = QCPScatterStyle(sshape, adjsize);
	if (timev.size() <= 0) {
		*graphPtr = nullptr;
		return;
	}
	graph = tracePlot->addGraph(tracePlot->xAxis, tracePlot->yAxis);
	graph->setName(name);
	pen.setColor(color);
	pen.setWidth(settingStore->getValue(Setting::LINE_WIDTH).intv());
	style.setPen(pen);
	graph->setScatterStyle(style);
	graph->setLineStyle(QCPGraph::lsNone);
	graph->setAdaptiveSampling(true);
	graph->setData(timev, scaledData);
	*graphPtr = graph;
}

void MainWindow::addStillRunningTaskGraph(Task *task)
{
	addAccessoryTaskGraph(&task->runningGraph, RUNNING_NAME,
			      task->runningTimev, task->scaledRunningData,
			      RUNNING_SHAPE, RUNNING_SIZE, RUNNING_COLOR);
}

void MainWindow::addPreemptedTaskGraph(Task *task)
{
	addAccessoryTaskGraph(&task->preemptedGraph, PREEMPTED_NAME,
			      task->preemptedTimev, task->scaledPreemptedData,
			      PREEMPTED_SHAPE, PREEMPTED_SIZE, PREEMPTED_COLOR);
}

void MainWindow::addUninterruptibleTaskGraph(Task *task)
{
	addAccessoryTaskGraph(&task->uninterruptibleGraph, UNINT_NAME,
			      task->uninterruptibleTimev,
			      task->scaledUninterruptibleData,
			      UNINT_SHAPE, UNINT_SIZE, UNINT_COLOR);
}

void MainWindow::removeTaskGraph(int pid)
{
	Task *task = analyzer->findRealTask(pid);
	QCPGraph *qcpGraph;

	if (task == nullptr) {
		setTaskGraphClearActionEnabled(
			!taskRangeAllocator->isEmpty());
		return;
	}

	if (task->graph != nullptr) {
		qcpGraph = task->graph->getQCPGraph();
		if (qcpGraph != nullptr && qcpGraph->selected() &&
		    taskToolBar->getPid() == task->pid)
			taskToolBar->removeTaskGraph();
		task->graph->destroy();
		task->graph = nullptr;
	}

	if (task->delayGraph != nullptr) {
		tracePlot->removeGraph(task->delayGraph);
		task->delayGraph = nullptr;
	}

	task->horizontalDelayBars = nullptr;

	if (task->runningGraph != nullptr) {
		tracePlot->removeGraph(task->runningGraph);
		task->runningGraph = nullptr;
	}

	if (task->preemptedGraph != nullptr) {
		tracePlot->removeGraph(task->preemptedGraph);
		task->preemptedGraph = nullptr;
	}

	if (task->uninterruptibleGraph != nullptr) {
		tracePlot->removeGraph(task->uninterruptibleGraph);
		task->uninterruptibleGraph = nullptr;
	}

	taskRangeAllocator->putTaskRange(task->pid);
	bottom = taskRangeAllocator->getBottom();

	QCPRange range = tracePlot->yAxis->range();
	tracePlot->yAxis->setRange(QCPRange(bottom, range.upper));

	tracePlot->replot();
	updateTaskGraphActions();
}

void MainWindow::clearTaskGraphsTriggered()
{
	TaskRange r;
	int pid;
	Task *task;
	QCPGraph *qcpGraph;
	TaskRangeAllocator::iterator iter;

	for (iter = taskRangeAllocator->begin();
	     iter != taskRangeAllocator->end();
	     iter++) {
		r = iter.value();
		pid = r.pid;
		task = analyzer->findTask(pid);
		if (task == nullptr)
			continue;

		if (task->graph == nullptr)
			continue;

		qcpGraph = task->graph->getQCPGraph();
		if (qcpGraph != nullptr && qcpGraph->selected() &&
		    taskToolBar->getPid() == task->pid)
			taskToolBar->removeTaskGraph();
		task->graph->destroy();
		task->graph = nullptr;

		if (task->delayGraph != nullptr) {
			tracePlot->removeGraph(task->delayGraph);
			task->delayGraph = nullptr;
		}

		if (task->runningGraph != nullptr) {
			tracePlot->removeGraph(task->runningGraph);
			task->runningGraph = nullptr;
		}

		if (task->preemptedGraph != nullptr) {
			tracePlot->removeGraph(task->preemptedGraph);
			task->preemptedGraph = nullptr;
		}

		if (task->uninterruptibleGraph != nullptr) {
			tracePlot->removeGraph(task->uninterruptibleGraph);
			task->uninterruptibleGraph = nullptr;
		}
	}

	taskRangeAllocator->clearAll();
	bottom = taskRangeAllocator->getBottom();

	QCPRange range = tracePlot->yAxis->range();
	tracePlot->yAxis->setRange(QCPRange(bottom, range.upper));

	tracePlot->replot();
	updateTaskGraphActions();
}

void MainWindow::updateTaskGraphActions()
{
	setTaskGraphClearActionEnabled(!taskRangeAllocator->isEmpty());
	int spid = taskToolBar->getPid();
	if (spid != 0) {
		bool TaskGraph_selected = taskRangeAllocator->contains(spid);
		setTaskGraphRemovalActionEnabled(TaskGraph_selected);
		setAddTaskGraphActionEnabled(!TaskGraph_selected);
	} else {
		setTaskGraphRemovalActionEnabled(false);
		setAddTaskGraphActionEnabled(false);
	}
}

void MainWindow::updateAddToLegendAction()
{
	int pid = taskToolBar->getPid();
	if (pid == 0) {
		/* No task is selected */
		setAddToLegendActionEnabled(false);
		return;
	}
	setAddToLegendActionEnabled(!taskToolBar->legendContains(pid));
}

TaskGraph *MainWindow::selectedGraph()
{
	TaskGraph *graph = nullptr;
	QCPGraph *qcpGraph = nullptr;
	QCPAbstractPlottable *plottable;
	QList<QCPAbstractPlottable*> plist = tracePlot->selectedPlottables();
	QList<QCPAbstractPlottable*>::const_iterator iter;

	for (iter = plist.begin(); iter != plist.end(); iter++) {
		plottable = *iter;
		qcpGraph = qobject_cast<QCPGraph *>(plottable);
		if (qcpGraph == nullptr)
			continue;
		graph = TaskGraph::fromQCPGraph(qcpGraph);
		if (graph == nullptr)
			continue;
	}

	if (qcpGraph == nullptr || !qcpGraph->selected())
		return nullptr;
	return graph;
}

void MainWindow::showTaskSelector()
{
	if (taskSelectDialog->isVisible()) {
		taskSelectDialog->hide();
		return;
	}
	taskSelectDialog->show();
	if (dockWidgetArea(taskSelectDialog) == Qt::NoDockWidgetArea)
		addDockWidget(Qt::LeftDockWidgetArea, taskSelectDialog);

	if (dockWidgetArea(statsDialog) == Qt::LeftDockWidgetArea)
		tabifyDockWidget(statsDialog, taskSelectDialog);
	else if (dockWidgetArea(wakeupLatencyWidget) == Qt::LeftDockWidgetArea)
		tabifyDockWidget(wakeupLatencyWidget, taskSelectDialog);
}

void MainWindow::showSchedLatencyWidget()
{
	showLatencyWidget(schedLatencyWidget, Qt::RightDockWidgetArea);
}

void MainWindow::showWakeupLatencyWidget()
{
	showLatencyWidget(wakeupLatencyWidget, Qt::LeftDockWidgetArea);
}

void MainWindow::showLatencyWidget(LatencyWidget *lwidget,
				   Qt::DockWidgetArea area)
{
	if (lwidget->isVisible()) {
		lwidget->hide();
		return;
	}

	lwidget->show();

	if (dockWidgetArea(lwidget) == Qt::NoDockWidgetArea)
		addDockWidget(area, lwidget);

	if (area == Qt::RightDockWidgetArea) {
		if (dockWidgetArea(statsLimitedDialog)
		    == Qt::RightDockWidgetArea)
			tabifyDockWidget(statsLimitedDialog, lwidget);
	} else if (area ==  Qt::LeftDockWidgetArea) {
		if (dockWidgetArea(taskSelectDialog) == Qt::LeftDockWidgetArea)
			tabifyDockWidget(taskSelectDialog, lwidget);
		else if (dockWidgetArea(statsDialog) == Qt::LeftDockWidgetArea)
			tabifyDockWidget(statsDialog, lwidget);
	}
}

void MainWindow::filterOnCPUs()
{
	if (cpuSelectDialog->isVisible())
		cpuSelectDialog->hide();
	else
		cpuSelectDialog->show();
}

void MainWindow::showEventFilter()
{
	if (eventSelectDialog->isVisible())
		eventSelectDialog->hide();
	else
		eventSelectDialog->show();
}

void MainWindow::showArgFilter()
{
	if (regexDialog->isVisible())
		regexDialog->hide();
	else
		regexDialog->show();
}

void MainWindow::showGraphEnable()
{
	if (graphEnableDialog->isVisible())
		graphEnableDialog->hide();
	else
		graphEnableDialog->show();
}

void MainWindow::showStats()
{
	if (statsDialog->isVisible()) {
		statsDialog->hide();
		return;
	}
	statsDialog->show();
	if (dockWidgetArea(statsDialog) == Qt::NoDockWidgetArea)
		addDockWidget(Qt::LeftDockWidgetArea, statsDialog);

	if (dockWidgetArea(taskSelectDialog) == Qt::LeftDockWidgetArea)
		tabifyDockWidget(taskSelectDialog, statsDialog);
	else if (dockWidgetArea(wakeupLatencyWidget) == Qt::LeftDockWidgetArea)
		tabifyDockWidget(wakeupLatencyWidget, statsDialog);
}

void MainWindow::showStatsTimeLimited()
{
	if (statsLimitedDialog->isVisible()) {
		statsLimitedDialog->hide();
		return;
	}
	statsLimitedDialog->beginResetModel();
	analyzer->doLimitedStats();
	statsLimitedDialog->setTaskMap(&analyzer->taskMap,
				       analyzer->getNrCPUs());
	statsLimitedDialog->endResetModel();
	statsLimitedDialog->show();
	if (dockWidgetArea(statsLimitedDialog) == Qt::NoDockWidgetArea)
		addDockWidget(Qt::RightDockWidgetArea, statsLimitedDialog);

	if (dockWidgetArea(schedLatencyWidget) == Qt::RightDockWidgetArea)
		tabifyDockWidget(schedLatencyWidget, statsLimitedDialog);
}

void MainWindow::exportTasks(bool csv)
{
	exportStats_(csv, EXPORT_TASK_NAMES);
}

void MainWindow::exportStats(bool csv)
{
	exportStats_(csv, EXPORT_STATS);
}

void MainWindow::exportStatsTimeLimited(bool csv)
{
	exportStats_(csv, EXPORT_STATS_LIMITED);
}

void MainWindow::exportStats_(bool csv, taskexport_t exporttype)
{
	QString name;
	QString caption = tr("Export statistics");
	QString filter;
	int ts_errno = 0;
	QString selected;
	bool override_csv = csv;

	if (csv)
		filter = CSV_FILTER + F_SEP + TXT_FILTER;
	else
		filter = TXT_FILTER + F_SEP + CSV_FILTER;

	name = QFileDialog::getSaveFileName(this, caption, QString(),
					    filter, &selected, foptions);

	if (name.isEmpty())
		return;

	/*
	 * First check if the user has typed in a suffix, then check what
	 * format has been selected by the combo box in the dialog.
	 */
	if (name.endsWith(ASC_SUFFIX) || name.endsWith(TXT_SUFFIX))
		override_csv = false;
	else if (name.endsWith(CSV_SUFFIX))
		override_csv = true;
	else if (selected == CSV_FILTER) {
		override_csv = true;
		TShark::checkSuffix(&name, CSV_SUFFIX);
	} else if (selected == TXT_FILTER) {
		override_csv = false;
		TShark::checkSuffix(&name, TXT_SUFFIX);
	} else {
		if (csv)
			TShark::checkSuffix(&name, CSV_SUFFIX);
		else
			TShark::checkSuffix(&name, TXT_SUFFIX);
	}

	switch (exporttype) {
	case EXPORT_TASK_NAMES:
		ts_errno = taskSelectDialog->exportStats(override_csv, name);
		break;
	case EXPORT_STATS_LIMITED:
		ts_errno = statsLimitedDialog->exportStats(override_csv, name);
		break;
	case EXPORT_STATS:
		ts_errno = statsDialog->exportStats(override_csv, name);
		break;
	default:
		vtl::errx(BSD_EX_SOFTWARE,
			   "Unexcpected failure at %s:%d", __FILE__, __LINE__);
		break;
	}

	if (ts_errno != 0)
		vtl::warn(ts_errno, "Failed to export statistics to %s",
			  name.toLocal8Bit().data());
}

void MainWindow::removeQDockWidget(QDockWidget *widget)
{
	if (dockWidgetArea(widget) != Qt::NoDockWidgetArea)
		removeDockWidget(widget);
}

void MainWindow::showWakeupOrWaking(int pid, event_t wakevent)
{
	int activeIdx = infoWidget->getCursorIdx();
	int inactiveIdx;
	int wakeUpIndex;
	int schedIndex;

	if (activeIdx != TShark::RED_CURSOR &&
	    activeIdx != TShark::BLUE_CURSOR) {
		oops_warnx();
		return;
	}

	inactiveIdx = TShark::RED_CURSOR;
	if (activeIdx == inactiveIdx)
		inactiveIdx = TShark::BLUE_CURSOR;

	Cursor *activeCursor = cursors[activeIdx];
	Cursor *inactiveCursor = cursors[inactiveIdx];

	if (activeCursor == nullptr || inactiveCursor == nullptr) {
		oops_warnx();
		return;
	}

	/*
	 * The time of the active cursor is taken to be the time that the
	 * user is interested in, i.e. finding the previous wake up event
	 * relative to
	 */
	double zerotime = activeCursor->getPosition();
	const TraceEvent *schedevent =
		analyzer->findPreviousSchedEvent(
			vtl::Time::fromDouble(zerotime), pid, &schedIndex);
	if (schedevent == nullptr)
		return;

	const TraceEvent *wakeupevent = analyzer->
		findPreviousWakEvent(schedIndex, pid, wakevent, &wakeUpIndex);
	if (wakeupevent == nullptr)
		return;
	/*
	 * This is what we do, we move the *active* cursor to the wakeup
	 * event, move the *inactive* cursor to the scheduling event and then
	 * finally scroll the events widget to the same time and highlight
	 * the task that was doing the wakeup. This way we can push the button
	 * again to see who woke up the task that was doing the wakeup
	 */
	activeCursor->setPosition(wakeupevent->time);
	inactiveCursor->setPosition(schedevent->time);
	checkStatsTimeLimited();
	infoWidget->setTime(wakeupevent->time, activeIdx);
	infoWidget->setTime(schedevent->time, inactiveIdx);
	cursorPos[activeIdx] = wakeupevent->time.toDouble();
	cursorPos[inactiveIdx] = schedevent->time.toDouble();

	if (!analyzer->isFiltered()) {
		eventsWidget->scrollTo(wakeUpIndex);
	} else {
		/*
		 * If a filter is enabled we need to try to find the index in
		 * analyzer->filteredEvents
		 */
		int filterIndex;
		if (analyzer->findFilteredEvent(wakeUpIndex, &filterIndex)
		    != nullptr)
			eventsWidget->scrollTo(filterIndex);
	}

	unsigned int wcpu = wakeupevent->cpu;
	int wpid = wakeupevent->pid;

	selectTaskByPid(wpid, &wcpu, PR_TRY_TASKGRAPH);
}

void MainWindow::showWaking(const TraceEvent *wakeupevent)
{
	int activeIdx = infoWidget->getCursorIdx();
	int wakingIndex;

	if (activeIdx != TShark::RED_CURSOR &&
	    activeIdx != TShark::BLUE_CURSOR) {
		return;
	}

	Cursor *activeCursor = cursors[activeIdx];

	if (activeCursor == nullptr)
		return;

	const TraceEvent *wakingevent =
		analyzer->findWakingEvent(wakeupevent, &wakingIndex);
	if (wakingevent == nullptr)
		return;

	activeCursor->setPosition(wakingevent->time);
	infoWidget->setTime(wakingevent->time, activeIdx);
	checkStatsTimeLimited();
	cursorPos[activeIdx] = wakingevent->time.toDouble();

	if (!analyzer->isFiltered()) {
		eventsWidget->scrollTo(wakingIndex);
	} else {
		/*
		 * If a filter is enabled we need to try to find the index in
		 * analyzer->filteredEvents
		 */
		int filterIndex;
		if (analyzer->findFilteredEvent(wakingIndex, &filterIndex)
		    != nullptr)
			eventsWidget->scrollTo(filterIndex);
	}

	unsigned int wcpu = wakingevent->cpu;
	int wpid = wakingevent->pid;

	selectTaskByPid(wpid, &wcpu, PR_TRY_TASKGRAPH);
}

void MainWindow::checkStatsTimeLimited()
{
	if (statsLimitedDialog->isVisible()) {
		statsLimitedDialog->beginResetModel();
		analyzer->doLimitedStats();
		statsLimitedDialog->setTaskMap(&analyzer->taskMap,
					       analyzer->getNrCPUs());
		statsLimitedDialog->endResetModel();
	}
}

bool MainWindow::selectQCPGraph(QCPGraph *graph)
{
	int end = graph->dataCount() - 1;
	if (end < 0)
		return false;
	QCPDataRange wholeRange(0,  end);
	QCPDataSelection wholeSelection(wholeRange);
	graph->setSelection(std::move(wholeSelection));
	return true;
}

/* Add a unified task graph for the currently selected task */
void MainWindow::addTaskGraphTriggered()
{
	addTaskGraph(taskToolBar->getPid());
	doReplot();
}

void MainWindow::selectTaskByPid(int pid, const unsigned int *preferred_cpu,
				 preference_t preference)
{
	Task *task;
	int realpid;
	QCPGraph *qcpGraph;
	CPUTask *cpuTask;
	TaskGraph *graph = nullptr;
	unsigned int cpu;
	int maxSize;
	CPUTask *maxTask;

	/* Deselect the selected task */
	tracePlot->deselectAll();

	/*
	 * If the task to be selected is pid 0, that is swapper, or negative,
	 * that is those negative pids that sometimes appears as the pid of
	 * sched_switch events, then remove the task from the task toolbar and
	 * disable the task actions.
	 */
	if (pid <= 0)
		goto out;

	task = analyzer->findRealTask(pid);

	/* task is always supposed to be != nullptr, so display warning */
	if (task == nullptr) {
		oops_warnx();
		goto out;
	}

	/*
	 * task->pid may be different from pid. Look at what findRealTask() does
	 * if you are confused.
	 */
	realpid = task->pid;

	if (preference == PR_CPUGRAPH_ONLY)
		goto do_cpugraph;

	if (task->graph == nullptr)
		goto do_cpugraph;
	qcpGraph = task->graph->getQCPGraph();
	if (qcpGraph == nullptr)
		goto do_cpugraph;
	selectQCPGraph(qcpGraph);
	graph = task->graph;
	goto out;

do_cpugraph:

	/*
	 * If no preference is given, we will selected the CPU graph with the
	 * highest number of scheduling events.
	 */
	if (preferred_cpu == nullptr) {
		maxTask = nullptr;
		maxSize = -1;
		for (cpu = 0; cpu < analyzer->getNrCPUs(); cpu++) {
			cpuTask = analyzer->findCPUTask(realpid, cpu);
			if (cpuTask != nullptr) {
				if (cpuTask->schedTimev.size() > maxSize) {
					maxSize = cpuTask->schedTimev.size();
					maxTask = cpuTask;
				}
			}
		}
		cpuTask = maxTask;
	} else {
		cpuTask = analyzer->findCPUTask(realpid, *preferred_cpu);
	}
	/*
	 * If we can't find what we expected we give up but don't warn the
	 * user. There is probably yet another case of tasks that has a global
	 * task but no per CPU task.
	 */
	if (cpuTask == nullptr || cpuTask->graph == nullptr)
		goto out;
	qcpGraph = cpuTask->graph->getQCPGraph();
	/*
	 * I would still expect all per CPU tasks that exists to have a QCP
	 * graph, so in this case we warn the user
	 */
	if (qcpGraph == nullptr) {
		oops_warnx();
		goto out;
	}

	selectQCPGraph(qcpGraph);

	/* Finally update the TaskToolBar to reflect the change in selection */
	graph = TaskGraph::fromQCPGraph(qcpGraph);
	if (graph == nullptr)
		oops_warnx();

out:
	if (graph != nullptr) {
		taskToolBar->setTaskGraph(graph);
		setTaskActionsEnabled(true);
	} else {
		taskToolBar->removeTaskGraph();
		setTaskActionsEnabled(false);
	}
	updateTaskGraphActions();
	updateAddToLegendAction();
	tracePlot->replot();
}

bool MainWindow::isOpenGLEnabled()
{
	if (has_opengl())
		return tracePlot->openGl();
	else
		return false;
}

void MainWindow::setupOpenGL()
{
	bool opengl = settingStore->getValue(Setting::OPENGL_ENABLED).boolv();

	if (has_opengl() && opengl) {
		if (!isOpenGLEnabled()) {
			tracePlot->setOpenGl(true, 4);
			if (!tracePlot->openGl()) {
				qcp_warn_failed_opengl_enable();
			}
		}
	} else {
		if (isOpenGLEnabled()) {
			tracePlot->setOpenGl(false, 4);
			if (tracePlot->openGl()) {
				qcp_warn_failed_opengl_disable();
			}
		}
	}
	if (opengl != isOpenGLEnabled()) {
		settingStore->setBoolValue(Setting::OPENGL_ENABLED,
					   isOpenGLEnabled());
		settingStore->updateDependents(Setting::OPENGL_ENABLED);
	}
}

/* Adds the currently selected task to the legend */
void MainWindow::addToLegendTriggered()
{
	taskToolBar->addCurrentTaskToLegend();
	doReplot();
	updateAddToLegendAction();
}

/* Clears the legend of all tasks */
void MainWindow::clearLegendTriggered()
{
	taskToolBar->clearLegend();
	updateAddToLegendAction();
}

/* let's the user chose a color for the toolbar task */
void MainWindow::colorTask(int pid)
{
	const QColorDialog::ColorDialogOptions options;
	const QColor oldcolor = analyzer->getTaskColor(pid);

	Task *task = analyzer->findTask(pid);
	if (task == nullptr || task->isGhostAlias)
		return;

	const QString title = tr("New Color for task: ") +
		QString(task->taskName->str) +
		QLatin1String(":") +
		QString::number(task->pid);

	const QColor color = QColorDialog::getColor(oldcolor, this, title, options);

	if (!color.isValid())
		return;

	stateFile->setTaskColor(pid, color);
	analyzer->setTaskColor(pid, color);

	setGraphColor(pid, color);
	tracePlot->replot();

	setResetTaskColorEnabled(true);
}

void MainWindow::setGraphColor(int pid, const QColor &color)
{
	const unsigned int nrCPUs = analyzer->getNrCPUs();
	unsigned int cpu;
	Task *task = analyzer->findTask(pid);
	if (task == nullptr || task->isGhostAlias)
		return;

	QPen pen = QPen();
	pen.setColor(color);
	pen.setWidth(settingStore->getValue(Setting::LINE_WIDTH).intv());

	/* Add code here for coloring a unified graph */
	if (task->graph != nullptr)
		task->graph->setPen(pen);
	if (task->horizontalDelayBars != nullptr)
		task->horizontalDelayBars->setPen(pen);

	for (cpu = 0; cpu < nrCPUs; cpu++) {
		DEFINE_CPUTASKMAP_ITERATOR(iter) = analyzer->
			cpuTaskMaps[cpu].find(pid);

		if (iter == analyzer->cpuTaskMaps[cpu].end())
			continue;

		CPUTask &cputask = iter.value();

		/* Add code here for coloring per cpu graph */
		if (cputask.verticalDelayBars != nullptr)
			cputask.verticalDelayBars->setPen(pen);
		if (cputask.horizontalDelayBars != nullptr)
			cputask.horizontalDelayBars->setPen(pen);

		if (cputask.graph != nullptr)
			cputask.graph->setPen(pen);
	}
}

/* let's the user chose a color for the toolbar task */
void MainWindow::colorToolbarTaskTriggered()
{
	const int pid = taskToolBar->getPid();

	colorTask(pid);
}

void MainWindow::colorTasks(const QList<int> &pids)
{
	QList<int>::const_iterator iter;

	for (iter = pids.constBegin(); iter != pids.constEnd(); iter++)
		colorTask(*iter);
}

void MainWindow::changeColors(const QList<int> *pids)
{
	colorTasks(*pids);
}

void MainWindow::resetTaskColors()
{
	QList<int> pids;
	QList<QColor> colors;
	int s1, s2, s, i;

	analyzer->getOrigTaskColors(pids, colors);
	setResetTaskColorEnabled(false);

	s1 = pids.size();
	s2 = colors.size();
	/* s1 and s2 should be equal but test anyway */
	s = TSMIN(s1, s2);

	for (i = 0; i < s; i++) {
		int pid = pids[i];
		QColor color = colors[i];
		setGraphColor(pid, color);
	}
	tracePlot->replot();
	analyzer->resetTaskColors();
	stateFile->resetColors();
}

/* Finds the preceding wakeup of the currently selected task */
void MainWindow::findWakeupTriggered()
{
	showWakeupOrWaking(taskToolBar->getPid(), SCHED_WAKEUP);
}

/* Finds the preceding waking of the currently selected wakeup event */
void MainWindow::findWakingTriggered()
{
	const TraceEvent *event = eventsWidget->getSelectedEvent();
	if (event != nullptr &&
	    (event->type == SCHED_WAKEUP || event->type == SCHED_WAKEUP_NEW))
		showWaking(event);
}

/* Finds the preceding waking of the currently selected task */
void MainWindow::findWakingDirectTriggered()
{
	showWakeupOrWaking(taskToolBar->getPid(), SCHED_WAKING);
}

/* Finds the next sched_switch event that puts the task to sleep */
void MainWindow::findSleepTriggered()
{
	int activeIdx = infoWidget->getCursorIdx();
	int pid = taskToolBar->getPid();
	int schedIndex;

	if (pid == 0)
		return;
	if (activeIdx != TShark::RED_CURSOR &&
	    activeIdx != TShark::BLUE_CURSOR) {
		return;
	}

	Cursor *activeCursor = cursors[activeIdx];

	if (activeCursor == nullptr)
		return;

	/*
	 * The time of the active cursor is taken to be the time that the
	 * user is interested in, i.e. finding the subsequent sched_swith event
	 * relative to
	 */
	double zerotime = activeCursor->getPosition();
	const TraceEvent *schedevent = analyzer->findNextSchedSleepEvent(
		vtl::Time::fromDouble(zerotime), pid, &schedIndex);

	if (schedevent == nullptr)
		return;

	activeCursor->setPosition(schedevent->time);
	checkStatsTimeLimited();
	infoWidget->setTime(schedevent->time, activeIdx);
	cursorPos[activeIdx] = schedevent->time.toDouble();

	if (!analyzer->isFiltered()) {
		eventsWidget->scrollTo(schedIndex);
	} else {
		/*
		 * If a filter is enabled we need to try to find the index in
		 * analyzer->filteredEvents
		 */
		int filterIndex;
		if (analyzer->findFilteredEvent(schedIndex, &filterIndex)
		    != nullptr)
			eventsWidget->scrollTo(filterIndex);
	}
}

/* Removes the task graph of the currently selected task */
void MainWindow::removeTaskGraphTriggered()
{
	removeTaskGraph(taskToolBar->getPid());
}

/* Filter on the currently selected task */
void MainWindow::taskFilter()
{
	vtl::Time saved = eventsWidget->getSavedScroll();
	int pid = taskToolBar->getPid();

	if (pid == 0)
		return;

	QMap<int, int> map;
	map[pid] = pid;

	eventsWidget->beginResetModel();
	analyzer->createPidFilter(map, false, true);
	setEventsWidgetEvents();
	eventsWidget->endResetModel();
	scrollTo(saved);
	updateResetFiltersEnabled();
}

/* Filter on the currently selected task */
void MainWindow::taskFilterTriggered()
{
	taskFilter();
}

/* Filter on the currently selected task */
void MainWindow::taskFilterLimitedTriggered()
{
	timeFilter();
	taskFilter();
}

void MainWindow::showBackTraceTriggered()
{
	const TraceEvent *event = eventsWidget->getSelectedEvent();

	if (event != nullptr)
		eventInfoDialog->show(*event, *analyzer->getTraceFile());
}

void MainWindow::eventCPUTriggered()
{
	const TraceEvent *event = eventsWidget->getSelectedEvent();

	if (event != nullptr)
		createEventCPUFilter(*event);
}

void MainWindow::eventTypeTriggered()
{
	const TraceEvent *event = eventsWidget->getSelectedEvent();

	if (event != nullptr)
		createEventTypeFilter(*event);
}

void MainWindow::eventPIDTriggered()
{
	const TraceEvent *event = eventsWidget->getSelectedEvent();

	if (event != nullptr)
		createEventPIDFilter(*event);
}

void MainWindow::eventMoveBlueTriggered()
{
	const TraceEvent *event = eventsWidget->getSelectedEvent();

	if (event != nullptr) {
		moveCursor(event->time, TShark::BLUE_CURSOR);
	}
}

void MainWindow::eventMoveRedTriggered()
{
	const TraceEvent *event = eventsWidget->getSelectedEvent();

	if (event != nullptr) {
		moveCursor(event->time, TShark::RED_CURSOR);
	}
}
