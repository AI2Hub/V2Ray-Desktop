#ifndef APPPROXY_H
#define APPPROXY_H

#include <QObject>
#include <QString>

#include "v2raycore.h"

class AppProxy: public QObject {
    Q_OBJECT
public:
    AppProxy();

    QString getV2RayCoreStatusTmp();
signals:
    void appVersionReady(QString appVersion);
    void v2RayCoreVersionReady(QString v2RayCoreVersion);
    void operatingSystemReady(QString operatingSystem);
    void v2RayCoreStatusReady(QString v2RayCoreStatus);
    void v2RayRunningStatusChanging(bool isChanged);

public slots:
    QString getAppVersion();
    QString getV2RayCoreVersion();
    QString getOperatingSystem();
    QString getV2RayCoreStatus();
    bool setV2RayCoreRunning(bool expectedRunning);

private:
    V2RayCore& v2ray;
};

#endif // APPPROXY_H