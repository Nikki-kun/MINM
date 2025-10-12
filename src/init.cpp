#include <QApplication>
#include <QMainWindow>
#include <QLabel>
#include <QVBoxLayout>
#include <QWidget>
#include <QFont>
#include <QPalette>
#include <QLinearGradient>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    QMainWindow mainWindow;
    mainWindow.setWindowTitle("My Beautiful Qt Application");
    mainWindow.resize(800, 600);
    
    mainWindow.setStyleSheet("QMainWindow { background: qlineargradient(x1:0, y1:0, x2:1, y2:1, stop:0 #667eea, stop:1 #764ba2); }");

    QWidget *centralWidget = new QWidget(&mainWindow);
    centralWidget->setStyleSheet("QWidget { background: rgba(255,255,255,0.9);}");
    
    QVBoxLayout *layout = new QVBoxLayout(centralWidget);
    layout->setContentsMargins(50, 50, 50, 50);
    layout->setSpacing(30);

    QLabel *mainLabel = new QLabel("Hello, Qt!", centralWidget);
    mainLabel->setAlignment(Qt::AlignCenter);
    mainLabel->setStyleSheet(
        "QLabel {"
        "    color: #2c3e50;"
        "    font-size: 48px;"
        "    font-weight: bold;"
        "    padding: 20px;"
		"    border-radius: 10px;"
        "    background: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 #3498db, stop:1 #2ecc71);"
        "    color: white;"
        "}"
    );

    QLabel *infoLabel = new QLabel("✨ Built with Qt Framework\n🎨 Beautiful design\n🚀 High performance", centralWidget);
    infoLabel->setAlignment(Qt::AlignCenter);
    infoLabel->setStyleSheet(
        "QLabel {"
        "    color: #16a085;"
        "    font-size: 16px;"
        "    font-weight: bold;"
        "    padding: 20px;"
        "    background-color: #d5f4e6;"
        "    border-radius: 12px;"
        "    border-left: 5px solid #1abc9c;"
        "}"
    );

    layout->addWidget(mainLabel);
    layout->addWidget(infoLabel);
    layout->addStretch();

    mainWindow.setCentralWidget(centralWidget);
    mainWindow.show();

    return app.exec();
}