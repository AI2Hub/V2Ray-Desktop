#include "serverconfighelper.h"

#include <QDebug>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMap>
#include <QPair>
#include <QString>
#include <QStringList>
#include <QUrl>
#include <QUrlQuery>

#include "constants.h"
#include "utility.h"

QString ServerConfigHelper::getServerNameError(const QJsonObject& serverConfig,
                                               const QString* pServerName) {
  if (pServerName != nullptr) {
    QString newServerName = serverConfig["serverName"].toString();
    if (newServerName == *pServerName) {
      return Utility::getStringConfigError(serverConfig, "serverName",
                                           tr("Server Name"));
    }
  }
  return Utility::getStringConfigError(
    serverConfig, "serverName", tr("Server Name"),
    {std::bind(&Utility::isServerNameNotUsed, std::placeholders::_1)},
    tr("The '%1' has been used by another server."));
}

QStringList ServerConfigHelper::getV2RayServerConfigErrors(
  const QJsonObject& serverConfig, const QString* serverName) {
  QStringList errors;
  errors.append(getServerNameError(serverConfig, serverName));
  errors.append(Utility::getStringConfigError(
    serverConfig, "serverAddr", tr("Server Address"),
    {
      std::bind(&Utility::isIpAddrValid, std::placeholders::_1),
      std::bind(&Utility::isDomainNameValid, std::placeholders::_1),
    }));
  errors.append(Utility::getNumericConfigError(serverConfig, "serverPort",
                                               tr("Server Port"), 0, 65535));
  errors.append(Utility::getStringConfigError(serverConfig, "id", tr("ID")));
  errors.append(Utility::getNumericConfigError(serverConfig, "alterId",
                                               tr("Alter ID"), 0, 65535));
  errors.append(
    Utility::getStringConfigError(serverConfig, "security", tr("Security")));
  errors.append(
    Utility::getNumericConfigError(serverConfig, "mux", tr("MUX"), -1, 1024));
  errors.append(
    Utility::getStringConfigError(serverConfig, "network", tr("Network")));
  errors.append(Utility::getStringConfigError(serverConfig, "networkSecurity",
                                              tr("Network Security")));
  errors.append(getV2RayStreamSettingsErrors(
    serverConfig, serverConfig["network"].toString()));

  // Remove empty error messages generated by Utility::getNumericConfigError()
  // and Utility::getStringConfigError()
  errors.removeAll("");
  return errors;
}

QStringList ServerConfigHelper::getV2RayStreamSettingsErrors(
  const QJsonObject& serverConfig, const QString& network) {
  QStringList errors;
  if (network == "kcp") {
    errors.append(Utility::getNumericConfigError(serverConfig, "kcpMtu",
                                                 tr("MTU"), 576, 1460));
    errors.append(Utility::getNumericConfigError(serverConfig, "kcpTti",
                                                 tr("TTI"), 10, 100));
    errors.append(Utility::getNumericConfigError(
      serverConfig, "kcpUpLink", tr("Uplink Capacity"), 0, -127));
    errors.append(Utility::getNumericConfigError(
      serverConfig, "kcpDownLink", tr("Downlink Capacity"), 0, -127));
    errors.append(Utility::getNumericConfigError(
      serverConfig, "kcpReadBuffer", tr("Read Buffer Size"), 0, -127));
    errors.append(Utility::getNumericConfigError(
      serverConfig, "kcpWriteBuffer", tr("Write Buffer Size"), 0, -127));
    errors.append(Utility::getStringConfigError(serverConfig, "packetHeader",
                                                tr("Packet Header")));
  } else if (network == "ws" || network == "http") {
    errors.append(Utility::getStringConfigError(
      serverConfig, "networkHost", tr("Host"),
      {std::bind(&Utility::isDomainNameValid, std::placeholders::_1)}));
    errors.append(
      Utility::getStringConfigError(serverConfig, "networkPath", tr("Path")));
  } else if (network == "domainsocket") {
    errors.append(Utility::getStringConfigError(
      serverConfig, "domainSocketFilePath", tr("Socket File Path"),
      {std::bind(&Utility::isFileExists, std::placeholders::_1)}));
  } else if (network == "quic") {
    errors.append(Utility::getStringConfigError(serverConfig, "quicSecurity",
                                                tr("QUIC Security")));
    errors.append(Utility::getStringConfigError(serverConfig, "packetHeader",
                                                tr("Packet Header")));
    errors.append(
      Utility::getStringConfigError(serverConfig, "quicKey", tr("QUIC Key")));
  }
  return errors;
}

QJsonObject ServerConfigHelper::getPrettyV2RayConfig(
  const QJsonObject& serverConfig) {
  QJsonObject v2RayConfig{
    {"autoConnect", serverConfig["autoConnect"].toBool()},
    {"serverName", serverConfig["serverName"].toString()},
    {"subscription", serverConfig.contains("subscription")
                       ? serverConfig["subscription"].toString()
                       : ""},
    {"protocol", "vmess"},
    {"mux",
     QJsonObject{
       {"enabled", serverConfig["mux"].toVariant().toInt() != -1},
       {"concurrency", serverConfig["mux"].toVariant().toInt()},
     }},
    {"settings",
     QJsonObject{
       {"vnext",
        QJsonArray{QJsonObject{
          {"address", serverConfig["serverAddr"].toString()},
          {"port", serverConfig["serverPort"].toVariant().toInt()},
          {"users",
           QJsonArray{QJsonObject{
             {"id", serverConfig["id"].toString()},
             {"alterId", serverConfig["alterId"].toVariant().toInt()},
             {"security", serverConfig["security"].toString().toLower()},
           }}}}}}}},
    {"tag", "proxy-vmess"}};

  QJsonObject streamSettings = getV2RayStreamSettingsConfig(serverConfig);
  v2RayConfig.insert("streamSettings", streamSettings);
  return v2RayConfig;
}

QJsonObject ServerConfigHelper::getV2RayStreamSettingsConfig(
  const QJsonObject& serverConfig) {
  QString network = serverConfig["network"].toString();
  QJsonObject streamSettings{
    {"network", serverConfig["network"]},
    {"security", serverConfig["networkSecurity"].toString().toLower()},
    {"tlsSettings",
     QJsonObject{{"allowInsecure", serverConfig["allowInsecure"].toBool()}}},
  };

  if (network == "tcp") {
    QString tcpHeaderType = serverConfig["tcpHeaderType"].toString().toLower();
    QJsonObject tcpSettings{{"type", tcpHeaderType}};
    if (tcpHeaderType == "http") {
      tcpSettings.insert(
        "request",
        QJsonObject{
          {"version", "1.1"},
          {"method", "GET"},
          {"path", QJsonArray{"/"}},
          {"headers",
           QJsonObject{
             {"host",
              QJsonArray{"www.baidu.com", "www.bing.com", "www.163.com",
                         "www.netease.com", "www.qq.com", "www.tencent.com",
                         "www.taobao.com", "www.tmall.com",
                         "www.alibaba-inc.com", "www.aliyun.com",
                         "www.sensetime.com", "www.megvii.com"}},
             {"User-Agent", getRandomUserAgents(24)},
             {"Accept-Encoding", QJsonArray{"gzip, deflate"}},
             {"Connection", QJsonArray{"keep-alive"}},
             {"Pragma", "no-cache"},
           }},
        });
      tcpSettings.insert(
        "response",
        QJsonObject{
          {"version", "1.1"},
          {"status", "200"},
          {"reason", "OK"},
          {"headers",
           QJsonObject{{"Content-Type", QJsonArray{"text/html;charset=utf-8"}},
                       {"Transfer-Encoding", QJsonArray{"chunked"}},
                       {"Connection", QJsonArray{"keep-alive"}},
                       {"Pragma", "no-cache"}}}});
    }
    streamSettings.insert("tcpSettings", tcpSettings);
  } else if (network == "kcp") {
    streamSettings.insert(
      "kcpSettings",
      QJsonObject{
        {"mtu", serverConfig["kcpMtu"].toVariant().toInt()},
        {"tti", serverConfig["kcpTti"].toVariant().toInt()},
        {"uplinkCapacity", serverConfig["kcpUpLink"].toVariant().toInt()},
        {"downlinkCapacity", serverConfig["kcpDownLink"].toVariant().toInt()},
        {"congestion", serverConfig["kcpCongestion"].toBool()},
        {"readBufferSize", serverConfig["kcpReadBuffer"].toVariant().toInt()},
        {"writeBufferSize", serverConfig["kcpWriteBuffer"].toVariant().toInt()},
        {"header",
         QJsonObject{
           {"type", serverConfig["packetHeader"].toString().toLower()}}}});
  } else if (network == "ws") {
    streamSettings.insert(
      "wsSettings",
      QJsonObject{
        {"path", serverConfig["networkPath"].toString()},
        {"headers", QJsonObject{{"host", serverConfig["networkHost"]}}}});
  } else if (network == "http") {
    streamSettings.insert(
      "httpSettings",
      QJsonObject{
        {"host", QJsonArray{serverConfig["networkHost"].toString()}},
        {"path", QJsonArray{serverConfig["networkPath"].toString()}}});
  } else if (network == "domainsocket") {
    streamSettings.insert(
      "dsSettings",
      QJsonObject{{"path", serverConfig["domainSocketFilePath"].toString()}});
  } else if (network == "quic") {
    streamSettings.insert(
      "quicSettings",
      QJsonObject{
        {"security", serverConfig["quicSecurity"].toString().toLower()},
        {"key", serverConfig["quicKey"].toString()},
        {"header",
         QJsonObject{
           {"type", serverConfig["packetHeader"].toString().toLower()}}}});
  }
  return streamSettings;
}

QJsonArray ServerConfigHelper::getRandomUserAgents(int n) {
  QStringList OPERATING_SYSTEMS{"Macintosh; Intel Mac OS X 10_15",
                                "X11; Linux x86_64",
                                "Windows NT 10.0; Win64; x64"};
  QJsonArray userAgents;
  for (int i = 0; i < n; ++i) {
    int osIndex            = std::rand() % 3;
    int chromeMajorVersion = std::rand() % 30 + 50;
    int chromeBuildVersion = std::rand() % 4000 + 1000;
    int chromePatchVersion = std::rand() % 100;
    userAgents.append(QString("Mozilla/5.0 (%1) AppleWebKit/537.36 (KHTML, "
                              "like Gecko) Chrome/%2.0.%3.%4 Safari/537.36")
                        .arg(OPERATING_SYSTEMS[osIndex],
                             QString::number(chromeMajorVersion),
                             QString::number(chromeBuildVersion),
                             QString::number(chromePatchVersion)));
  }
  return userAgents;
}

QJsonObject ServerConfigHelper::getV2RayServerConfigFromUrl(
  const QString& server, const QString& subscriptionUrl) {
  // Ref:
  // https://github.com/2dust/v2rayN/wiki/%E5%88%86%E4%BA%AB%E9%93%BE%E6%8E%A5%E6%A0%BC%E5%BC%8F%E8%AF%B4%E6%98%8E(ver-2)
  const QMap<QString, QString> NETWORK_MAPPER = {
    {"tcp", "tcp"}, {"kcp", "kcp"},   {"ws", "ws"},
    {"h2", "http"}, {"quic", "quic"},
  };
  QJsonObject rawServerConfig =
    QJsonDocument::fromJson(QByteArray::fromBase64(server.mid(8).toUtf8()))
      .object();
  QString network =
    rawServerConfig.contains("net") ? rawServerConfig["net"].toString() : "tcp";
  QString serverAddr =
    rawServerConfig.contains("add") ? rawServerConfig["add"].toString() : "";
  int alterId = rawServerConfig.contains("aid")
                  ? (rawServerConfig["aid"].isString()
                       ? rawServerConfig["aid"].toString().toInt()
                       : rawServerConfig["aid"].toInt())
                  : 0;

  QJsonObject serverConfig{
    {"autoConnect", false},
    {"serverName", rawServerConfig.contains("ps")
                     ? rawServerConfig["ps"].toString()
                     : serverAddr},
    {"serverAddr", serverAddr},
    {"serverPort", rawServerConfig.contains("port")
                     ? QString::number(rawServerConfig["port"].toInt())
                     : ""},
    {"subscription", subscriptionUrl},
    {"id",
     rawServerConfig.contains("id") ? rawServerConfig["id"].toString() : ""},
    {"alterId", alterId},
    {"mux", -1},
    {"security", "auto"},
    {"network",
     NETWORK_MAPPER.contains(network) ? NETWORK_MAPPER[network] : "tcp"},
    {"networkHost", rawServerConfig.contains("host")
                      ? rawServerConfig["host"].toString()
                      : ""},
    {"networkPath", rawServerConfig.contains("path")
                      ? rawServerConfig["path"].toString()
                      : ""},
    {"tcpHeaderType", rawServerConfig.contains("type")
                        ? rawServerConfig["type"].toString()
                        : ""},
    {"networkSecurity", rawServerConfig.contains("tls") ? "tls" : "none"}};

  qDebug() << rawServerConfig;
  qDebug() << serverConfig;
  return serverConfig;
}

QStringList ServerConfigHelper::getShadowsocksServerConfigErrors(
  const QJsonObject& serverConfig, const QString* pServerName) {
  QStringList errors;
  errors.append(getServerNameError(serverConfig, pServerName));
  errors.append(Utility::getStringConfigError(
    serverConfig, "serverAddr", tr("Server Address"),
    {
      std::bind(&Utility::isIpAddrValid, std::placeholders::_1),
      std::bind(&Utility::isDomainNameValid, std::placeholders::_1),
    }));
  errors.append(Utility::getNumericConfigError(serverConfig, "serverPort",
                                               tr("Server Port"), 0, 65535));
  errors.append(
    Utility::getStringConfigError(serverConfig, "encryption", tr("Security")));
  errors.append(
    Utility::getStringConfigError(serverConfig, "password", tr("Password")));
  if (serverConfig.contains("plugins")) {
    errors.append(tr("The Shadowsocks plugins are currently not supported."));
  }

  // Remove empty error messages generated by getNumericConfigError() and
  // getStringConfigError()
  errors.removeAll("");
  return errors;
}

QJsonObject ServerConfigHelper::getPrettyShadowsocksConfig(
  const QJsonObject& serverConfig) {
  return QJsonObject{
    {"autoConnect", serverConfig["autoConnect"].toBool()},
    {"serverName", serverConfig["serverName"].toString()},
    {"subscription", serverConfig.contains("subscription")
                       ? serverConfig["subscription"].toString()
                       : ""},
    {"protocol", "shadowsocks"},
    {"settings",
     QJsonObject{{"servers",
                  QJsonArray{QJsonObject{
                    {"address", serverConfig["serverAddr"].toString()},
                    {"port", serverConfig["serverPort"].toVariant().toInt()},
                    {"method", serverConfig["encryption"].toString().toLower()},
                    {"password", serverConfig["password"].toString()}}}}}},
    {"streamSettings", QJsonObject{{"network", "tcp"}}},
    {"tag", "proxy-shadowsocks"}};
}

QJsonObject ServerConfigHelper::getShadowsocksServerConfigFromUrl(
  QString serverUrl, const QString& subscriptionUrl) {
  serverUrl             = serverUrl.mid(5);
  int atIndex           = serverUrl.indexOf('@');
  int colonIndex        = serverUrl.indexOf(':');
  int splashIndex       = serverUrl.indexOf('/');
  int sharpIndex        = serverUrl.indexOf('#');
  int questionMarkIndex = serverUrl.indexOf('?');

  QString confidential =
    QByteArray::fromBase64(serverUrl.left(atIndex).toUtf8());
  QString serverAddr = serverUrl.mid(atIndex + 1, colonIndex - atIndex - 1);
  QString serverPort =
    serverUrl.mid(colonIndex + 1, splashIndex - colonIndex - 1);
  QString plugins =
    serverUrl.mid(questionMarkIndex + 1, sharpIndex - questionMarkIndex - 1);
  QString serverName =
    QUrl::fromPercentEncoding(serverUrl.mid(sharpIndex + 1).toUtf8());

  colonIndex         = confidential.indexOf(':');
  QString encryption = confidential.left(colonIndex);
  QString password   = confidential.mid(colonIndex + 1);

  QJsonObject serverConfig{{"serverName", serverName},
                           {"autoConnect", false},
                           {"subscription", subscriptionUrl},
                           {"serverAddr", serverAddr},
                           {"serverPort", serverPort},
                           {"encryption", encryption},
                           {"password", password}};

  QJsonObject pluginOptions = getShadowsocksPlugins(plugins);
  if (!pluginOptions.empty()) {
    serverConfig["plugins"] = pluginOptions;
  }
  return serverConfig;
}

QJsonObject ServerConfigHelper::getShadowsocksPlugins(
  const QString& pluginString) {
  QJsonObject plugins;
  for (QPair<QString, QString> p : QUrlQuery(pluginString).queryItems()) {
    if (p.first == "plugin") {
      QStringList options =
        QUrl::fromPercentEncoding(p.second.toUtf8()).split(';');
      for (QString o : options) {
        QStringList t = o.split('=');
        if (t.size() == 2) {
          plugins[t[0]] = t[1];
        }
      }
    }
  }
  return plugins;
}

ServerConfigHelper::Protocol ServerConfigHelper::getProtocol(QString protocol) {
  if (protocol == "vmess" || protocol == "v2ray") {
    return Protocol::VMESS;
  } else if (protocol == "shadowsocks") {
    return Protocol::SHADOWSOCKS;
  } else {
    return Protocol::UNKNOWN;
  }
}

QStringList ServerConfigHelper::getServerConfigErrors(
  Protocol protocol,
  const QJsonObject& serverConfig,
  const QString* pServerName) {
  if (protocol == Protocol::VMESS) {
    return getV2RayServerConfigErrors(serverConfig, pServerName);
  } else if (protocol == Protocol::SHADOWSOCKS) {
    return getShadowsocksServerConfigErrors(serverConfig, pServerName);
  }
  return QStringList{tr("Unknown Server protocol")};
}

QJsonObject ServerConfigHelper::getPrettyServerConfig(
  Protocol protocol, const QJsonObject& serverConfig) {
  if (protocol == Protocol::VMESS) {
    return getPrettyV2RayConfig(serverConfig);
  } else if (protocol == Protocol::SHADOWSOCKS) {
    return getPrettyShadowsocksConfig(serverConfig);
  }
  return QJsonObject{};
}

QJsonObject ServerConfigHelper::getServerConfigFromUrl(
  Protocol protocol, const QString& serverUrl, const QString& subscriptionUrl) {
  if (protocol == Protocol::VMESS) {
    return getV2RayServerConfigFromUrl(serverUrl, subscriptionUrl);
  } else if (protocol == Protocol::SHADOWSOCKS) {
    return getShadowsocksServerConfigFromUrl(serverUrl, subscriptionUrl);
  }
  return QJsonObject{};
}

QList<QJsonObject> ServerConfigHelper::getServerConfigFromV2RayConfig(
  const QJsonObject& config) {
  QList<QJsonObject> servers;
  QJsonArray serversConfig = config["outbounds"].toArray();
  for (auto itr = serversConfig.begin(); itr != serversConfig.end(); ++itr) {
    QJsonObject server = (*itr).toObject();
    QString protocol   = server["protocol"].toString();
    if (protocol != "vmess") {
      qWarning() << QString("Ignore the server protocol: %1").arg(protocol);
      continue;
    }
    QJsonObject serverSettings =
      getV2RayServerSettingsFromConfig(server["settings"].toObject());
    QJsonObject streamSettings = getV2RayStreamSettingsFromConfig(
      server["streamSettings"].toObject(), config["transport"].toObject());
    if (!serverSettings.empty()) {
      QJsonObject serverConfig = serverSettings;
      // Stream Settings
      for (auto itr = streamSettings.begin(); itr != streamSettings.end();
           ++itr) {
        serverConfig.insert(itr.key(), itr.value());
      }
      // MUX Settings
      if (server.contains("mux")) {
        QJsonObject muxObject = server["mux"].toObject();
        int mux               = muxObject["concurrency"].toVariant().toInt();
        if (mux > 0) {
          serverConfig.insert("mux", QString::number(mux));
        }
      }
      if (!serverConfig.contains("mux")) {
        // Default value for MUX
        serverConfig["mux"] = -1;
      }
      servers.append(serverConfig);
    }
  }
  return servers;
}

QJsonObject ServerConfigHelper::getV2RayServerSettingsFromConfig(
  const QJsonObject& settings) {
  QJsonObject server;
  QJsonArray vnext = settings["vnext"].toArray();
  if (vnext.size()) {
    QJsonObject _server  = vnext.at(0).toObject();
    server["serverAddr"] = _server["address"].toString();
    server["serverPort"] = _server["port"].toVariant().toInt();
    server["serverName"] =
      QString("%1:%2").arg(server["serverAddr"].toString(),
                           QString::number(server["serverPort"].toInt()));
    QJsonArray users = _server["users"].toArray();
    if (users.size()) {
      QJsonObject user  = users.at(0).toObject();
      server["id"]      = user["id"].toString();
      server["alterId"] = user["alterId"].toVariant().toInt();
      server["security"] =
        user.contains("security") ? user["security"].toString() : "auto";
    }
  }
  return server;
}

QJsonObject ServerConfigHelper::getV2RayStreamSettingsFromConfig(
  const QJsonObject& transport, const QJsonObject& streamSettings) {
  QJsonObject _streamSettings =
    streamSettings.empty() ? transport : streamSettings;
  QJsonObject serverStreamSettings;
  QString network = _streamSettings.contains("network")
                      ? _streamSettings["network"].toString()
                      : "tcp";
  serverStreamSettings["network"] = network;
  serverStreamSettings["networkSecurity"] =
    _streamSettings.contains("security")
      ? _streamSettings["security"].toString()
      : "none";
  serverStreamSettings["allowInsecure"] = true;
  if (_streamSettings.contains("tlsSettings")) {
    QJsonObject tlsSettings = _streamSettings["tlsSettings"].toObject();
    serverStreamSettings["allowInsecure"] = tlsSettings["allowInsecure"];
  }
  if (network == "tcp") {
    QJsonObject tcpSettings = _streamSettings["tcpSettings"].toObject();
    QJsonObject header      = tcpSettings["header"].toObject();
    serverStreamSettings["tcpHeaderType"] =
      header.contains("type") ? header["type"].toString() : "none";
  } else if (network == "kcp") {
    QJsonObject kcpSettings        = _streamSettings["kcpSettings"].toObject();
    QJsonObject header             = kcpSettings["header"].toObject();
    serverStreamSettings["kcpMtu"] = kcpSettings.contains("mtu")
                                       ? kcpSettings["mtu"].toVariant().toInt()
                                       : DEFAULT_V2RAY_KCP_MTU;
    serverStreamSettings["kcpTti"] = kcpSettings.contains("tti")
                                       ? kcpSettings["tti"].toVariant().toInt()
                                       : DEFAULT_V2RAY_KCP_TTI;
    serverStreamSettings["kcpUpLink"] =
      kcpSettings.contains("uplinkCapacity")
        ? kcpSettings["uplinkCapacity"].toVariant().toInt()
        : DEFAULT_V2RAY_KCP_UP_CAPACITY;
    serverStreamSettings["kcpDownLink"] =
      kcpSettings.contains("downlinkCapacity")
        ? kcpSettings["downlinkCapacity"].toVariant().toInt()
        : DEFAULT_V2RAY_KCP_DOWN_CAPACITY;
    serverStreamSettings["kcpReadBuffer"] =
      kcpSettings.contains("readBufferSize")
        ? kcpSettings["readBufferSize"].toVariant().toInt()
        : DEFAULT_V2RAY_KCP_READ_BUF_SIZE;
    serverStreamSettings["kcpWriteBuffer"] =
      kcpSettings.contains("writeBufferSize")
        ? kcpSettings["writeBufferSize"].toVariant().toInt()
        : DEFAULT_V2RAY_KCP_READ_BUF_SIZE;
    serverStreamSettings["kcpCongestion"] = kcpSettings["congestion"].toBool();
    serverStreamSettings["packetHeader"] =
      header.contains("type") ? header["type"].toString() : "none";
  } else if (network == "ws") {
    QJsonObject wsSettings = _streamSettings["wsSettings"].toObject();
    QJsonObject headers    = wsSettings["headers"].toObject();
    serverStreamSettings["networkHost"] = headers.contains("host")
                                            ? headers["host"].toString()
                                            : headers["Host"].toString();
    serverStreamSettings["networkPath"] = wsSettings["path"];
  } else if (network == "http") {
    QJsonObject httpSettings = _streamSettings["httpSettings"].toObject();
    serverStreamSettings["networkHost"] = httpSettings["host"].toArray().at(0);
    serverStreamSettings["networkPath"] = httpSettings["path"].toString();
  } else if (network == "domainsocket") {
    QJsonObject dsSettings = _streamSettings["dsSettings"].toObject();
    serverStreamSettings["domainSocketFilePath"] =
      dsSettings["path"].toString();
  } else if (network == "quic") {
    QJsonObject quicSettings = _streamSettings["quicSettings"].toObject();
    QJsonObject header       = quicSettings["header"].toObject();
    serverStreamSettings["quicSecurity"] =
      quicSettings.contains("security") ? quicSettings["security"].toString()
                                        : "none";
    serverStreamSettings["packetHeader"] =
      header.contains("type") ? header["type"].toString() : "none";
    serverStreamSettings["quicKey"] = quicSettings["key"].toString();
  }
  return serverStreamSettings;
}

QList<QJsonObject> ServerConfigHelper::getServerConfigFromShadowsocksQt5Config(
  const QJsonObject& config) {
  QList<QJsonObject> servers;
  QJsonArray serversConfig = config["configs"].toArray();

  for (auto itr = serversConfig.begin(); itr != serversConfig.end(); ++itr) {
    QJsonObject server       = (*itr).toObject();
    QJsonObject serverConfig = {
      {"serverName", server["remarks"].toString()},
      {"serverAddr", server["server"].toString()},
      {"serverPort", QString::number(server["server_port"].toInt())},
      {"encryption", server["method"].toString()},
      {"password", server["password"].toString()},
    };
    if (server.contains("plugin_opts") &&
        !server["plugin_opts"].toString().isEmpty()) {
      QString plugins =
        QString("plugin=%1%3B%2")
          .arg(
            server["plugin"].toString(),
            QString(QUrl::toPercentEncoding(server["plugin_opts"].toString())));
      serverConfig["plugins"] = getShadowsocksPlugins(plugins);
    }
    servers.append(serverConfig);
  }
  return servers;
}
