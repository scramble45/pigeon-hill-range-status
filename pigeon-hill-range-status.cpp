#include <QtWidgets/QApplication>
#include <QtWidgets/QSystemTrayIcon>
#include <QtWidgets/QMenu>
#include <QtGui/QAction>
#include <QtCore/QTimer>
#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QNetworkRequest>
#include <QtNetwork/QNetworkReply>
#include <QtCore/QRegularExpression>
#include <QtGui/QIcon>
#include <QtGui/QPainter>
#include <QtGui/QDesktopServices>
#include <QtCore/QUrl>
#include <QtCore/QDebug>
#include <QtCore/QTime>

class RangeMonitor : public QObject {
    Q_OBJECT

public:
    RangeMonitor() {
        setupSystemTray();
        setupNetworking();
        
        // Check every 2 hours
        timer = new QTimer(this);
        connect(timer, &QTimer::timeout, this, &RangeMonitor::checkStatus);
        timer->start(2 * 60 * 60 * 1000);
        
        // Initial check
        checkStatus();
    }

private slots:
    void checkStatus() {
        QNetworkRequest request(QUrl("https://mdc.mo.gov/discover-nature/places/pigeon-hill-conservation-area"));
        request.setHeader(QNetworkRequest::UserAgentHeader, "pigeon-hill-range-status/1.0");
        
        QNetworkReply *reply = network->get(request);
        connect(reply, &QNetworkReply::finished, [this, reply]() {
            reply->deleteLater();
            
            if (reply->error() != QNetworkReply::NoError) {
                updateIcon(false, "Error checking status");
                return;
            }
            
            QString html = reply->readAll();
            bool isOpen = parseStatus(html);
            updateIcon(isOpen, isOpen ? "Range is OPEN" : "Range is CLOSED");
        });
    }
    
    void onTrayClick(QSystemTrayIcon::ActivationReason reason) {
        if (reason == QSystemTrayIcon::MiddleClick) {
            checkStatus();
        }
    }
    
    void openWebsite() {
        QDesktopServices::openUrl(QUrl("https://mdc.mo.gov/discover-nature/places/pigeon-hill-conservation-area"));
    }

private:
    void setupSystemTray() {
        tray = new QSystemTrayIcon(this);
        tray->setIcon(createIcon(false)); // Start with unknown (red)
        tray->setToolTip("Range Monitor - Checking...");
        
        // Create menu
        QMenu *menu = new QMenu();
        
        QAction *checkAction = new QAction("Check Now", this);
        connect(checkAction, &QAction::triggered, this, &RangeMonitor::checkStatus);
        menu->addAction(checkAction);
        
        QAction *websiteAction = new QAction("Open Website", this);
        connect(websiteAction, &QAction::triggered, this, &RangeMonitor::openWebsite);
        menu->addAction(websiteAction);
        
        menu->addSeparator();
        
        QAction *quitAction = new QAction("Quit", this);
        connect(quitAction, &QAction::triggered, QApplication::instance(), &QApplication::quit);
        menu->addAction(quitAction);
        
        tray->setContextMenu(menu);
        connect(tray, &QSystemTrayIcon::activated, this, &RangeMonitor::onTrayClick);
        tray->show();
    }
    
    void setupNetworking() {
        network = new QNetworkAccessManager(this);
    }
    
    QIcon createIcon(bool isOpen) {
        QPixmap pixmap(32, 32);
        pixmap.fill(Qt::transparent);
        
        QPainter painter(&pixmap);
        painter.setRenderHint(QPainter::Antialiasing);
        
        // Draw circle
        QColor color = isOpen ? QColor(76, 175, 80) : QColor(244, 67, 54); // Green or Red
        painter.setBrush(QBrush(color));
        painter.setPen(QPen(color.darker(), 2));
        painter.drawEllipse(4, 4, 24, 24);
        
        // Draw symbol  
        painter.setPen(QPen(Qt::white, 2));
        if (isOpen) {
            // Checkmark
            painter.drawLine(10, 16, 14, 20);
            painter.drawLine(14, 20, 22, 12);
        } else {
            // X mark
            painter.drawLine(10, 10, 22, 22);
            painter.drawLine(22, 10, 10, 22);
        }
        
        return QIcon(pixmap);
    }
    
    bool parseStatus(const QString &html) {
        // Look for danger alerts indicating closure
        QRegularExpression alertRegex(R"(<div[^>]*class="[^"]*alert alert-danger[^"]*"[^>]*>.*?field__item[^>]*>([^<]+)</div>)", 
                                     QRegularExpression::DotMatchesEverythingOption);
        QRegularExpressionMatch match = alertRegex.match(html);
        
        if (match.hasMatch()) {
            QString alertText = match.captured(1).toLower();
            // If alert mentions closure, range is closed
            if (alertText.contains("closed") || alertText.contains("closure") || alertText.contains("prohibited")) {
                return false; // CLOSED
            }
        }
        
        return true; // OPEN (no closure alerts found)
    }
    
    void updateIcon(bool isOpen, const QString &message) {
        tray->setIcon(createIcon(isOpen));
        tray->setToolTip(QString("Range Monitor\n%1\nLast check: %2")
                        .arg(message)
                        .arg(QTime::currentTime().toString()));
        
        // Show notification
        if (tray->supportsMessages()) {
            tray->showMessage("Range Status", message, QSystemTrayIcon::Information, 3000);
        }
        
        qDebug() << message;
    }

private:
    QSystemTrayIcon *tray;
    QNetworkAccessManager *network;
    QTimer *timer;
};

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    app.setQuitOnLastWindowClosed(false);
    
    if (!QSystemTrayIcon::isSystemTrayAvailable()) {
        qCritical() << "System tray not available!";
        return 1;
    }
    
    RangeMonitor monitor;
    
    return app.exec();
}

#include "pigeon-hill-range-status.moc"