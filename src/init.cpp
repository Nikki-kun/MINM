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
#include <QListWidget>
#include <QStackedWidget>
#include <QScrollArea>
#include <QPainter>
#include <QPainterPath>
#include <QMouseEvent>
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

class AvatarWidget : public QWidget {
public:
    AvatarWidget(const QString& text, QWidget* parent = nullptr) 
        : QWidget(parent), displayText(text) {
        setFixedSize(60, 60);
    }

protected:
    void paintEvent(QPaintEvent* event) override {
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing);
        
        painter.setBrush(QColor(COLOR_ACCENT_GREEN));
        painter.setPen(Qt::NoPen);
        painter.drawEllipse(rect());
        
        painter.setPen(QColor("white"));
        painter.setFont(QFont("Arial", 20, QFont::Bold));
        painter.drawText(rect(), Qt::AlignCenter, displayText.left(1).toUpper());
        
        QWidget::paintEvent(event);
    }

private:
    QString displayText;
};

class MainAppWindow : public QMainWindow {
    Q_OBJECT

public:
    MainAppWindow(User* user, QWidget *parent = nullptr) 
        : QMainWindow(parent), currentUser(user) {
        setupUI();
        applyTheme(THEME_DARK);
    }

public slots:
    void onLogoutClicked() {
        if (g_authManager) {
            g_authManager->logout(currentUser->getId());
        }
        this->deleteLater();
        emit logoutRequested();
    }

signals:
    void logoutRequested();

private:
    void setupUI() {
        setWindowTitle("MINM Messenger - Главная");
        setMinimumSize(1000, 600);
        
        QWidget *centralWidget = new QWidget();
        setCentralWidget(centralWidget);
        
        QHBoxLayout *mainLayout = new QHBoxLayout(centralWidget);
        mainLayout->setSpacing(0);
        mainLayout->setContentsMargins(0, 0, 0, 0);
        
        setupSidebar(mainLayout);
        setupMainContent(mainLayout);
    }
    
    void setupSidebar(QHBoxLayout *mainLayout) {
        QWidget *sidebar = new QWidget();
        sidebar->setFixedWidth(250);
        sidebar->setObjectName("sidebar");
        
        QVBoxLayout *sidebarLayout = new QVBoxLayout(sidebar);
        sidebarLayout->setSpacing(0);
        sidebarLayout->setContentsMargins(0, 0, 0, 0);
        
        QWidget *userWidget = new QWidget();
        userWidget->setFixedHeight(100);
        QHBoxLayout *userLayout = new QHBoxLayout(userWidget);
        userLayout->setContentsMargins(15, 0, 15, 0);
        
        AvatarWidget *avatar = new AvatarWidget(currentUser->getUserName());
        
        QVBoxLayout *userInfoLayout = new QVBoxLayout();
        QLabel *userNameLabel = new QLabel(currentUser->getUserName());
        userNameLabel->setStyleSheet("font-weight: bold; font-size: 14px;");
        QLabel *statusLabel = new QLabel(currentUser->isOnline() ? "В сети" : "Не в сети");
        statusLabel->setStyleSheet("color: #888; font-size: 12px;");
        
        userInfoLayout->addWidget(userNameLabel);
        userInfoLayout->addWidget(statusLabel);
        
        userLayout->addWidget(avatar);
        userLayout->addLayout(userInfoLayout);
        userLayout->addStretch();
        
        QPushButton *logoutButton = new QPushButton("Выйти");
        logoutButton->setObjectName("logoutButton");
        logoutButton->setFixedHeight(50);
        
        sidebarLayout->addWidget(userWidget);
        sidebarLayout->addStretch();
        sidebarLayout->addWidget(logoutButton);
        
        connect(logoutButton, &QPushButton::clicked, this, &MainAppWindow::onLogoutClicked);
        
        mainLayout->addWidget(sidebar);
    }
    
    void setupMainContent(QHBoxLayout *mainLayout) {
        QWidget *contentWidget = new QWidget();
        contentWidget->setObjectName("contentWidget");
        
        QVBoxLayout *contentLayout = new QVBoxLayout(contentWidget);
        contentLayout->setAlignment(Qt::AlignCenter);
        
        AvatarWidget *bigAvatar = new AvatarWidget(currentUser->getUserName());
        bigAvatar->setFixedSize(120, 120);
        
        QLabel *welcomeLabel = new QLabel(
            QString("Добро пожаловать, %1!").arg(currentUser->getUserName())
        );
        welcomeLabel->setAlignment(Qt::AlignCenter);
        QFont welcomeFont("Arial", 24, QFont::Bold);
        welcomeLabel->setFont(welcomeFont);
        
        contentLayout->addWidget(bigAvatar);
        contentLayout->addSpacing(20);
        contentLayout->addWidget(welcomeLabel);
        
        mainLayout->addWidget(contentWidget);
    }
    
    void applyTheme(const QString &theme) {
        QString bgColor = theme == THEME_DARK ? COLOR_DARK_BG : COLOR_LIGHT_BG;
        QString textColor = theme == THEME_DARK ? COLOR_DARK_TEXT : COLOR_LIGHT_TEXT;
        QString borderColor = theme == THEME_DARK ? COLOR_DARK_BORDER : COLOR_LIGHT_BORDER;
        
        setStyleSheet(QString(
            "QMainWindow { background: %1; }"
            "#sidebar { background: %2; border-right: 1px solid %3; }"
            "#contentWidget { background: %1; }"
            "QLabel { color: %4; }"
            "#logoutButton { "
            "    background: transparent; "
            "    color: %4; "
            "    border: none; "
            "    border-top: 1px solid %3; "
            "    font-size: 14px; "
            "}"
            "#logoutButton:hover { background: rgba(255,255,255,0.1); }"
        ).arg(bgColor).arg(theme == THEME_DARK ? "#2a2a2a" : "#e8e8e8").arg(borderColor).arg(textColor));
    }

private:
    User* currentUser;
};

class QuickLoginCircle : public QWidget {
    Q_OBJECT

public:
    QuickLoginCircle(const QString& username, QWidget* parent = nullptr) 
        : QWidget(parent), username(username) {
        setFixedSize(40, 40);
        setCursor(Qt::PointingHandCursor);
        
        deleteButton = new QPushButton(this);
        deleteButton->setFixedSize(16, 16);
        deleteButton->setStyleSheet(
            "QPushButton {"
            "    background: #ff4444;"
            "    border: none;"
            "    border-radius: 8px;"
            "    color: white;"
            "    font-size: 10px;"
            "    font-weight: bold;"
            "}"
            "QPushButton:hover {"
            "    background: #ff6666;"
            "}"
        );
        deleteButton->setText("×");
        deleteButton->move(24, 0);
        deleteButton->hide();
        
        connect(deleteButton, &QPushButton::clicked, this, &QuickLoginCircle::onDeleteClicked);
    }

    void enterEvent(QEnterEvent* event) override {
        deleteButton->show();
        QWidget::enterEvent(event);
    }
    
    void leaveEvent(QEvent* event) override {
        deleteButton->hide();
        QWidget::leaveEvent(event);
    }

signals:
    void clicked(const QString& username);
    void deleteRequested(const QString& username);

protected:
    void paintEvent(QPaintEvent* event) override {
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing);
        
        painter.setBrush(QColor(COLOR_ACCENT_GREEN));
        painter.setPen(Qt::NoPen);
        painter.drawEllipse(rect());
        
        painter.setPen(QColor("white"));
        painter.setFont(QFont("Arial", 14, QFont::Bold));
        painter.drawText(rect(), Qt::AlignCenter, username.left(1).toUpper());
    }
    
    void mousePressEvent(QMouseEvent* event) override {
        if (event->button() == Qt::LeftButton) {
            emit clicked(username);
        }
        QWidget::mousePressEvent(event);
    }

private slots:
    void onDeleteClicked() {
        QMessageBox::StandardButton reply = QMessageBox::question(
            this, 
            "Удаление пользователя",
            QString("Удалить пользователя '%1' из быстрого доступа?").arg(username),
            QMessageBox::Yes | QMessageBox::No,
            QMessageBox::No
        );
        
        if (reply == QMessageBox::Yes) {
            emit deleteRequested(username);
        }
    }

private:
    QString username;
    QPushButton* deleteButton;
};

class LoginWindow : public QMainWindow {
    Q_OBJECT

public:
    LoginWindow(QWidget *parent = nullptr) : QMainWindow(parent) {
        setupUI();
        applyTheme(THEME_DARK);
        updateQuickLoginCircles();
    }

public slots:
    void onLoginSuccess(User* user) {
        loginButton->setEnabled(true);
        loginButton->setText("Войти");
        
        saveUserForQuickLogin(user->getUserName());
        
        MainAppWindow *mainWindow = new MainAppWindow(user);
        connect(mainWindow, &MainAppWindow::logoutRequested, this, [this]() {
            this->show();
            updateQuickLoginCircles();
        });
        
        mainWindow->show();
        this->hide();
    }
    
    void onLoginFailed(const QString& error) {
        loginButton->setEnabled(true);
        loginButton->setText("Войти");
        QMessageBox::warning(this, "Ошибка входа", error);
    }
    
    void onRegistrationSuccess(User* user) {
        QMessageBox::information(this, "Успех", 
            QString("Регистрация выполнена!\nДобро пожаловать, %1").arg(user->getUserName()));
        saveUserForQuickLogin(user->getUserName());
        updateQuickLoginCircles();
    }
    
    void onQuickLoginClicked(const QString& username) {
        loginEdit->setText(username);
        
        QSettings settings;
        QString password = settings.value("user_password_" + username).toString();
        if (!password.isEmpty()) {
            passwordEdit->setText(password);
            onLoginButtonClicked();
        } else {
            passwordEdit->setFocus();
        }
    }
    
    void onQuickLoginDeleteRequested(const QString& username) {
        QSettings settings;
        QStringList users = settings.value("quick_login_users").toStringList();
        users.removeAll(username);
        settings.setValue("quick_login_users", users);
        
        settings.remove("user_password_" + username);
        
        updateQuickLoginCircles();
        
        qDebug() << "Пользователь" << username << "удален из быстрого доступа";
    }

private:
    void setupUI() {
        setWindowTitle("MINM Messenger - Вход");
        
        QWidget *centralWidget = new QWidget();
        setCentralWidget(centralWidget);
        
        QVBoxLayout *mainLayout = new QVBoxLayout(centralWidget);
        mainLayout->setSpacing(0);
        mainLayout->setContentsMargins(0, 0, 0, 0);
        
        setupLoginForm(mainLayout);
        setupBottomPanel(mainLayout);
    }
    
    void setupBottomPanel(QVBoxLayout *mainLayout) {
        QWidget *bottomPanel = new QWidget();
        bottomPanel->setFixedHeight(80);
        bottomPanel->setObjectName("bottomPanel");
        
        QHBoxLayout *bottomLayout = new QHBoxLayout(bottomPanel);
        bottomLayout->setContentsMargins(15, 10, 15, 10);
        bottomLayout->setAlignment(Qt::AlignCenter);
        
        quickLoginContainer = new QWidget();
        QHBoxLayout *circlesLayout = new QHBoxLayout(quickLoginContainer);
        circlesLayout->setSpacing(10);
        circlesLayout->setContentsMargins(0, 0, 0, 0);
        
        bottomLayout->addWidget(quickLoginContainer);
        
        mainLayout->addWidget(bottomPanel);
    }
    
    void setupLoginForm(QVBoxLayout *mainLayout) {
        QWidget *loginFormWidget = new QWidget();
        loginFormWidget->setObjectName("loginFormWidget");
        
        QVBoxLayout *formLayout = new QVBoxLayout(loginFormWidget);
        formLayout->setSpacing(20);
        formLayout->setContentsMargins(60, 40, 60, 30);
        formLayout->setAlignment(Qt::AlignCenter);
        
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
        
        loginEdit = new QLineEdit();
        loginEdit->setPlaceholderText("Логин");
        loginEdit->setObjectName("loginEdit");
        
        passwordEdit = new QLineEdit();
        passwordEdit->setPlaceholderText("Пароль");
        passwordEdit->setEchoMode(QLineEdit::Password);
        passwordEdit->setObjectName("passwordEdit");
        
        loginButton = new QPushButton("Войти");
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
        
        formLayout->addSpacing(10);
        formLayout->addLayout(headerLayout);
        formLayout->addWidget(subtitleLabel);
        formLayout->addSpacing(40);
        formLayout->addWidget(loginEdit);
        formLayout->addWidget(passwordEdit);
        formLayout->addSpacing(30);
        formLayout->addWidget(loginButton);
        formLayout->addSpacing(30);
        formLayout->addWidget(separator);
        formLayout->addSpacing(20);
        formLayout->addLayout(registerLayout);
        formLayout->addStretch();
        
        connect(loginButton, &QPushButton::clicked, this, &LoginWindow::onLoginButtonClicked);
        connect(registerButton, &QPushButton::clicked, this, &LoginWindow::onRegisterButtonClicked);
        connect(loginEdit, &QLineEdit::returnPressed, this, [this]() { passwordEdit->setFocus(); });
        connect(passwordEdit, &QLineEdit::returnPressed, this, [this]() { onLoginButtonClicked(); });
        
        mainLayout->addWidget(loginFormWidget);
    }
    
    void onLoginButtonClicked() {
        QString login = loginEdit->text();
        QString password = passwordEdit->text();
        
        if (login.isEmpty() || password.isEmpty()) {
            QMessageBox::warning(this, "Ошибка", "Заполните все поля");
        } else {
            loginButton->setEnabled(false);
            loginButton->setText("Вход...");
            
            QSettings settings;
            settings.setValue("user_password_" + login, password);
            
            if (g_authManager) {
                g_authManager->login(login, password);
            }
        }
    }
    
    void onRegisterButtonClicked() {
        QString login = loginEdit->text();
        QString password = passwordEdit->text();
        
        if (login.isEmpty() || password.isEmpty()) {
            QMessageBox::warning(this, "Ошибка", "Заполните все поля");
        } else {
            if (g_authManager) {
                g_authManager->registerUser(login, password);
            }
        }
    }
    
    void saveUserForQuickLogin(const QString& username) {
        if (username.isEmpty()) return;
        
        QSettings settings;
        QStringList users = settings.value("quick_login_users").toStringList();
        
        if (!users.contains(username)) {
            users.append(username);
            settings.setValue("quick_login_users", users);
        }
    }
    
    void updateQuickLoginCircles() {
        QLayoutItem* child;
        while ((child = quickLoginContainer->layout()->takeAt(0)) != nullptr) {
            delete child->widget();
            delete child;
        }
        
        QSettings settings;
        QStringList users = settings.value("quick_login_users").toStringList();
        
        for (const QString& username : users) {
            QuickLoginCircle* circle = new QuickLoginCircle(username);
            connect(circle, &QuickLoginCircle::clicked, this, &LoginWindow::onQuickLoginClicked);
            connect(circle, &QuickLoginCircle::deleteRequested, this, &LoginWindow::onQuickLoginDeleteRequested);
            quickLoginContainer->layout()->addWidget(circle);
        }
    }
    
    void applyTheme(const QString &theme) {
        QString mainBgColor = theme == THEME_DARK ? COLOR_DARK_BG : COLOR_LIGHT_BG;
        QString fieldBgColor = theme == THEME_DARK ? COLOR_DARK_FIELD_BG : COLOR_LIGHT_FIELD_BG;
        QString textColor = theme == THEME_DARK ? COLOR_DARK_TEXT : COLOR_LIGHT_TEXT;
        QString subtextColor = theme == THEME_DARK ? COLOR_DARK_SUBTEXT : COLOR_LIGHT_SUBTEXT;
        QString fieldBorderColor = theme == THEME_DARK ? COLOR_DARK_BORDER : COLOR_LIGHT_BORDER;

        setStyleSheet(QString(
            "QMainWindow { background: %1; }"
            "#bottomPanel { background: transparent; }"
            "#loginFormWidget { background: %1; }"
        ).arg(mainBgColor));

        for (QLabel *label : findChildren<QLabel*>()) {
            if (label->font().pointSize() >= 30) {
                 label->setStyleSheet("color: " + textColor + ";");
            } else {
                 label->setStyleSheet("color: " + subtextColor + ";");
            }
        }
        
        for (QLineEdit *edit : findChildren<QLineEdit*>()) {
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
        
        if (loginButton) {
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
        
        if (QPushButton *registerButton = findChild<QPushButton*>("registerButton")) {
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
        
        if (QWidget *separator = findChild<QWidget*>("separatorWidget")) {
            separator->setStyleSheet(QString("background: rgba(255,255,255,") + (theme == THEME_DARK ? "0.2" : "0.5") + ");");
        }
    }

private:
    QLineEdit *loginEdit;
    QLineEdit *passwordEdit;
    QPushButton *loginButton;
    QWidget *quickLoginContainer;
};

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    
    QCoreApplication::setOrganizationName("MINMDev");
    QCoreApplication::setApplicationName("MINMMessenger");
    
    AuthManager authManager("http://localhost:8080/api");
    g_authManager = &authManager;
    
    LoginWindow loginWindow;
    
    QObject::connect(&authManager, &AuthManager::loginSuccess, &loginWindow, &LoginWindow::onLoginSuccess);
    QObject::connect(&authManager, &AuthManager::loginFailed, &loginWindow, &LoginWindow::onLoginFailed);
    QObject::connect(&authManager, &AuthManager::registrationSuccess, &loginWindow, &LoginWindow::onRegistrationSuccess);
    
    const QRect screenGeometry = QGuiApplication::primaryScreen()->geometry();
    loginWindow.setGeometry(QStyle::alignedRect(
        Qt::LeftToRight,
        Qt::AlignCenter,
        QSize(900, 600),
        screenGeometry
    ));
    
    loginWindow.show();
    
    int result = app.exec();
    
    g_authManager = nullptr;
    
    return result;
}

#include "init.moc"