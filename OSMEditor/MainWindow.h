#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QtWidgets/QMainWindow>
#include "ui_MainWindow.h"
#include "Canvas.h"
#include "PropertyWidget.h"

class MainWindow : public QMainWindow {
	Q_OBJECT

public:
	Ui::MainWindowClass ui;
	Canvas* canvas;
	PropertyWidget* propertyWidget;

public:
	MainWindow(QWidget *parent = 0);
	~MainWindow();

protected:
	void keyPressEvent(QKeyEvent* e);
	void keyReleaseEvent(QKeyEvent* e);

public slots:
	void onOpen();
	void onSave();
	void onUndo();
	void onRedo();
	void onDeleteEdge();
	void onPlanarGraph();
	void onPropertyWindow();
};

#endif // MAINWINDOW_H
