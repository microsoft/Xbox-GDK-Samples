  ![](./media/image1.png)

#   InGameChat Sample

*This sample is compatible with the Microsoft Game Development Kit (June
2020)*

# 

# Description

The InGameChat sample provides a working example of integrating the
GameChat2 library into an Xbox title. It brings together the pieces
needed to demonstrate in-title VOIP communications: GameChat,
Multiplayer Sessions, and Peer Networking.

# Building the sample

When building for PC select Gaming.Desktop.x64 platform configurations.

*For more information, see* __Running samples__, *in the GDK documentation.*

# Using the sample

You will need at least two participants each with a microphone and
speaker. These can be Kinect, mono and stereo headsets, TV speakers,
etc. You will need a valid Xbox Live user for each participant and it
helps if they are friends. The PCs, Xboxes and user accounts should be
configured for the XDKS.1 sandbox.

## ![](./media/image3.png)Local Lobby

| Action                                           |  Gamepad           |
|--------------------------------------------------|-------------------|
| Start your own chat session where you can invite users or they can join you |  A button |
| Search for open chat sessions in your social graph |  X button |

## ![](./media/image4.png)Select A Session

| Action                                          |  Gamepad            |
|-------------------------------------------------|--------------------|
| Select between list items                       |  D-Pad Up/Down      |
| Refresh the list                                |  X button           |
| Go back                                         |  B button           |

## Chat Session

![](./media/image5.png)

| Action                                 |  Gamepad                     |
|----------------------------------------|-----------------------------|
| Select a user in the list              |  D-Pad Up/Down               |
| Toggle mute for selected user          |  A button                    |
| Send a text message                    |  X button                    |
| Invite users                           |  Y button                    |
| Go back                                |  B button                    |

# Implementation notes

The main purpose of this sample is to demonstrate integrating GameChat2
into your title. GameChat2 is controlled through the
xbox::services::game_chat_2::chat_manager::singleton_instance() class.
This class is wrapped up into the GameChatManager. The manager is the
interface between GameChat and your title. It handles interfacing with
the peer network and multiplayer session members.

The GameChatManager.cpp/.h files are called out to aid in understanding
and should be the primary focus of the sample.

The remaining code is there to facilitate end-to-end VOIP. It was
written to be simple and straightforward rather than performant or
production ready. We do not recommend using other parts of the sample
directly or to use it as a canonical example for things other than its
intended purpose.

# Update history

March, 2020 -- Created from InGameChat sample
