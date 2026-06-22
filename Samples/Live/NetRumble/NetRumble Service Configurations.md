# "**NetRumble Service Configurations**"

This document contains the service configuration data used by the
NetRumble sample for the various Xbox Live enabled features.

## Multiplayer & Matchmaking

Below are the templates provisioned on the developer portal for the
sample to enable Xbox multiplayer and matchmaking functionality.

| **GameSession**  |  Contract Version: 107                 |
|------------------------------------------------|---------------------|
| { \"constants\": { \"system\": { \"version\": 1, \"maxMembersCount\": 8, \"visibility\": \"open\", \"inviteProtocol\": \"game\", \"capabilities\": { \"connectivity\": true, \"connectionRequiredForActiveMembers\": true, \"gameplay\" : true, \"crossPlay\": true, \"userAuthorizationStyle\": true }, }, \"custom\": {} } } |  |

| **LobbySession**  |  Contract Version: 107                 |
|------------------------------------------------|---------------------|
| { \"constants\": { \"system\": { \"version\": 1, \"maxMembersCount\": 8, \"visibility\": \"open\", \"inviteProtocol\": \"game\", \"capabilities\": { \"connectivity\": true, \"connectionRequiredForActiveMembers\": true, \"crossPlay\": true, \"userAuthorizationStyle\": true }, }, \"custom\": {} } } |  |

