#pragma once

#include "ui_GroupWidget.h"
#include "Scene.h"

class GroupPanel : public QWidget{
	Q_OBJECT

public:
	GroupPanel(QWidget * parent = NULL);

	Ui::GroupManagmentWidget groupWidget;
	Scene * activeScene;

	// Types
	QVector<QString> groupTypes;

	// Utility function
	QString getItemId(QTreeWidgetItem* item);


public slots:
	void setActiveScene( Scene * newScene );
	void updateWidget();
	void removeSelectedItem();
	void saveGroups();
	void loadGroups();
	void clearGroups();

signals:

};
