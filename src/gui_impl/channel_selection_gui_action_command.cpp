#include "channel_selection_gui_action_command.h"
#include "../gui/gui_action_factory.h"

#include <iostream>

namespace BioSig_
{

//-----------------------------------------------------------------------------
GuiActionFactoryRegistrator ChannelSelectionGUIActionCommand::registrator_ ("ChannelSelection",
                                                                            QSharedPointer<GuiActionCommand>(new ChannelSelectionGUIActionCommand));

//-----------------------------------------------------------------------------
ChannelSelectionGUIActionCommand::ChannelSelectionGUIActionCommand ()
    : GuiActionCommand ("ChannelSelection")
{

}

//-----------------------------------------------------------------------------
ChannelSelectionGUIActionCommand::~ChannelSelectionGUIActionCommand ()
{
    // todo: implement?
}

//-----------------------------------------------------------------------------
void ChannelSelectionGUIActionCommand::init ()
{
    // todo: implement!!
}

//-----------------------------------------------------------------------------
void ChannelSelectionGUIActionCommand::trigger ()
{
    std::cout << "triggered" << std::endl;
}

}