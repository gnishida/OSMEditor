#include "MainWindow.h"
#include <QFileDialog>

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent) {
	ui.setupUi(this);

	setCentralWidget(&canvas);

	connect(ui.actionOpen, SIGNAL(triggered()), this, SLOT(onOpen()));
	connect(ui.actionSave, SIGNAL(triggered()), this, SLOT(onSave()));
	connect(ui.actionExit, SIGNAL(triggered()), this, SLOT(close()));
	connect(ui.actionUndo, SIGNAL(triggered()), this, SLOT(onUndo()));
	connect(ui.actionDeleteEdge, SIGNAL(triggered()), this, SLOT(onDeleteEdge()));

	// create tool bar for file menu
	ui.mainToolBar->addAction(ui.actionOpen);
	ui.mainToolBar->addAction(ui.actionSave);
	ui.mainToolBar->addSeparator();

	// create tool bar for edit menu
	ui.mainToolBar->addAction(ui.actionUndo);
	ui.mainToolBar->addAction(ui.actionDeleteEdge);
	//ui.mainToolBar->addSeparator();
}

MainWindow::~MainWindow() {
}

void MainWindow::keyPressEvent(QKeyEvent* e) {
	canvas.keyPressEvent(e);
}

void MainWindow::keyReleaseEvent(QKeyEvent* e) {
	canvas.keyReleaseEvent(e);
}

void MainWindow::onOpen() {
	QString filename = QFileDialog::getOpenFileName(this, tr("Open StreetMap file..."), "", tr("StreetMap Files (*.osm)"));

	if (filename.isEmpty()) {
		return;
	}

	QApplication::setOverrideCursor(Qt::WaitCursor);
	canvas.open(filename);
	canvas.update();
	QApplication::restoreOverrideCursor();

	setWindowTitle("OSM Editor - " + filename);
}

void MainWindow::onSave() {
	QString filename = QFileDialog::getSaveFileName(this, tr("Open StreetMap file..."), "", tr("StreetMap Files (*.osm)"));

	if (filename.isEmpty()) {
		return;
	}

	QApplication::setOverrideCursor(Qt::WaitCursor);
	canvas.save(filename);
	canvas.update();
	QApplication::restoreOverrideCursor();

	setWindowTitle("OSM Editor - " + filename);
}

void MainWindow::onUndo() {
	canvas.undo();
}

void MainWindow::onDeleteEdge() {
	canvas.deleteEdge();
}