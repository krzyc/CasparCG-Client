#pragma once

#include "Shared.h"

#include <QtCore/QObject>
#include <QtCore/QString>

#include <QtNetwork/QNetworkReply>
#include <QtNetwork/QNetworkAccessManager>

class PANASONIC_EXPORT PtzDevice : public QObject
{
    Q_OBJECT

    public:
        explicit PtzDevice(QObject* parent = 0);

        void selectPreset(const QString& address, int preset);

    private:
        QNetworkAccessManager* networkManager;

        Q_SLOT void selectPresetFinished(QNetworkReply*);
};
