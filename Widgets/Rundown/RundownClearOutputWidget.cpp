#include "RundownClearOutputWidget.h"

#include "Global.h"

#include "DeviceManager.h"
#include "GpiManager.h"
#include "Events/ConnectionStateChangedEvent.h"
#include "Events/RundownItemChangedEvent.h"

#include <QtCore/QObject>
#include <QtCore/QTimer>

RundownClearOutputWidget::RundownClearOutputWidget(const LibraryModel& model, QWidget* parent, const QString& color, bool active,
                                                   bool inGroup, bool disconnected, bool compactView)
    : QWidget(parent),
      active(active), inGroup(inGroup), disconnected(disconnected), compactView(compactView), color(color), model(model)
{
    setupUi(this);

    this->animation = new ActiveAnimation(this->labelActiveColor);

    setColor(color);
    setActive(active);
    setCompactView(compactView);

    this->labelDisconnected->setVisible(this->disconnected);
    this->labelGroupColor->setVisible(this->inGroup);
    this->labelGroupColor->setStyleSheet(QString("background-color: %1;").arg(Color::DEFAULT_GROUP_COLOR));
    this->labelColor->setStyleSheet(QString("background-color: %1;").arg(Color::DEFAULT_CLEAR_OUTPUT_COLOR));

    this->labelLabel->setText(this->model.getLabel());
    this->labelChannel->setText(QString("Channel: %1").arg(this->command.getChannel()));
    this->labelDelay->setText(QString("Delay: %1").arg(this->command.getDelay()));
    this->labelDevice->setText(QString("Device: %1").arg(this->model.getDeviceName()));

    this->executeTimer.setSingleShot(true);

    QObject::connect(&this->command, SIGNAL(channelChanged(int)), this, SLOT(channelChanged(int)));
    QObject::connect(&this->command, SIGNAL(delayChanged(int)), this, SLOT(delayChanged(int)));
    QObject::connect(&this->command, SIGNAL(allowGpiChanged(bool)), this, SLOT(allowGpiChanged(bool)));
    QObject::connect(GpiManager::getInstance().getGpiDevice().data(), SIGNAL(connectionStateChanged(bool, GpiDevice*)),
                     this, SLOT(gpiDeviceConnected(bool, GpiDevice*)));

    checkEmptyDevice();
    checkGpiTriggerable();

    qApp->installEventFilter(this);
}

bool RundownClearOutputWidget::eventFilter(QObject* target, QEvent* event)
{
    if (event->type() == static_cast<QEvent::Type>(Enum::EventType::RundownItemChanged))
    {
        // This event is not for us.
        if (!this->active)
            return false;

        RundownItemChangedEvent* rundownItemChangedEvent = dynamic_cast<RundownItemChangedEvent*>(event);
        this->model.setLabel(rundownItemChangedEvent->getLabel());
        this->model.setDeviceName(rundownItemChangedEvent->getDeviceName());
        this->model.setName(rundownItemChangedEvent->getName());

        this->labelLabel->setText(this->model.getLabel());
        this->labelDevice->setText(QString("Device: %1").arg(this->model.getDeviceName()));

        checkEmptyDevice();
    }
    else if (event->type() == static_cast<QEvent::Type>(Enum::EventType::ConnectionStateChanged))
    {
        ConnectionStateChangedEvent* connectionStateChangedEvent = dynamic_cast<ConnectionStateChangedEvent*>(event);
        if (connectionStateChangedEvent->getDeviceName() == this->model.getDeviceName())
        {
            this->disconnected = !connectionStateChangedEvent->getConnected();

            if (connectionStateChangedEvent->getConnected())
                this->labelDisconnected->setVisible(false);
            else
                this->labelDisconnected->setVisible(true);
        }
    }

    return QObject::eventFilter(target, event);
}

AbstractRundownWidget* RundownClearOutputWidget::clone()
{
    RundownClearOutputWidget* widget = new RundownClearOutputWidget(this->model, this->parentWidget(), this->color, this->active,
                                                                    this->inGroup, this->disconnected, this->compactView);

    ClearOutputCommand* command = dynamic_cast<ClearOutputCommand*>(widget->getCommand());
    command->setChannel(this->command.getChannel());
    command->setVideolayer(this->command.getVideolayer());
    command->setDelay(this->command.getDelay());
    command->setAllowGpi(this->command.getAllowGpi());
    command->setClearChannel(this->command.getClearChannel());
    command->setTriggerOnNext(this->command.getTriggerOnNext());

    return widget;
}

void RundownClearOutputWidget::setCompactView(bool compactView)
{
    if (compactView)
    {
        this->labelThumbnail->setFixedSize(Define::COMPACT_ICON_WIDTH, Define::COMPACT_ICON_HEIGHT);
        this->labelGpiConnected->setFixedSize(Define::COMPACT_ICON_WIDTH, Define::COMPACT_ICON_HEIGHT);
        this->labelDisconnected->setFixedSize(Define::COMPACT_ICON_WIDTH, Define::COMPACT_ICON_HEIGHT);
    }
    else
    {
        this->labelThumbnail->setFixedSize(Define::DEFAULT_ICON_WIDTH, Define::DEFAULT_ICON_HEIGHT);
        this->labelGpiConnected->setFixedSize(Define::DEFAULT_ICON_WIDTH, Define::DEFAULT_ICON_HEIGHT);
        this->labelDisconnected->setFixedSize(Define::DEFAULT_ICON_WIDTH, Define::DEFAULT_ICON_HEIGHT);
    }

    this->compactView = compactView;
}

void RundownClearOutputWidget::readProperties(boost::property_tree::wptree& pt)
{
    if (pt.count(L"color") > 0) setColor(QString::fromStdWString(pt.get<std::wstring>(L"color")));
}

void RundownClearOutputWidget::writeProperties(QXmlStreamWriter* writer)
{
    writer->writeTextElement("color", this->color);
}

bool RundownClearOutputWidget::isGroup() const
{
    return false;
}

AbstractCommand* RundownClearOutputWidget::getCommand()
{
    return &this->command;
}

LibraryModel* RundownClearOutputWidget::getLibraryModel()
{
    return &this->model;
}

void RundownClearOutputWidget::setActive(bool active)
{
    this->active = active;

    this->animation->stop();

    if (this->active)
        this->labelActiveColor->setStyleSheet("background-color: red;");
    else
        this->labelActiveColor->setStyleSheet("");
}

void RundownClearOutputWidget::setInGroup(bool inGroup)
{
    this->inGroup = inGroup;
    this->labelGroupColor->setVisible(inGroup);

    if (this->inGroup)
    {
        this->labelChannel->setGeometry(this->labelChannel->geometry().x() + Define::GROUP_XPOS_OFFSET,
                                        this->labelChannel->geometry().y(),
                                        this->labelChannel->geometry().width(),
                                        this->labelChannel->geometry().height());
        this->labelDelay->setGeometry(this->labelDelay->geometry().x() + Define::GROUP_XPOS_OFFSET,
                                      this->labelDelay->geometry().y(),
                                      this->labelDelay->geometry().width(),
                                      this->labelDelay->geometry().height());
        this->labelDevice->setGeometry(this->labelDevice->geometry().x() + Define::GROUP_XPOS_OFFSET,
                                       this->labelDevice->geometry().y(),
                                       this->labelDevice->geometry().width(),
                                       this->labelDevice->geometry().height());
    }
}

void RundownClearOutputWidget::setColor(const QString& color)
{
    this->color = color;
    this->setStyleSheet(QString("#frameItem, #frameStatus { background-color: rgba(%1); }").arg(color));
}

void RundownClearOutputWidget::checkEmptyDevice()
{
    if (this->labelDevice->text() == "Device: ")
        this->labelDevice->setStyleSheet("color: black;");
    else
        this->labelDevice->setStyleSheet("");
}

bool RundownClearOutputWidget::executeCommand(enum Playout::PlayoutType::Type type)
{
    if (type == Playout::PlayoutType::Stop)
        QTimer::singleShot(0, this, SLOT(executeStop()));
    else if ((type == Playout::PlayoutType::Play && !this->command.getTriggerOnNext()) ||
             (type == Playout::PlayoutType::Next && this->command.getTriggerOnNext()))
    {  
        QObject::disconnect(&this->executeTimer, SIGNAL(timeout()), this, SLOT(executeClearChannel()));
        QObject::disconnect(&this->executeTimer, SIGNAL(timeout()), this, SLOT(executeClearVideolayer()));

        if (this->command.getClearChannel())
            QObject::connect(&this->executeTimer, SIGNAL(timeout()), this, SLOT(executeClearChannel()));
        else
            QObject::connect(&this->executeTimer, SIGNAL(timeout()), this, SLOT(executeClearVideolayer()));

        this->executeTimer.setInterval(this->command.getDelay());
        this->executeTimer.start();
    }
    else if (type == Playout::PlayoutType::Clear)
        QTimer::singleShot(0, this, SLOT(executeClearVideolayer()));
    else if (type == Playout::PlayoutType::ClearVideolayer)
        QTimer::singleShot(0, this, SLOT(executeClearVideolayer()));
    else if (type == Playout::PlayoutType::ClearChannel)
        QTimer::singleShot(0, this, SLOT(executeClearChannel()));

    if (this->active)
        this->animation->start(1);

    return true;
}

void RundownClearOutputWidget::executeStop()
{
    this->executeTimer.stop();
}

void RundownClearOutputWidget::executeClearVideolayer()
{
    this->executeTimer.stop();

    const QSharedPointer<CasparDevice> device = DeviceManager::getInstance().getConnectionByName(this->model.getDeviceName());
    if (device != NULL && device->isConnected())
        device->clearVideolayer(this->command.getChannel(), this->command.getVideolayer());

    foreach (const DeviceModel& model, DeviceManager::getInstance().getDeviceModels())
    {
        if (model.getShadow() == "No")
            continue;

        const QSharedPointer<CasparDevice> deviceShadow = DeviceManager::getInstance().getConnectionByName(model.getName());
        if (deviceShadow != NULL && deviceShadow->isConnected())
            deviceShadow->clearVideolayer(this->command.getChannel(), this->command.getVideolayer());
    }
}

void RundownClearOutputWidget::executeClearChannel()
{
    this->executeTimer.stop();

    const QSharedPointer<CasparDevice> device = DeviceManager::getInstance().getConnectionByName(this->model.getDeviceName());
    if (device != NULL && device->isConnected())
    {
        device->clearChannel(this->command.getChannel());
        device->clearMixerChannel(this->command.getChannel());
    }

    foreach (const DeviceModel& model, DeviceManager::getInstance().getDeviceModels())
    {
        if (model.getShadow() == "No")
            continue;

        const QSharedPointer<CasparDevice> deviceShadow = DeviceManager::getInstance().getConnectionByName(model.getName());
        if (deviceShadow != NULL && deviceShadow->isConnected())
        {
            deviceShadow->clearChannel(this->command.getChannel());
            deviceShadow->clearMixerChannel(this->command.getChannel());
        }
    }
}

void RundownClearOutputWidget::channelChanged(int channel)
{
    this->labelChannel->setText(QString("Channel: %1").arg(channel));
}

void RundownClearOutputWidget::delayChanged(int delay)
{
    this->labelDelay->setText(QString("Delay: %1").arg(delay));
}

void RundownClearOutputWidget::checkGpiTriggerable()
{
    labelGpiConnected->setVisible(this->command.getAllowGpi());

    if (GpiManager::getInstance().getGpiDevice()->isConnected())
        this->labelGpiConnected->setPixmap(QPixmap(":/Graphics/Images/GpiConnected.png"));
    else
        this->labelGpiConnected->setPixmap(QPixmap(":/Graphics/Images/GpiDisconnected.png"));
}

void RundownClearOutputWidget::allowGpiChanged(bool allowGpi)
{
    checkGpiTriggerable();
}

void RundownClearOutputWidget::gpiDeviceConnected(bool connected, GpiDevice* device)
{
    checkGpiTriggerable();
}