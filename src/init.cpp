#include <QApplication>
#include <QMainWindow>
#include <QLabel>
#include <QVBoxLayout>
#include <QWidget>

int main(int argc, char *argv[])
{
    	QApplication app(argc, argv);

    	QMainWindow mainWindow;
    	mainWindow.setWindowTitle("My Qt Application");
    	mainWindow.resize(800, 600);

    	QWidget *centralWidget = new QWidget(&mainWindow);
	QVBoxLayout *layout = new QVBoxLayout(centralWidget);

	QLabel *label = new QLabel("Hello, Qt!", centralWidget);
	label->setAlignment(Qt::AlignCenter);
	layout->addWidget(label);

    	mainWindow.setCentralWidget(centralWidget);
    	mainWindow.show();

	return app.exec();
}
