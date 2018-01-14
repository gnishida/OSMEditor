#include "MainWindow.h"
#include <QFileDialog>

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent) {
	ui.setupUi(this);

	setCentralWidget(&canvas);

	connect(ui.actionOpen, SIGNAL(triggered()), this, SLOT(onOpen()));
	connect(ui.actionExit, SIGNAL(triggered()), this, SLOT(close()));
}

MainWindow::~MainWindow() {
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