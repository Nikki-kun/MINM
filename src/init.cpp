#include <QApplication>
#include <QMainWindow>
#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QWidget>
#include <QFont>
#include <QLineEdit>
#include <QPushButton>
#include <QMessageBox>
#include <QSettings>
#include <QScreen>
#include <QStyle>
#include <QIcon>
#include <QDebug>
#include "managers/auth_manager.h"

const QString SETTINGS_KEY_THEME = "ui/theme";
const QString THEME_DARK = "dark";
const QString THEME_LIGHT = "light";

const QString COLOR_DARK_BG = "#222222";
const QString COLOR_DARK_FIELD_BG = "#333333";
const QString COLOR_DARK_TEXT = "white";
const QString COLOR_DARK_SUBTEXT = "#aaaaaa";
const QString COLOR_DARK_BORDER = "#444444";
const QString COLOR_ACCENT_GREEN = "#6DB33F";

const QString COLOR_LIGHT_BG = "#f5f5f5";
const QString COLOR_LIGHT_FIELD_BG = "white";
const QString COLOR_LIGHT_TEXT = "#222222";
const QString COLOR_LIGHT_SUBTEXT = "#555555";
const QString COLOR_LIGHT_BORDER = "#e1e1e1";

AuthManager* g_authManager = nullptr;

void applyTheme(QMainWindow &window, const QString &theme) {
    QString mainBgColor;
    QString fieldBgColor;
    QString textColor;
    QString subtextColor;
    QString fieldBorderColor;
    
    if (theme == THEME_DARK) {
        mainBgColor = COLOR_DARK_BG;
        fieldBgColor = COLOR_DARK_FIELD_BG;
        textColor = COLOR_DARK_TEXT;
        subtextColor = COLOR_DARK_SUBTEXT;
        fieldBorderColor = COLOR_DARK_BORDER;
    } else {
        mainBgColor = COLOR_LIGHT_BG;
        fieldBgColor = COLOR_LIGHT_FIELD_BG;
        textColor = COLOR_LIGHT_TEXT;
        subtextColor = COLOR_LIGHT_SUBTEXT;
        fieldBorderColor = COLOR_LIGHT_BORDER;
    }

    window.setStyleSheet(
        "QMainWindow {"
        "    background-color: " + mainBgColor + ";"
        "    border: 1px solid #000000;"
        "}"
    );

    for (QLabel *label : window.centralWidget()->findChildren<QLabel*>()) {
        if (label->font().pointSize() >= 30) {
             label->setStyleSheet("color: " + textColor + ";");
        } else {
             label->setStyleSheet("color: " + subtextColor + ";");
        }
    }
    
    for (QLineEdit *edit : window.centralWidget()->findChildren<QLineEdit*>()) {
        edit->setStyleSheet(
            "QLineEdit {"
            "    padding: 15px;"
            "    border: 1px solid " + fieldBorderColor + ";"
            "    border-radius: 4px;"
            "    font-size: 16px;"
            "    background: " + fieldBgColor + ";"
            "    color: " + textColor + ";"
            "}"
            "QLineEdit:focus {"
            "    border-color: " + COLOR_ACCENT_GREEN + ";"
            "    background: " + (theme == THEME_DARK ? "#3a3a3a" : "#ffffff") + ";"
            "}"
        );
    }
    
    if (QPushButton *loginButton = window.centralWidget()->findChild<QPushButton*>("loginButton")) {
        loginButton->setStyleSheet(
            "QPushButton {"
            "    background: " + COLOR_ACCENT_GREEN + ";"
            "    color: " + (theme == THEME_DARK ? COLOR_DARK_BG : COLOR_LIGHT_BG) + ";"
            "    padding: 15px;"
            "    border: none;"
            "    border-radius: 4px;"
            "    font-size: 16px;"
            "    font-weight: bold;"
            "    letter-spacing: 1px;"
            "}"
            "QPushButton:hover {"
            "    background: #5aa133;"
            "}"
            "QPushButton:pressed {"
            "    background: #4a8429;"
            "}"
            "QPushButton:disabled {"
            "    background: #cccccc;"
            "    color: #666666;"
            "}"
        );
    }
    
    if (QPushButton *registerButton = window.centralWidget()->findChild<QPushButton*>("registerButton")) {
        registerButton->setStyleSheet(
            "QPushButton {"
            "    background: transparent;"
            "    color: " + COLOR_ACCENT_GREEN + ";"
            "    border: none;"
            "    font-weight: bold;"
            "    text-decoration: none;"
            "}"
            "QPushButton:hover {"
            "    color: #89C768;"
            "    background: transparent;"
            "    text-decoration: underline;"
            "}"
        );
    }
    
    if (QWidget *separator = window.centralWidget()->findChild<QWidget*>("separatorWidget")) {
        separator->setStyleSheet(QString("background: rgba(255,255,255,") + (theme == THEME_DARK ? "0.2" : "0.5") + ");");
    }
}

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    
    QCoreApplication::setOrganizationName("MINMDev");
    QCoreApplication::setApplicationName("MINMMessenger");
    
    QSettings settings;
    
    QString currentTheme = settings.value(SETTINGS_KEY_THEME, THEME_DARK).toString();
    
    AuthManager authManager("http://localhost:8080/api");
    g_authManager = &authManager;
    
    QMainWindow window;
    window.setWindowTitle("MINM Messenger - Вход");
    
    const QRect screenGeometry = QGuiApplication::primaryScreen()->geometry();
    window.setGeometry(QStyle::alignedRect(
        Qt::LeftToRight,
        Qt::AlignCenter,
        window.size(),
        screenGeometry
    ));
    
    QWidget *centralWidget = new QWidget();
    window.setCentralWidget(centralWidget);
    
    QVBoxLayout *mainLayout = new QVBoxLayout(centralWidget);
    mainLayout->setSpacing(20);
    mainLayout->setContentsMargins(40, 40, 40, 30);
    
    QHBoxLayout *headerLayout = new QHBoxLayout();
    
    QLabel *titleLabel = new QLabel("MINM");
    titleLabel->setAlignment(Qt::AlignCenter);
    QFont titleFont("Arial", 36, QFont::ExtraBold);
    titleLabel->setFont(titleFont);
    
    headerLayout->addStretch();
    headerLayout->addWidget(titleLabel);
    headerLayout->addStretch();
    
    QLabel *subtitleLabel = new QLabel("Добро пожаловать");
    subtitleLabel->setAlignment(Qt::AlignCenter);
    QFont subtitleFont("Arial", 14);
    subtitleLabel->setFont(subtitleFont);
    
    QLineEdit *loginEdit = new QLineEdit();
    loginEdit->setPlaceholderText("Логин");
    loginEdit->setObjectName("loginEdit");
    
    QLineEdit *passwordEdit = new QLineEdit();
    passwordEdit->setPlaceholderText("Пароль");
    passwordEdit->setEchoMode(QLineEdit::Password);
    passwordEdit->setObjectName("passwordEdit");
    
    QPushButton *loginButton = new QPushButton("Войти");
    loginButton->setObjectName("loginButton");
    
    QWidget *separator = new QWidget();
    separator->setFixedHeight(1);
    separator->setObjectName("separatorWidget");
    
    QHBoxLayout *registerLayout = new QHBoxLayout();
    QLabel *registerLabel = new QLabel("Нет аккаунта?");
    
    QPushButton *registerButton = new QPushButton("Зарегистрироваться");
    registerButton->setObjectName("registerButton");
    
    registerLayout->addStretch();
    registerLayout->addWidget(registerLabel);
    registerLayout->addWidget(registerButton);
    registerLayout->addStretch();
    
    mainLayout->addSpacing(20);
    mainLayout->addLayout(headerLayout);
    mainLayout->addWidget(subtitleLabel);
    mainLayout->addSpacing(40);
    mainLayout->addWidget(loginEdit);
    mainLayout->addWidget(passwordEdit);
    mainLayout->addSpacing(30);
    mainLayout->addWidget(loginButton);
    mainLayout->addSpacing(30);
    mainLayout->addWidget(separator);
    mainLayout->addSpacing(20);
    mainLayout->addLayout(registerLayout);
    mainLayout->addStretch();
    
    QObject::connect(&authManager, &AuthManager::loginSuccess, [&](User* user) {
        loginButton->setEnabled(true);
        QMessageBox::information(&window, "Успех", 
            QString("Вход выполнен!\nДобро пожаловать, %1").arg(QString::fromStdString(user->getUserName())));
        
        qDebug() << "Пользователь вошел:" << user->getId();
    });
    
    QObject::connect(&authManager, &AuthManager::loginFailed, [&](const QString& error) {
        loginButton->setEnabled(true);
        QMessageBox::warning(&window, "Ошибка входа", error);
    });
    
    QObject::connect(&authManager, &AuthManager::registrationSuccess, [&](User* user) {
        QMessageBox::information(&window, "Успех", 
            QString("Регистрация выполнена!\nДобро пожаловать, %1").arg(QString::fromStdString(user->getUserName())));
    });
    
    QObject::connect(&authManager, &AuthManager::registrationFailed, [&](const QString& error) {
        QMessageBox::warning(&window, "Ошибка регистрации", error);
    });
    
    QObject::connect(loginButton, &QPushButton::clicked, [&]() {
        QString login = loginEdit->text();
        QString password = passwordEdit->text();
        
        if (login.isEmpty() || password.isEmpty()) {
            QMessageBox::warning(&window, "Ошибка", "Заполните все поля");
        } else {
            loginButton->setEnabled(false);
            loginButton->setText("Вход...");
            
            authManager.login(login.toStdString(), password.toStdString());
        }
    });
    
    QObject::connect(registerButton, &QPushButton::clicked, [&]() {
        QString login = loginEdit->text();
        QString password = passwordEdit->text();
        
        if (login.isEmpty() || password.isEmpty()) {
            QMessageBox::warning(&window, "Ошибка", "Заполните все поля");
        } else {
            authManager.registerUser(login.toStdString(), password.toStdString());
        }
    });
    
    QObject::connect(loginEdit, &QLineEdit::returnPressed, [&]() {
        passwordEdit->setFocus();
    });
    
    QObject::connect(passwordEdit, &QLineEdit::returnPressed, [&]() {
        loginButton->click();
    });

    applyTheme(window, currentTheme);
    
    window.show();
    
    int result = app.exec();
    
    g_authManager = nullptr;
    
    return result;
}