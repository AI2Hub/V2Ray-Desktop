#include "appproxy.h"

#include <QDebug>
#include <QSysInfo>

#include "constants.h"

AppProxy::AppProxy() : v2ray(V2RayCore::getInstance()) {}

QString AppProxy::getAppVersion() {
  QString appVersion = QString("%1.%2.%3")
                         .arg(QString::number(APP_VERSION_MAJOR),
                              QString::number(APP_VERSION_MINOR),
                              QString::number(APP_VERSION_PATCH));
  emit appVersionReady(appVersion);
  return appVersion;
}

QString AppProxy::getV2RayCoreVersion() { return ""; }

QString AppProxy::getOperatingSystem() {
  QString operatingSystem = QSysInfo::prettyProductName();
  emit operatingSystemReady(operatingSystem);
  return operatingSystem;
}

QString AppProxy::getV2RayCoreStatus() {
  bool isInstalled = v2ray.isInstalled();
  bool isRunning   = v2ray.isRunning();
  QString v2rayStatus =
    isInstalled ? (isRunning ? "Running" : "Stopped") : "Not Installed";
  emit v2RayCoreStatusReady(v2rayStatus);
  return v2rayStatus;
}

bool AppProxy::setV2RayCoreRunning(bool expectedRunning) {
  bool isSuccessful = false;
  if (expectedRunning) {
    isSuccessful = v2ray.start();
    qInfo() << QString("Start V2Ray Core ... %1")
                 .arg(isSuccessful ? "success" : "failed");
    emit v2RayRunningStatusChanging(isSuccessful);
  } else {
    isSuccessful = v2ray.stop();
    qInfo() << QString("Stop V2Ray Core ... %1")
                 .arg(isSuccessful ? "success" : "failed");
    emit v2RayRunningStatusChanging(isSuccessful);
  }
  return isSuccessful;
}