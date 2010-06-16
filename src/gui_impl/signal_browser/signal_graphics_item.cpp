#include "signal_graphics_item.h"
#include "event_graphics_item.h"
#include "signal_browser_model_4.h"
#include "../../editing_commands/new_event_undo_command.h"
#include "y_axis_widget_4.h"
#include "../../file_handling/channel_manager.h"
#include "../../command_executer.h"
#include "../../base/signal_event.h"
#include "../../base/signal_channel.h"
#include "../../base/math_utils.h"
#include "../signal_browser_mouse_handling.h"
#include "../../gui/color_manager.h"
#include "../../gui/gui_action_factory.h"

#include <QStyleOptionGraphicsItem>
#include <QGraphicsSceneMouseEvent>
#include <QPoint>
#include <QTime>
#include <QObject>
#include <QMenu>
#include <QPainter>
#include <QDebug>
#include <QToolTip>

#include <cmath>

namespace BioSig_
{

//-----------------------------------------------------------------------------
SignalGraphicsItem::SignalGraphicsItem (QSharedPointer<EventManager> event_manager,
                                        QSharedPointer<CommandExecuter> command_executor,
                                        QSharedPointer<ChannelManager> channel_manager,
                                        QSharedPointer<ColorManager const> color_manager,
                                        ChannelID id,
                                        SignalBrowserModel& model)
: event_manager_ (event_manager),
  command_executor_ (command_executor),
  channel_manager_ (channel_manager),
  color_manager_ (color_manager),
  id_ (id),
  signal_browser_model_(model),
  minimum_ (channel_manager_->getMinValue (id_)),
  maximum_ (channel_manager_->getMaxValue (id_)),
  y_zoom_ (1),
  draw_y_grid_ (true),
  draw_x_grid_ (true),
  y_offset_ (0),
  height_ (model.getSignalHeight()),
  width_ (0),
  shifting_ (false),
  new_event_ (false),
  created_event_item_ (0),
  hand_tool_on_ (false)
{
#if QT_VERSION >= 0x040600
    setFlag (QGraphicsItem::ItemUsesExtendedStyleOption, true);
#endif    
    float64 value_range = (fabs(maximum_) + fabs(minimum_));
    float64 y_grid_intervall
        = round125(value_range / height_ * signal_browser_model_.getPreferedYGirdPixelIntervall());
    y_grid_pixel_intervall_ = y_zoom_ * y_grid_intervall;
    setAcceptHoverEvents(false);
}

//-----------------------------------------------------------------------------
SignalGraphicsItem::~SignalGraphicsItem ()
{

}


//-----------------------------------------------------------------------------
void SignalGraphicsItem::setHeight (uint32 height)
{
   this->prepareGeometryChange();
   y_zoom_ = y_zoom_ * height / height_;
   y_offset_ = y_offset_* height / height_;
   height_ = height;
   width_ = channel_manager_->getNumberSamples() * signal_browser_model_.getPixelPerSample();
   updateYGridIntervall ();
}

//-----------------------------------------------------------------------------
void SignalGraphicsItem::setXGridInterval (unsigned interval)
{
    x_grid_interval_ = interval;
}

//-----------------------------------------------------------------------------
QRectF SignalGraphicsItem::boundingRect () const
{
    return QRectF (0, 0, width_, height_);
}

//-----------------------------------------------------------------------------
float64 SignalGraphicsItem::getYZoom() const
{
    return y_zoom_;
}

//-----------------------------------------------------------------------------
float64 SignalGraphicsItem::getYOffset() const
{
    return y_offset_;
}

//-----------------------------------------------------------------------------
float64 SignalGraphicsItem::getYGridPixelIntervall() const
{
    return y_grid_pixel_intervall_;
}

//-----------------------------------------------------------------------------
void SignalGraphicsItem::zoomIn()
{
    y_zoom_ *= 2.0;
}

//-----------------------------------------------------------------------------
void SignalGraphicsItem::zoomOut()
{
    y_zoom_ /= 2.0;
}

//-----------------------------------------------------------------------------
void SignalGraphicsItem::scale (double lower_value, double upper_value)
{
    minimum_ = lower_value;
    maximum_ = upper_value;
    float64 abs_max = fabs(upper_value);
    float64 abs_min = fabs(lower_value);

    scaleImpl (abs_min, abs_max);
}


//-----------------------------------------------------------------------------
void SignalGraphicsItem::autoScale (ScaleMode auto_zoom_type)
{
    minimum_ = channel_manager_->getMinValue (id_);
    maximum_ = channel_manager_->getMaxValue (id_);
    float64 abs_max = fabs(maximum_);
    float64 abs_min = fabs(minimum_);

    if (auto_zoom_type == MAX_TO_MAX)
    {
        if (abs_max > abs_min)
            abs_min = abs_max;
        else
            abs_max = abs_min;
    }
    scaleImpl (abs_min, abs_max);
}

//-----------------------------------------------------------------------------
void SignalGraphicsItem::scaleImpl (double abs_min, double abs_max)
{
    float64 new_y_zoom = height_ / (abs_max + abs_min);
    float64 new_y_offset = ((abs_max - abs_min) / 2) * new_y_zoom;
    if (y_zoom_ != new_y_zoom ||
        y_offset_ != new_y_offset)
    {
        y_zoom_ = new_y_zoom;
        y_offset_ = new_y_offset;
        updateYGridIntervall ();
    }
}


//-----------------------------------------------------------------------------
void SignalGraphicsItem::paint (QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget*)
{
    painter->drawRect(boundingRect());
    if (new_event_)
        painter->fillRect(new_signal_event_->getPosition(), 0, new_signal_event_->getDuration(), height_, new_event_color_);

    QRectF clip (option->exposedRect);

    painter->setClipping(true);
    painter->setClipRect(clip);

    float32 pixel_per_sample = signal_browser_model_.getPixelPerSample();

    float32 last_x = clip.x () - pixel_per_sample;
    if (last_x < 0)
        last_x = 0;
    unsigned start_sample = last_x / pixel_per_sample;
    if (start_sample > 0)
        start_sample--;

    unsigned length = ((clip.x() - start_sample * pixel_per_sample)
                       + clip.width() + pixel_per_sample);
    if (last_x + length > width_)
        length = width_ - last_x;

    length /= pixel_per_sample;
    if (length < channel_manager_->getNumberSamples() - start_sample)
        length++;

    QSharedPointer<DataBlock const> data_block = channel_manager_->getData (id_, start_sample, length);

    last_x = start_sample * pixel_per_sample;
    float32 last_y = (*data_block)[0];
    float32 new_y = 0;

    if (draw_x_grid_)
        drawXGrid (painter, option);
    painter->translate (0, height_ / 2.0f);
    if (draw_y_grid_)
        drawYGrid (painter, option);
    painter->setPen (color_manager_->getChannelColor (id_));

    int sub_sampler = 1;
    while (pixel_per_sample * 4 <= 1)
    {
        pixel_per_sample *= 2;
        sub_sampler *= 2;
    }

    for (int index = 0;
         index < static_cast<int>(data_block->size()) - sub_sampler;
         index += sub_sampler)
    {
        new_y = (*data_block)[index+1];
        painter->drawLine(last_x, y_offset_ - (y_zoom_ * last_y), last_x + pixel_per_sample, y_offset_ - (y_zoom_ * new_y));

        last_x += pixel_per_sample;
        last_y = new_y;
    }
    return;
}

//-----------------------------------------------------------------------------
void SignalGraphicsItem::updateYGridIntervall ()
{
    float64 value_range = (fabs(maximum_) + fabs(minimum_));
    float64 y_grid_intervall
        = round125(value_range / height_ * signal_browser_model_.getPreferedYGirdPixelIntervall());
    y_grid_pixel_intervall_ = y_zoom_ * y_grid_intervall;

    emit updatedYGrid (id_);
}

//-----------------------------------------------------------------------------
void SignalGraphicsItem::enableYGrid(bool enabled)
{
    draw_y_grid_ = enabled;
}

//-----------------------------------------------------------------------------
void SignalGraphicsItem::enableXGrid(bool enabled)
{
    draw_x_grid_ = enabled;
}

//-----------------------------------------------------------------------------
void SignalGraphicsItem::mouseMoveEvent (QGraphicsSceneMouseEvent* event)
{
    QPoint p = event->screenPos();
    if (shifting_)
    {
        int32 dy = p.y() - move_start_point_.y();
        move_start_point_ = p;
        move_start_point_ = p;
        y_offset_ = y_offset_ + dy;
        update();
        emit shifting (id_);
    }
    else if (new_event_)
    {
        float32 pixel_per_sample = signal_browser_model_.getPixelPerSample ();
        int32 sample_cleaned_pos = event->scenePos().x() / pixel_per_sample + 0.5;
        sample_cleaned_pos *= pixel_per_sample;
        int32 new_event_width = new_signal_event_->getDuration ();
        uint32 old_pos = new_signal_event_->getPosition ();
        uint32 old_width = new_signal_event_->getDuration ();

        if (sample_cleaned_pos < new_signal_event_reference_x_)
        {
            new_event_width = new_signal_event_reference_x_ - sample_cleaned_pos;
            new_signal_event_->setPosition (sample_cleaned_pos);
        }
        else
        {
            new_signal_event_->setPosition (new_signal_event_reference_x_);
            new_event_width = sample_cleaned_pos - new_signal_event_->getPosition();
        }

        if (new_event_width < 0)
            new_event_width = 0;

        new_signal_event_->setDuration (new_event_width);

        int32 update_start = std::min(old_pos, new_signal_event_->getPosition());
        int32 update_end = std::max(old_pos + old_width, new_event_width + new_signal_event_->getPosition ());
        update (update_start, 0, update_end - update_start, height_);

        emit mouseAtSecond (sample_cleaned_pos / pixel_per_sample *
                                   channel_manager_->getSampleRate());
    }
    else
        event->ignore();
}

//-----------------------------------------------------------------------------
void SignalGraphicsItem::hoverMoveEvent (QGraphicsSceneHoverEvent* event)
{
    unsigned sample_pos = event->scenePos().x() / signal_browser_model_.getPixelPerSample();

    emit mouseMoving (true);
    emit mouseAtSecond (sample_pos / channel_manager_->getSampleRate());

    if (event_manager_.isNull())
        return;

    std::set<EventID> events = event_manager_->getEventsAt (sample_pos, id_);
    QString event_string;
    foreach (EventID event, events)
    {
        if (event_string.size())
            event_string += "<br /><br />";
        QSharedPointer<SignalEvent const> signal_event = event_manager_->getEvent(event);
        event_string += "<b>" + event_manager_->getNameOfEvent (event) + "</b><br />";
        event_string += "Duration: " + QString::number(signal_event->getDurationInSec()) + "s";
    }

    if (event_string.size())
    {
        QToolTip::showText (event->screenPos(), event_string);
        setToolTip (event_string);
    }
    else
    {
        setToolTip ("");
        QToolTip::hideText();
    }
}


//-----------------------------------------------------------------------------
void SignalGraphicsItem::mousePressEvent (QGraphicsSceneMouseEvent * event )
{
    SignalVisualisationMode mode = signal_browser_model_.getMode();

    switch (SignalBrowserMouseHandling::getAction(event, mode))
    {
        case SignalBrowserMouseHandling::HAND_SCROLL_ACTION:
            hand_tool_on_ = true;
            event->ignore();
            break;
        case SignalBrowserMouseHandling::SHIFT_CHANNEL_ACTION:
            shifting_ = true;

            setCursor(QCursor(Qt::SizeVerCursor));
            move_start_point_ = event->screenPos();
            break;
        case SignalBrowserMouseHandling::NEW_EVENT_ACTION:
            {             
            if (signal_browser_model_.getShownEventTypes ().size() == 0)
                break;

            float32 pixel_per_sample = signal_browser_model_.getPixelPerSample ();
            int32 sample_cleaned_pos = event->scenePos().x() / pixel_per_sample + 0.5;
            sample_cleaned_pos *= pixel_per_sample;
            new_event_ = true;
            new_signal_event_ = QSharedPointer<SignalEvent>(new SignalEvent(sample_cleaned_pos,
                                                                            signal_browser_model_.getActualEventCreationType(),
                                                                            event_manager_->getSampleRate(),
                                                                            id_));
            new_event_color_ = color_manager_->getEventColor(signal_browser_model_.getActualEventCreationType());
            new_signal_event_reference_x_ = sample_cleaned_pos;
            emit mouseMoving (true);
            break;
        }
            break;
        default:
            event->accept();
            if (!EventGraphicsItem::displaySelectionMenu (event))
                signal_browser_model_.unselectEvent ();
    }
}

//-----------------------------------------------------------------------------
void SignalGraphicsItem::mouseReleaseEvent (QGraphicsSceneMouseEvent* event)
{
    if (hand_tool_on_)
        event->ignore();

    if (new_event_)
    {
        emit mouseMoving (false);
        NewEventUndoCommand* new_event_command = new NewEventUndoCommand (event_manager_, new_signal_event_, 1.0 / signal_browser_model_.getPixelPerSample());
        command_executor_->executeCommand (new_event_command);
    }

    shifting_ = false;
    hand_tool_on_ = false;
    new_event_ = false;
    setCursor(QCursor());
}

//-----------------------------------------------------------------------------
void SignalGraphicsItem::contextMenuEvent (QGraphicsSceneContextMenuEvent * event)
{
    signal_browser_model_.selectChannel (id_);
    QMenu* context_menu = new QMenu (channel_manager_->getChannelLabel(id_));
    context_menu->addAction(GuiActionFactory::getInstance()->getQAction("Change Color..."));
    context_menu->addAction(GuiActionFactory::getInstance()->getQAction("Scale..."));
    if (signal_browser_model_.getShownChannels().size() > 1)
    {
        context_menu->addSeparator ();
        context_menu->addAction(GuiActionFactory::getInstance()->getQAction("Hide Channel"));
    }

    if (!EventGraphicsItem::displayContextMenu (event, context_menu))
        context_menu->exec(event->screenPos());
    else
        event->accept();
}

//-----------------------------------------------------------------------------
void SignalGraphicsItem::wheelEvent (QGraphicsSceneWheelEvent* event)
{
    if (event->modifiers().testFlag(Qt::ControlModifier))
    {
        if (event->delta() > 0)
            y_zoom_ *= event->delta() / 60;
        else if (event->delta() < 0)
            y_zoom_ /= -(event->delta()) / 60;
        update ();
        emit shifting (id_);
    }
    else if (event->modifiers().testFlag(Qt::ShiftModifier))
    {
        if (event->delta() > 0)
            signal_browser_model_.zoomInAll ();
        else if (event->delta() < 0)
            signal_browser_model_.zoomOutAll ();
    }
    else
        event->ignore();
}


//-----------------------------------------------------------------------------
void SignalGraphicsItem::drawYGrid (QPainter* painter,
                                    QStyleOptionGraphicsItem const* option)
{
    QRectF clip (option->exposedRect);
    painter->setPen (Qt::lightGray);

    for (float64 y = y_offset_;
         y < height_ / 2;
         y += y_grid_pixel_intervall_)
    {
        if (y > -static_cast<int>(height_ / 2))
        {
            painter->drawLine (clip.x(), y, clip.x() + clip.width(), y);
        }
    }

    for (float64 y = y_offset_;
         y > -static_cast<int>(height_ / 2);
         y -= y_grid_pixel_intervall_)
    {
        if (y < height_ / 2)
        {
            painter->drawLine (clip.x(), y, clip.x() + clip.width(), y);
        }
    }

}

//-----------------------------------------------------------------------------
void SignalGraphicsItem::drawXGrid (QPainter* painter,
                                    QStyleOptionGraphicsItem const* option)
{
    QRectF clip (option->exposedRect);
    painter->setPen (Qt::lightGray);

    if (clip.width() < 1)
        return;

    for (int x = clip.x() - (static_cast<int>(clip.x()) % x_grid_interval_);
         x < clip.x() + clip.width(); x += x_grid_interval_)
        painter->drawLine (x, 0, x, height_);
}


}