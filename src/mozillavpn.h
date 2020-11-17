/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef MOZILLAVPN_H
#define MOZILLAVPN_H

#include "captiveportal/captiveportal.h"
#include "captiveportal/captiveportaldetection.h"
#include "closeeventhandler.h"
#include "connectiondataholder.h"
#include "connectionhealth.h"
#include "controller.h"
#include "errorhandler.h"
#include "models/devicemodel.h"
#include "models/helpmodel.h"
#include "models/keys.h"
#include "models/servercountrymodel.h"
#include "models/serverdata.h"
#include "models/user.h"
#include "releasemonitor.h"
#include "statusicon.h"

#include <QList>
#include <QNetworkReply>
#include <QObject>
#include <QPointer>
#include <QStandardPaths>
#include <QTimer>

class Task;

#ifdef UNIT_TEST
class TestTasks;
#endif

class MozillaVPN final : public QObject
{
    Q_OBJECT

public:
    enum State {
        StateInitialize,
        StateAuthenticating,
        StatePostAuthentication,
        StateMain,
        StateUpdateRequired,
        StateSubscriptionNeeded,
        StateDeviceLimit,
    };
    Q_ENUM(State);

    enum AlertType {
        NoAlert,
        AuthenticationFailedAlert,
        ConnectionFailedAlert,
        LogoutAlert,
        NoConnectionAlert,
        BackendServiceErrorAlert,
    };
    Q_ENUM(AlertType)

    enum LinkType {
        LinkAccount,
        LinkContact,
        LinkFeedback,
        LinkHelpSupport,
        LinkTermsOfService,
        LinkPrivacyPolicy,
        LinkUpdate,
    };
    Q_ENUM(LinkType)

private:
    Q_PROPERTY(State state READ state NOTIFY stateChanged)
    Q_PROPERTY(AlertType alert READ alert NOTIFY alertChanged)
    Q_PROPERTY(QString versionString READ versionString CONSTANT)
    Q_PROPERTY(bool updateRecommended READ updateRecommended NOTIFY updateRecommendedChanged)
    Q_PROPERTY(bool userAuthenticated READ userAuthenticated NOTIFY userAuthenticationChanged)
    Q_PROPERTY(bool startMinimized READ startMinimized CONSTANT)
    Q_PROPERTY(bool startOnBootSupported READ startOnBootSupported CONSTANT)

public:
    MozillaVPN();
    ~MozillaVPN();

    static MozillaVPN *instance();

    void initialize();

    State state() const;
    AlertType alert() const { return m_alert; }

    // Exposed QML methods:
    Q_INVOKABLE void authenticate();
    Q_INVOKABLE void cancelAuthentication();
    Q_INVOKABLE void openLink(LinkType linkType);
    Q_INVOKABLE void removeDevice(const QString &deviceName);
    Q_INVOKABLE void hideAlert() { setAlert(NoAlert); }
    Q_INVOKABLE void hideUpdateRecommendedAlert() { setUpdateRecommended(false); }
    Q_INVOKABLE void postAuthenticationCompleted();
    Q_INVOKABLE void subscribe();
    Q_INVOKABLE void restoreSubscription();
    Q_INVOKABLE void viewLogs();
    Q_INVOKABLE QString retrieveLogs();

    // Internal object getters:
    CaptivePortal *captivePortal() { return &m_private->m_captivePortal; }
    CaptivePortalDetection *captivePortalDetection()
    {
        return &m_private->m_captivePortalDetection;
    }
    CloseEventHandler *closeEventHandler() { return &m_private->m_closeEventHandler; }
    ConnectionDataHolder *connectionDataHolder() { return &m_private->m_connectionDataHolder; }
    ConnectionHealth *connectionHealth() { return &m_private->m_connectionHealth; }
    Controller *controller() { return &m_private->m_controller; }
    ServerData *currentServer() { return &m_private->m_serverData; }
    DeviceModel *deviceModel() { return &m_private->m_deviceModel; }
    Keys *keys() { return &m_private->m_keys; }
    HelpModel *helpModel() { return &m_private->m_helpModel; }
    ServerCountryModel *serverCountryModel() { return &m_private->m_serverCountryModel; }
    StatusIcon *statusIcon() { return &m_private->m_statusIcon; }
    User *user() { return &m_private->m_user; }

    // Called at the end of the authentication flow. We can continue adding the device
    // if it doesn't exist yet, or we can go to OFF state.
    void authenticationCompleted(const QByteArray &json, const QString &token);

    void deviceAdded(const QString &deviceName, const QString &publicKey, const QString &privateKey);

    void deviceRemoved(const QString &deviceName);

    void serversFetched(const QByteArray &serverData);

    void accountChecked(const QByteArray &json);

    const QList<Server> getServers() const;

    void errorHandle(ErrorHandler::ErrorType error);

    void abortAuthentication();

    void changeServer(const QString &countryCode, const QString &city);

    const QString versionString() const { return QString(APP_VERSION); }

    void logout();

    bool updateRecommended() const { return m_updateRecommended; }

    void setUpdateRecommended(bool value);

    bool userAuthenticated() const { return m_userAuthenticated; }

    bool startMinimized() const { return m_startMinimized; }

    bool startOnBootSupported() const;

    void setStartMinimized(bool startMinimized) { m_startMinimized = startMinimized; }

    void setToken(const QString &token);

    [[nodiscard]] bool setServerList(const QByteArray& serverData);

    void reset();

    bool modelsInitialized() const;

private:
    void setState(State state);

    void maybeStateMain();

    void scheduleTask(Task *task);
    void maybeRunTask();
    void deleteTasks();

    void setUserAuthenticated(bool state);

    void startSchedulingPeriodicOperations();

    void stopSchedulingPeriodicOperations();

    void setAlert(AlertType alert);

    bool writeAndShowLogs(QStandardPaths::StandardLocation location);

    bool writeLogs(QStandardPaths::StandardLocation location,
                   std::function<void(const QString &filename)> &&a_callback);

#ifdef MVPN_IOS
    void startIAP(bool restore);
#endif

    void completeActivation();

public slots:
    void requestSettings();
    void requestAbout();
    void requestViewLogs();

private slots:
    void taskCompleted();

signals:
    void stateChanged();
    void alertChanged();
    void updateRecommendedChanged();
    void userAuthenticationChanged();
    void deviceRemoving(const QString& deviceName);
    void settingsNeeded();
    void aboutNeeded();
    void viewLogsNeeded();

#ifdef MVPN_ANDROID
    void loadAndroidAuthenticationView();
#endif

private:
    bool m_initialized = false;

    // Internal objects.
    struct Private
    {
        CaptivePortal m_captivePortal;
        CaptivePortalDetection m_captivePortalDetection;
        CloseEventHandler m_closeEventHandler;
        ConnectionDataHolder m_connectionDataHolder;
        ConnectionHealth m_connectionHealth;
        Controller m_controller;
        DeviceModel m_deviceModel;
        Keys m_keys;
        HelpModel m_helpModel;
        ReleaseMonitor m_releaseMonitor;
        ServerCountryModel m_serverCountryModel;
        ServerData m_serverData;
        StatusIcon m_statusIcon;
        User m_user;
    };

    Private *m_private = nullptr;

    // Task handling.
    QList<QPointer<Task>> m_tasks;
    bool m_task_running = false;

    State m_state = StateInitialize;
    AlertType m_alert = NoAlert;

    QTimer m_alertTimer;
    QTimer m_periodicOperationsTimer;

    bool m_updateRecommended = false;
    bool m_userAuthenticated = false;
    bool m_startMinimized = false;

#ifdef UNIT_TEST
   friend class TestTasks;
#endif
};

#endif // MOZILLAVPN_H
